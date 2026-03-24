#pragma once
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <vector>
#include "Entities.h"
bool NetInit();
bool NetConnect(const char* ip, int port = 5050);
void NetDisconnect();
bool NetIsConnected();
int  NetGetPlayerId();
bool NetIsStarted();
void NetSendState(const GameState& gs,
    const std::vector<Asteroid>& ast,
    const std::vector<Projectile>& prj);
void NetSendInput(bool thrust, bool left, bool right, bool shoot);
bool NetGetInput(bool& thrust, bool& left, bool& right, bool& shoot);
bool NetGetState(GameState& gs,
    std::vector<Asteroid>& ast,
    std::vector<Projectile>& prj);
void NetSendNickname(const char* name);
bool NetGetNickname(char* name, int maxLen);
void NetSendReady();
bool NetGetReady();
void NetSendPing();
float NetGetPing();
void NetSendRematch();
bool NetGetRematch();
