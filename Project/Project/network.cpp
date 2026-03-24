#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "network.h"
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>

static SOCKET            g_sock = INVALID_SOCKET;
static int               g_pid = -1;
static std::atomic<bool> g_conn{ false };
static std::atomic<bool> g_started{ false };
static std::mutex        g_mu;
static std::string       g_lastMsg;
static bool              g_hasMsg = false;
static std::string       g_nickMsg;
static bool              g_hasNick = false;

static std::atomic<bool> g_remoteReady{ false };
static std::atomic<bool> g_remoteRematch{ false };


static std::atomic<float> g_ping{ 0.0f };
static Uint32 g_pingTime = 0;

static char g_recvBuf[65536];
static int  g_recvLen = 0;

static std::string RecvLine() {
    while (true) {
        // Check if we already have a complete line in buffer
        for (int i = 0; i < g_recvLen; i++) {
            if (g_recvBuf[i] == '\n') {
                std::string line(g_recvBuf, i);
                int remaining = g_recvLen - i - 1;
                if (remaining > 0)
                    memmove(g_recvBuf, g_recvBuf + i + 1, remaining);
                g_recvLen = remaining;
                return line;
            }
        }
        // Need more data
        int r = recv(g_sock, g_recvBuf + g_recvLen, sizeof(g_recvBuf) - g_recvLen, 0);
        if (r <= 0) throw std::runtime_error("disconnected");
        g_recvLen += r;
    }
}

static bool SendLine(const std::string& msg) {
    std::string s = msg + '\n';
    return send(g_sock, s.c_str(), (int)s.size(), 0) != SOCKET_ERROR;
}

static float GetF(const std::string& j, const char* key) {
    std::string k = std::string("\"") + key + "\":";
    auto p = j.find(k);
    if (p == std::string::npos) return 0.f;
    return (float)atof(j.c_str() + p + k.size());
}

static int GetI(const std::string& j, const char* key) {
    std::string k = std::string("\"") + key + "\":";
    auto p = j.find(k);
    if (p == std::string::npos) return 0;
    return atoi(j.c_str() + p + k.size());
}

static bool GetB(const std::string& j, const char* key) {
    std::string k = std::string("\"") + key + "\":";
    auto p = j.find(k);
    if (p == std::string::npos) return false;
    char c = j[p + k.size()];
    return c == 't' || c == '1';
}

static std::string GetS(const std::string& j, const char* key) {
    std::string k = std::string("\"") + key + "\":\"";
    auto p = j.find(k);
    if (p == std::string::npos) return "";
    p += k.size();
    auto e = j.find('"', p);
    return j.substr(p, e - p);
}

static void RecvThread() {
    try {
        while (g_conn && g_sock != INVALID_SOCKET) {
            std::string line = RecvLine();
            if (line.find("START") != std::string::npos)
                g_started = true;
            if (line.find("player_id") != std::string::npos && g_pid == -1)
                g_pid = GetI(line, "player_id");
            if (line.find("\"ready\"") != std::string::npos) {
                g_remoteReady = true;
            }
            if (line.find("\"rematch\"") != std::string::npos) {
                g_remoteRematch = true;
            }
            if (line.find("\"pong\"") != std::string::npos) {
                Uint32 now = SDL_GetTicks();
                g_ping = (float)(now - g_pingTime);
            }
            if (line.find("\"ping\"") != std::string::npos) {
                SendLine("{\"type\":\"pong\"}");
            }
            if (line.find("\"nickname\"") != std::string::npos) {
                std::lock_guard<std::mutex> lk(g_mu);
                g_nickMsg = line;
                g_hasNick = true;
            }
            else if (line.find("\"input\"") != std::string::npos ||
                line.find("\"state\"") != std::string::npos) {
                std::lock_guard<std::mutex> lk(g_mu);
                g_lastMsg = line;
                g_hasMsg = true;
            }
        }
    }
    catch (...) {
        g_conn = false;
    }
}

bool NetInit() {
    WSADATA wd;
    return WSAStartup(MAKEWORD(2, 2), &wd) == 0;
}

bool NetConnect(const char* ip, int port) {
    // Clean up any previous connection
    if (g_sock != INVALID_SOCKET) {
        g_conn = false;
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
        SDL_Delay(100); // Let old recv thread die
    }

    g_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_sock == INVALID_SOCKET) return false;

    int flag = 1;
    setsockopt(g_sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    if (connect(g_sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        return false;
    g_conn = true;
    g_started = false;
    g_pid = -1;
    g_remoteReady = false;
    g_hasNick = false;
    g_hasMsg = false;
    g_lastMsg.clear();
    g_nickMsg.clear();
    g_remoteRematch = false;
    g_recvLen = 0;

    std::thread(RecvThread).detach();
    return true;
}

void NetDisconnect() {
    g_conn = false;
    if (g_sock != INVALID_SOCKET) {
        shutdown(g_sock, SD_BOTH);
        closesocket(g_sock);
        g_sock = INVALID_SOCKET;
    }
    SDL_Delay(50); // Let recv thread finish
    g_started = false;
    g_pid = -1;
    g_remoteReady = false;
    g_remoteRematch = false;
    g_hasMsg = false;
    g_hasNick = false;
    g_recvLen = 0;
    g_lastMsg.clear();
    g_nickMsg.clear();
    g_ping = 0.0f;
}

bool NetIsConnected() { return g_conn; }
int  NetGetPlayerId() { return g_pid; }
bool NetIsStarted() { return g_started; }

void NetSendInput(bool thrust, bool left, bool right, bool shoot) {
    char buf[128];
    snprintf(buf, sizeof(buf),
        "{\"type\":\"input\",\"tf\":%d,\"rl\":%d,\"rr\":%d,\"sh\":%d}",
        thrust ? 1 : 0, left ? 1 : 0, right ? 1 : 0, shoot ? 1 : 0);
    if (!SendLine(buf)) {
        g_conn = false;
    }
}

void NetSendState(const GameState& gs,
    const std::vector<Asteroid>& ast,
    const std::vector<Projectile>& prj)
{
    char buf[8192];
    int len = 0;
    len += snprintf(buf + len, sizeof(buf) - len,
        "{\"type\":\"state\","
        "\"cs\":%d,"
        "\"p0x\":%.3f,\"p0z\":%.3f,\"p0r\":%.3f,"
        "\"p0vx\":%.3f,\"p0vz\":%.3f,"
        "\"p0l\":%d,\"p0s\":%d,\"p0si\":%.3f,\"p0it\":%.3f,\"p0a\":%d,"
        "\"p1x\":%.3f,\"p1z\":%.3f,\"p1r\":%.3f,"
        "\"p1vx\":%.3f,\"p1vz\":%.3f,"
        "\"p1l\":%d,\"p1s\":%d,\"p1si\":%.3f,\"p1it\":%.3f,\"p1a\":%d,"
        "\"rt\":%.3f,\"rn\":%d,\"rw0\":%d,\"rw1\":%d,\"rwn\":%d,\"ret\":%.3f,",
        (int)gs.currentState,
        gs.players[0].x, gs.players[0].z, gs.players[0].rotation,
        gs.players[0].vx, gs.players[0].vz,
        gs.players[0].lives, gs.players[0].score,
        gs.players[0].shootCooldown, gs.players[0].invulnTime,
        gs.players[0].alive ? 1 : 0,
        gs.players[1].x, gs.players[1].z, gs.players[1].rotation,
        gs.players[1].vx, gs.players[1].vz,
        gs.players[1].lives, gs.players[1].score,
        gs.players[1].shootCooldown, gs.players[1].invulnTime,
        gs.players[1].alive ? 1 : 0,
        gs.roundTimer, gs.roundNumber,
        gs.roundWins[0], gs.roundWins[1],
        gs.roundWinner, gs.roundEndTimer);
    len += snprintf(buf + len, sizeof(buf) - len, "\"ast\":\"");
    for (const auto& a : ast)
        len += snprintf(buf + len, sizeof(buf) - len,
            "%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d;",
            a.id, a.x, a.z, a.vx, a.vz,
            a.rotation, a.rotationspeed, a.radius, a.size);
    len += snprintf(buf + len, sizeof(buf) - len, "\",");
    len += snprintf(buf + len, sizeof(buf) - len, "\"prj\":\"");
    for (const auto& p : prj)
        len += snprintf(buf + len, sizeof(buf) - len,
            "%d,%d,%.3f,%.3f,%.3f,%.3f;",
            p.id, p.ownerId, p.x, p.z, p.vx, p.vz);
    len += snprintf(buf + len, sizeof(buf) - len, "\"}");
    if (!SendLine(buf)) {
        g_conn = false;
    }
}

bool NetGetInput(bool& thrust, bool& left, bool& right, bool& shoot) {
    std::lock_guard<std::mutex> lk(g_mu);
    if (!g_hasMsg) return false;
    if (g_lastMsg.find("\"input\"") == std::string::npos) return false;
    g_hasMsg = false;
    thrust = GetB(g_lastMsg, "tf");
    left = GetB(g_lastMsg, "rl");
    right = GetB(g_lastMsg, "rr");
    shoot = GetB(g_lastMsg, "sh");
    return true;
}

bool NetGetState(GameState& gs,
    std::vector<Asteroid>& ast,
    std::vector<Projectile>& prj)
{
    std::string msg;
    {
        std::lock_guard<std::mutex> lk(g_mu);
        if (!g_hasMsg) return false;
        if (g_lastMsg.find("\"state\"") == std::string::npos) return false;
        g_hasMsg = false;
        msg = g_lastMsg;
    }
    gs.currentState = (GameStateEnum)GetI(msg, "cs");
    gs.players[0].id = 0;
    gs.players[0].x = GetF(msg, "p0x");
    gs.players[0].z = GetF(msg, "p0z");
    gs.players[0].rotation = GetF(msg, "p0r");
    gs.players[0].vx = GetF(msg, "p0vx");
    gs.players[0].vz = GetF(msg, "p0vz");
    gs.players[0].lives = GetI(msg, "p0l");
    gs.players[0].score = GetI(msg, "p0s");
    gs.players[0].shootCooldown = GetF(msg, "p0si");
    gs.players[0].invulnTime = GetF(msg, "p0it");
    gs.players[0].alive = GetB(msg, "p0a");
    gs.players[1].id = 1;
    gs.players[1].x = GetF(msg, "p1x");
    gs.players[1].z = GetF(msg, "p1z");
    gs.players[1].rotation = GetF(msg, "p1r");
    gs.players[1].vx = GetF(msg, "p1vx");
    gs.players[1].vz = GetF(msg, "p1vz");
    gs.players[1].lives = GetI(msg, "p1l");
    gs.players[1].score = GetI(msg, "p1s");
    gs.players[1].shootCooldown = GetF(msg, "p1si");
    gs.players[1].invulnTime = GetF(msg, "p1it");
    gs.players[1].alive = GetB(msg, "p1a");
    gs.roundTimer = GetF(msg, "rt");
    gs.roundNumber = GetI(msg, "rn");
    gs.roundWins[0] = GetI(msg, "rw0");
    gs.roundWins[1] = GetI(msg, "rw1");
    gs.roundWinner = GetI(msg, "rwn");
    gs.roundEndTimer = GetF(msg, "ret");
    ast.clear();
    std::string astStr = GetS(msg, "ast");
    const char* p = astStr.c_str();
    while (*p) {
        Asteroid a{};
        int parsed = sscanf(p, "%d,%f,%f,%f,%f,%f,%f,%f,%d;",
            &a.id, &a.x, &a.z, &a.vx, &a.vz,
            &a.rotation, &a.rotationspeed, &a.radius, &a.size);
        if (parsed < 9) break;
        ast.push_back(a);
        p = strchr(p, ';');
        if (!p) break;
        p++;
    }
    prj.clear();
    std::string prjStr = GetS(msg, "prj");
    p = prjStr.c_str();
    while (*p) {
        Projectile proj{};
        int parsed = sscanf(p, "%d,%d,%f,%f,%f,%f;",
            &proj.id, &proj.ownerId, &proj.x, &proj.z, &proj.vx, &proj.vz);
        if (parsed < 6) break;
        prj.push_back(proj);
        p = strchr(p, ';');
        if (!p) break;
        p++;
    }
    return true;
}

void NetSendNickname(const char* name) {
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"type\":\"nickname\",\"name\":\"%s\"}", name);
    if (!SendLine(buf)) g_conn = false;
}

bool NetGetNickname(char* name, int maxLen) {
    std::lock_guard<std::mutex> lk(g_mu);
    if (!g_hasNick) return false;
    g_hasNick = false;
    std::string n = GetS(g_nickMsg, "name");
    strncpy(name, n.c_str(), maxLen - 1);
    name[maxLen - 1] = '\0';
    return true;
}

void NetSendReady() {
    if (!SendLine("{\"type\":\"ready\"}")) g_conn = false;
}

bool NetGetReady() {
    return g_remoteReady;
}

void NetSendPing() {
    g_pingTime = SDL_GetTicks();
    if (!SendLine("{\"type\":\"ping\"}")) g_conn = false;
}

float NetGetPing() {
    return g_ping;
}

void NetSendRematch() {
    if (!SendLine("{\"type\":\"rematch\"}")) g_conn = false;
}

bool NetGetRematch() {
    return g_remoteRematch;
}