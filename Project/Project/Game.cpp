#define _CRT_SECURE_NO_WARNINGS
#include "Game.h"
#include "network.h"
#include <ws2tcpip.h>

Game::Game() {
    mCounter = 0;
    mW = 1280; mH = 720.f;
    mMouseX = mMouseY = 0;
    mMouseButton = mMouseState = 0;
    mQuadratic = gluNewQuadric();
    gluQuadricNormals(mQuadratic, GLU_SMOOTH);
    memset(keys, false, sizeof(keys));
    menuSelection = 0;
    strcpy(nickname, "");
    strcpy(player1Name, "P1");
    strcpy(player2Name, "P2");
    enteringNickname = false;
    nicknameLen = 0;
    lobbyChoice = -1;
    strcpy(lobbyCode, "");
    strcpy(lobbyCodeInput, "");
    lobbyCodeLen = 0;
    isHost = false;
    joinActive = false;
    lobbyReady = false;
    opponentConnected = false;
    localReady = false;
    remoteReady = false;
    localRematch = false;
    remoteRematch = false;
    opponentDisconnected = false;
    nicknameSent = false;
    strcpy(opponentName, "");
    netSendTimer = 0.0f;
    pingTimer = 0.0f;
    lastMouseX = lastMouseY = -1;
    currentState = STATE_MAIN_MENU;
    roundNumber = 0;
    roundWins[0] = roundWins[1] = 0;
    roundWinner = -1;
    roundEndTimer = 0.0f;
    asteroidSpawnTimer = 0.0f;
    roundTimer = ROUND_TIME;
    nextProjectileId = 0;
    nextAsteroidId = 0;
    players[0].alive = false;
    players[1].alive = false;

    isMultiplayer = false;
    remThrust = remLeft = remRight = remShoot = false;
    mMouseMotionX = mMouseMotionY = 0;

    TTF_Init();
    fontLarge = TTF_OpenFont("fonts/ShareTechMono-Regular.ttf", 96);
    fontMedium = TTF_OpenFont("fonts/ShareTechMono-Regular.ttf", 56);
    fontSmall = TTF_OpenFont("fonts/ShareTechMono-Regular.ttf", 36);
}

Game::~Game() {
    gluDeleteQuadric(mQuadratic);

    if (fontLarge)  TTF_CloseFont(fontLarge);
    if (fontMedium) TTF_CloseFont(fontMedium);
    if (fontSmall)  TTF_CloseFont(fontSmall);
    TTF_Quit();
}

void Game::ResetRound() {
    players[0].id = 0;
    players[0].x = -PLAYER_SPAWN_X;  players[0].z = 0.0f;
    players[0].rotation = 0.0f;
    players[0].vx = 0.0f;  players[0].vz = 0.0f;
    players[0].lives = PLAYER_START_LIVES;
    players[0].score = 0;
    players[0].shootCooldown = 0.0f;
    players[0].invulnTime = 0.0f;
    players[0].alive = true;

    players[1].id = 1;
    players[1].x = PLAYER_SPAWN_X;  players[1].z = 0.0f;
    players[1].rotation = 180.0f;
    players[1].vx = 0.0f;  players[1].vz = 0.0f;
    players[1].lives = PLAYER_START_LIVES;
    players[1].score = 0;
    players[1].shootCooldown = 0.0f;
    players[1].invulnTime = 0.0f;
    players[1].alive = true;

    asteroids.clear();
    projectiles.clear();
    roundTimer = ROUND_TIME;
    roundWinner = -1;
    asteroidSpawnTimer = 0.0f;
    nextProjectileId = 0;
    nextAsteroidId = 0;
    SpawnInitialAsteroids();
    roundNumber++;
}

void Game::ResetMatch() {
    roundWins[0] = roundWins[1] = 0;
    roundNumber = 0;
    ResetRound();

}

static void Begin2D(float w, float h) {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();
}

static void End2D() {
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

static void DrawRect2D(float x, float y, float w, float h,
    unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
    glEnable(GL_BLEND);
    glColor4ub(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);   glVertex2f(x + w, y);
    glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
    glDisable(GL_BLEND);
}

void Game::DrawText(const char* text, float x, float y, TTF_Font* font,
    unsigned char r, unsigned char g, unsigned char b) {
    if (!font) return;
    SDL_Color color = { r, g, b, 255 };
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) return;

    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    SDL_Surface* converted = SDL_CreateRGBSurface(0, surface->w, surface->h, 32,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
    SDL_BlitSurface(surface, NULL, converted, NULL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, converted->w, converted->h,
        0, GL_RGBA, GL_UNSIGNED_BYTE, converted->pixels);
    SDL_FreeSurface(converted);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(255, 255, 255, 255);

    float w = (float)surface->w;
    float h = (float)surface->h;
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(x, y);
    glTexCoord2f(1, 0); glVertex2f(x + w, y);
    glTexCoord2f(1, 1); glVertex2f(x + w, y + h);
    glTexCoord2f(0, 1); glVertex2f(x, y + h);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDeleteTextures(1, &texId);
    SDL_FreeSurface(surface);
}

void Game::SetState(GameStateEnum s) {
    currentState = s;


    if (s == STATE_MAIN_MENU) {

        if (isMultiplayer) {
            NetDisconnect();
            isMultiplayer = false;
        }

        strcpy(lobbyCode, "");
        strcpy(lobbyCodeInput, "");
        lobbyCodeLen = 0;
        isHost = false;
        lobbyReady = false;
        joinActive = false;
        lobbyChoice = -1;
        opponentConnected = false;
        strcpy(opponentName, "");
        localReady = false;
        remoteReady = false;
        nicknameSent = false;
        localRematch = false;
        remoteRematch = false;
        strcpy(nickname, "");
        nicknameLen = 0;
    }

    if (s == STATE_ROUND_END || s == STATE_MATCH_END)
        roundEndTimer = ROUND_END_PAUSE;
}

void Game::InitGFX() {
    glClearColor(.0f, .0f, .0f, 1.f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LIGHTING);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    GLfloat ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat position[] = { 2.0f, 2.0f, 2.0f, 0.0f };
    GLfloat front_mat_shininess[] = { 60.0f };
    GLfloat front_mat_specular[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat front_mat_diffuse[] = { 0.5f, 0.5f, 0.28f, 1.0f };
    GLfloat back_mat_shininess[] = { 60.0f };
    GLfloat back_mat_specular[] = { 0.5f, 0.5f, 0.2f, 1.0f };
    GLfloat back_mat_diffuse[] = { 1.0f, 0.9f, 0.2f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glMaterialfv(GL_FRONT, GL_SHININESS, front_mat_shininess);
    glMaterialfv(GL_FRONT, GL_SPECULAR, front_mat_specular);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, front_mat_diffuse);
    glMaterialfv(GL_BACK, GL_SHININESS, back_mat_shininess);
    glMaterialfv(GL_BACK, GL_SPECULAR, back_mat_specular);
    glMaterialfv(GL_BACK, GL_DIFFUSE, back_mat_diffuse);

    const GLfloat lmodel_twoside[] = { (GLfloat)GL_TRUE };
    glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
    glEnable(GL_LIGHT0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_MULTISAMPLE_ARB);
}

void Game::ChangeSize(int w, int h) {
    mW = (float)w; mH = (float)h;
    glViewport(0, 0, (int)mW, (int)mH);
    double fov = 60, nearX = .1, farX = 2500.;
    double ratio = double(mW) / double(mH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, ratio, nearX, farX);
    glMatrixMode(GL_MODELVIEW);
}

void Game::DrawMainMenu() {
    glLoadIdentity();
    Begin2D(mW, mH);
    DrawRect2D(0, 0, mW, mH, 5, 5, 20);

    // Title
    const char* title = "ASTEROID 3D";
    int tw, th;
    TTF_SizeText(fontLarge, title, &tw, &th);
    DrawText(title, (mW - tw) * 0.5f, mH * 0.1f, fontLarge, 255, 255, 255);

    // Menu items
    const char* items[] = { "PLAY", "RULES", "CREDITS", "QUIT" };
    
    for (int i = 0; i < 4; i++) {
        float iy = mH * 0.35f + i * mH * 0.1f;
        float bx = mW * 0.3f;
        float bw = mW * 0.4f;
        float bh = mH * 0.075f;

        bool mouseMoved = (mMouseX != lastMouseX || mMouseY != lastMouseY);
        bool hover = (mMouseX >= bx && mMouseX <= bx + bw &&
            mMouseY >= iy && mMouseY <= iy + bh);
        if (hover && mouseMoved) menuSelection = i;
        bool sel = (menuSelection == i);

        DrawRect2D(bx, iy, bw, bh,
            sel ? 80 : 30, sel ? 120 : 60, sel ? 200 : 100);
        int iw, ih;
        TTF_SizeText(fontMedium, items[i], &iw, &ih);
        DrawText(items[i], (mW - iw) * 0.5f, iy + (bh - ih) * 0.5f,
            fontMedium, sel ? 255 : 150, sel ? 255 : 150, sel ? 255 : 150);
    }

    // Controls hint
    const char* hint = "Mouse or Arrow keys to navigate  |  Click or ENTER to select";
    int hw, hh;
    TTF_SizeText(fontSmall, hint, &hw, &hh);
    DrawText(hint, (mW - hw) * 0.5f, mH * 0.85f, fontSmall, 100, 100, 100);

    lastMouseX = mMouseX;
    lastMouseY = mMouseY;

    End2D();
    SDL_SetWindowTitle(gScreen, "ASTEROID 3D");
}

void Game::DrawNickname() {

    glLoadIdentity();
    Begin2D(mW, mH);
    DrawRect2D(0, 0, mW, mH, 5, 5, 20);

    // Title
    const char* title = "ENTER NICKNAME";
    int tw, th;
    TTF_SizeText(fontLarge, title, &tw, &th);
    DrawText(title, (mW - tw) * 0.5f, mH * 0.15f, fontLarge, 255, 255, 255);

    // Input box
    float bx = mW * 0.25f;
    float by = mH * 0.4f;
    float bw = mW * 0.5f;
    float bh = mH * 0.1f;
    DrawRect2D(bx, by, bw, bh, 20, 20, 50);
    DrawRect2D(bx + 2, by + 2, bw - 4, bh - 4, 10, 10, 30);

    // Nickname text with cursor
    char display[64];
    sprintf(display, "%s_", nickname);
    int dw, dh;
    TTF_SizeText(fontLarge, display, &dw, &dh);
    DrawText(display, (mW - dw) * 0.5f, by + (bh - dh) * 0.5f, fontLarge, 0, 255, 255);

    // Hint
    const char* hint = "Type your name (max 15 chars) then press ENTER";
    int hw, hh;
    TTF_SizeText(fontSmall, hint, &hw, &hh);
    DrawText(hint, (mW - hw) * 0.5f, mH * 0.6f, fontSmall, 100, 100, 100);

    End2D();
    SDL_SetWindowTitle(gScreen, "ASTEROID 3D - NICKNAME");
}
void Game::DrawRules() {
    glLoadIdentity();
    Begin2D(mW, mH);
    DrawRect2D(0, 0, mW, mH, 5, 5, 20);
    DrawRect2D(mW * 0.1f, mH * 0.08f, mW * 0.8f, mH * 0.84f, 15, 15, 40);

    DrawText("RULES", (mW - 120) * 0.5f, mH * 0.12f, fontLarge, 255, 255, 255);

    const char* lines[] = {
        "Best of 3 rounds - 30 seconds each",
        "Each player has 3 lives per round",
        "Shoot asteroids for points",
        "Shoot your enemy to take their lives",
        "Kill enemy or have higher score to win round",
        "First to 2 round wins takes the match",
        "",
        "W/A/D to move  SPACE to shoot",
        
    };
    for (int i = 0; i < 8; i++) {
        if (lines[i][0] == '\0') continue;
        DrawText(lines[i], mW * 0.15f, mH * 0.25f + i * mH * 0.065f,
            fontSmall, 180, 180, 180);
    }

    DrawText("ESC to go back", (mW - 160) * 0.5f, mH * 0.88f, fontSmall, 100, 100, 100);
    End2D();
    SDL_SetWindowTitle(gScreen, "ASTEROID 3D - RULES");
}

void Game::DrawCredits() {
    glLoadIdentity();
    Begin2D(mW, mH);
    DrawRect2D(0, 0, mW, mH, 5, 5, 20);
    DrawRect2D(mW * 0.1f, mH * 0.08f, mW * 0.8f, mH * 0.84f, 15, 15, 40);

    DrawText("CREDITS", (mW - 160) * 0.5f, mH * 0.12f, fontLarge, 255, 255, 255);

    DrawText("Okan Ozcan", mW * 0.12f, mH * 0.30f, fontMedium, 0, 255, 255);
    DrawText("Marius-Raul Filipiuc", mW * 0.12f, mH * 0.35f, fontMedium, 0, 255, 255);

    DrawText("Uppsala University - Campus Gotland", mW * 0.12f, mH * 0.42f, fontMedium, 180, 180, 180);
    DrawText("Linear Algebra, Trigonometry and Geometry 2026 Spring", mW * 0.12f, mH * 0.49f, fontMedium, 180, 180, 180);

    DrawText("ESC to go back", (mW - 160) * 0.5f, mH * 0.88f, fontSmall, 100, 100, 100);

    End2D();
    SDL_SetWindowTitle(gScreen, "ASTEROID 3D - CREDITS");
}

void Game::DrawLobby() {
    glLoadIdentity();
    Begin2D(mW, mH);
    DrawRect2D(0, 0, mW, mH, 5, 5, 20);

    char welcome[64];
    sprintf(welcome, "Welcome, %s", nickname);
    int ww, wh;
    TTF_SizeText(fontMedium, welcome, &ww, &wh);
    DrawText(welcome, (mW - ww) * 0.5f, mH * 0.05f, fontMedium, 0, 255, 255);

    float bx = mW * 0.25f;
    float bw = mW * 0.5f;
    float bh = mH * 0.08f;
    bool mouseMoved = (mMouseX != lastMouseX || mMouseY != lastMouseY);
    float hostY = mH * 0.15f;
    float joinY = mH * 0.25f;
    bool hoverHost = (mMouseX >= bx && mMouseX <= bx + bw &&
        mMouseY >= hostY && mMouseY <= hostY + bh);
    bool hoverJoin = (mMouseX >= bx && mMouseX <= bx + bw &&
        mMouseY >= joinY && mMouseY <= joinY + bh);
    bool selHost = hoverHost || (!hoverJoin && lobbyChoice == 0);
    bool selJoin = hoverJoin || (!hoverHost && lobbyChoice == 1);

    DrawRect2D(bx, hostY, bw, bh, selHost ? 80 : 30, selHost ? 120 : 60, selHost ? 200 : 100);
    const char* hostTxt = "HOST GAME";
    int htw, hth;
    TTF_SizeText(fontMedium, hostTxt, &htw, &hth);
    DrawText(hostTxt, (mW - htw) * 0.5f, hostY + (bh - hth) * 0.5f,
        fontMedium, selHost ? 255 : 150, selHost ? 255 : 150, selHost ? 255 : 150);

    DrawRect2D(bx, joinY, bw, bh, selJoin ? 80 : 30, selJoin ? 120 : 60, selJoin ? 200 : 100);
    const char* joinTxt = "JOIN GAME";
    int jtw, jth;
    TTF_SizeText(fontMedium, joinTxt, &jtw, &jth);
    DrawText(joinTxt, (mW - jtw) * 0.5f, joinY + (bh - jth) * 0.5f,
        fontMedium, selJoin ? 255 : 150, selJoin ? 255 : 150, selJoin ? 255 : 150);

    if (isHost && lobbyCode[0] != '\0') {
        const char* ipLabel = "YOUR IP:";
        int ipw, iph;
        TTF_SizeText(fontSmall, ipLabel, &ipw, &iph);
        DrawText(ipLabel, (mW - ipw) * 0.5f, mH * 0.38f, fontSmall, 150, 150, 150);

        int cw, ch;
        TTF_SizeText(fontLarge, lobbyCode, &cw, &ch);
        DrawText(lobbyCode, (mW - cw) * 0.5f, mH * 0.43f, fontLarge, 255, 255, 0);

        if (opponentConnected) {
            char connMsg[64];
            sprintf(connMsg, "%s connected!", player2Name);
            int cmw, cmh;
            TTF_SizeText(fontMedium, connMsg, &cmw, &cmh);
            DrawText(connMsg, (mW - cmw) * 0.5f, mH * 0.56f, fontMedium, 0, 255, 0);

            char readyStatus[128];
            sprintf(readyStatus, "You: %s  |  %s: %s",
                localReady ? "READY" : "NOT READY",
                player2Name,
                remoteReady ? "READY" : "NOT READY");
            int rsw, rsh;
            TTF_SizeText(fontSmall, readyStatus, &rsw, &rsh);
            DrawText(readyStatus, (mW - rsw) * 0.5f, mH * 0.64f, fontSmall, 180, 180, 180);

            if (!localReady) {
                float rbx = mW * 0.3f;
                float rby = mH * 0.72f;
                float rbw = mW * 0.4f;
                float rbh = mH * 0.07f;
                bool hoverReady = (mMouseX >= rbx && mMouseX <= rbx + rbw &&
                    mMouseY >= rby && mMouseY <= rby + rbh);
                DrawRect2D(rbx, rby, rbw, rbh,
                    0, hoverReady ? 180 : 120, 0);
                const char* rdyTxt = "READY";
                int rtw, rth;
                TTF_SizeText(fontMedium, rdyTxt, &rtw, &rth);
                DrawText(rdyTxt, (mW - rtw) * 0.5f, rby + (rbh - rth) * 0.5f,
                    fontMedium, 255, 255, 255);
            }

            if (localReady && remoteReady) {
                float sbx = mW * 0.3f;
                float sby = mH * 0.82f;
                float sbw = mW * 0.4f;
                float sbh = mH * 0.08f;
                bool hoverStart = (mMouseX >= sbx && mMouseX <= sbx + sbw &&
                    mMouseY >= sby && mMouseY <= sby + sbh);
                DrawRect2D(sbx, sby, sbw, sbh,
                    0, hoverStart ? 255 : 200, 0);
                const char* startTxt = "START MATCH";
                int stw, sth;
                TTF_SizeText(fontMedium, startTxt, &stw, &sth);
                DrawText(startTxt, (mW - stw) * 0.5f, sby + (sbh - sth) * 0.5f,
                    fontMedium, 255, 255, 255);
            }
        }
        else {
            const char* wait2 = "Waiting for opponent...";
            int w2w, w2h;
            TTF_SizeText(fontSmall, wait2, &w2w, &w2h);
            DrawText(wait2, (mW - w2w) * 0.5f, mH * 0.56f, fontSmall, 100, 100, 100);
        }
    }

    if (joinActive && !isHost) {
        if (!opponentConnected) {
            const char* enterCode = "ENTER HOST IP:";
            int etw, eth;
            TTF_SizeText(fontMedium, enterCode, &etw, &eth);
            DrawText(enterCode, (mW - etw) * 0.5f, mH * 0.38f, fontMedium, 200, 200, 200);

            float cbx = mW * 0.25f;
            float cby = mH * 0.46f;
            float cbw = mW * 0.5f;
            float cbh = mH * 0.1f;
            DrawRect2D(cbx, cby, cbw, cbh, 20, 20, 50);
            DrawRect2D(cbx + 2, cby + 2, cbw - 4, cbh - 4, 10, 10, 30);

            char codeDisp[32];
            sprintf(codeDisp, "%s_", lobbyCodeInput);
            int cdw, cdh;
            TTF_SizeText(fontLarge, codeDisp, &cdw, &cdh);
            DrawText(codeDisp, (mW - cdw) * 0.5f, cby + (cbh - cdh) * 0.5f, fontLarge, 255, 255, 0);

            const char* codeHint = "Type host IP, then press ENTER";
            int chw, chh;
            TTF_SizeText(fontSmall, codeHint, &chw, &chh);
            DrawText(codeHint, (mW - chw) * 0.5f, mH * 0.60f, fontSmall, 100, 100, 100);
        }
        else {
            char connMsg[64];
            sprintf(connMsg, "Connected to %s!", player1Name);
            int cmw, cmh;
            TTF_SizeText(fontMedium, connMsg, &cmw, &cmh);
            DrawText(connMsg, (mW - cmw) * 0.5f, mH * 0.42f, fontMedium, 0, 255, 0);

            char readyStatus[128];
            sprintf(readyStatus, "%s: %s  |  You: %s",
                player1Name,
                remoteReady ? "READY" : "NOT READY",
                localReady ? "READY" : "NOT READY");
            int rsw, rsh;
            TTF_SizeText(fontSmall, readyStatus, &rsw, &rsh);
            DrawText(readyStatus, (mW - rsw) * 0.5f, mH * 0.52f, fontSmall, 180, 180, 180);

            if (!localReady) {
                float rbx = mW * 0.3f;
                float rby = mH * 0.62f;
                float rbw = mW * 0.4f;
                float rbh = mH * 0.07f;
                bool hoverReady = (mMouseX >= rbx && mMouseX <= rbx + rbw &&
                    mMouseY >= rby && mMouseY <= rby + rbh);
                DrawRect2D(rbx, rby, rbw, rbh,
                    0, hoverReady ? 180 : 120, 0);
                const char* rdyTxt = "READY";
                int rtw, rth;
                TTF_SizeText(fontMedium, rdyTxt, &rtw, &rth);
                DrawText(rdyTxt, (mW - rtw) * 0.5f, rby + (rbh - rth) * 0.5f,
                    fontMedium, 255, 255, 255);
            }

            if (localReady) {
                const char* waitMsg = "Waiting for host to start...";
                int wmw, wmh;
                TTF_SizeText(fontSmall, waitMsg, &wmw, &wmh);
                DrawText(waitMsg, (mW - wmw) * 0.5f, mH * 0.75f, fontSmall, 100, 100, 100);
            }
        }
    }

    const char* escHint = "ESC to go back";
    int ew, eh;
    TTF_SizeText(fontSmall, escHint, &ew, &eh);
    DrawText(escHint, (mW - ew) * 0.5f, mH * 0.93f, fontSmall, 100, 100, 100);

    lastMouseX = mMouseX;
    lastMouseY = mMouseY;

    End2D();
    SDL_SetWindowTitle(gScreen, "ASTEROID 3D - LOBBY");
}

void Game::DrawRoundEnd() {
    DrawGame();
    Begin2D(mW, mH);
    DrawRect2D(mW * 0.2f, mH * 0.25f, mW * 0.6f, mH * 0.4f, 10, 10, 50, 200);

    char msg[128];
    if (roundWinner == -1)
        sprintf(msg, "DRAW!");
    else {
        const char* winnerName = (roundWinner == 0) ? player1Name : player2Name;
        sprintf(msg, "%s WINS THE ROUND!", winnerName);
    }
    int tw, th;
    TTF_SizeText(fontLarge, msg, &tw, &th);
    DrawText(msg, (mW - tw) * 0.5f, mH * 0.32f, fontLarge,
        roundWinner == 0 ? 0 : 255, roundWinner == 0 ? 255 : 50, roundWinner == 0 ? 255 : 50);

    char score[64];
    sprintf(score, "Wins: %d - %d", roundWins[0], roundWins[1]);
    TTF_SizeText(fontMedium, score, &tw, &th);
    DrawText(score, (mW - tw) * 0.5f, mH * 0.45f, fontMedium, 200, 200, 200);

    char next[32];
    sprintf(next, "Next round in %.0fs...", roundEndTimer);
    TTF_SizeText(fontSmall, next, &tw, &th);
    DrawText(next, (mW - tw) * 0.5f, mH * 0.55f, fontSmall, 150, 150, 150);

    End2D();
    SDL_SetWindowTitle(gScreen, "ASTEROID 3D");
}

void Game::DrawMatchEnd() {
    DrawGame();
    Begin2D(mW, mH);
    DrawRect2D(mW * 0.15f, mH * 0.2f, mW * 0.7f, mH * 0.5f, 10, 10, 50, 220);

    // Disconnect message
    if (opponentDisconnected) {
        char dcMsg[64];
        if (NetGetPlayerId() == 0)
            sprintf(dcMsg, "%s left the game!", player2Name);
        else
            sprintf(dcMsg, "%s left the game!", player1Name);
        int dw, dh;
        TTF_SizeText(fontMedium, dcMsg, &dw, &dh);
        DrawText(dcMsg, (mW - dw) * 0.5f, mH * 0.22f, fontMedium, 255, 100, 100);
    }

    int winIdx = (roundWins[0] >= ROUNDS_TO_WIN) ? 0 : 1;
    const char* winnerName = (winIdx == 0) ? player1Name : player2Name;
    char msg[128];
    sprintf(msg, "%s WINS!", winnerName);
    int tw, th;
    TTF_SizeText(fontLarge, msg, &tw, &th);
    unsigned char cr = (winIdx == 0) ? 0 : 255;
    unsigned char cg = (winIdx == 0) ? 255 : 50;
    unsigned char cb = (winIdx == 0) ? 255 : 50;
    float msgY = opponentDisconnected ? mH * 0.30f : mH * 0.22f;
    DrawText(msg, (mW - tw) * 0.5f, msgY, fontLarge, cr, cg, cb);

    char score[64];
    sprintf(score, "Final: %d - %d", roundWins[0], roundWins[1]);
    TTF_SizeText(fontMedium, score, &tw, &th);
    float scoreY = opponentDisconnected ? mH * 0.40f : mH * 0.35f;
    DrawText(score, (mW - tw) * 0.5f, scoreY, fontMedium, 200, 200, 200);

    if (isMultiplayer && opponentDisconnected) {
        // Only show return to lobby
        float rbx = mW * 0.3f;
        float rby = mH * 0.55f;
        float rbw = mW * 0.4f;
        float rbh = mH * 0.08f;
        bool hoverRet = (mMouseX >= rbx && mMouseX <= rbx + rbw &&
            mMouseY >= rby && mMouseY <= rby + rbh);
        DrawRect2D(rbx, rby, rbw, rbh, hoverRet ? 80 : 50, hoverRet ? 120 : 80, hoverRet ? 200 : 150);
        const char* retTxt = "RETURN TO LOBBY";
        int rtw2, rth2;
        TTF_SizeText(fontMedium, retTxt, &rtw2, &rth2);
        DrawText(retTxt, (mW - rtw2) * 0.5f, rby + (rbh - rth2) * 0.5f,
            fontMedium, 255, 255, 255);
    }
    else if (isMultiplayer) {
        // Rematch status
        char remStatus[128];
        sprintf(remStatus, "You: %s  |  Opponent: %s",
            localRematch ? "REMATCH" : "---",
            remoteRematch ? "REMATCH" : "---");
        int rsw, rsh;
        TTF_SizeText(fontSmall, remStatus, &rsw, &rsh);
        DrawText(remStatus, (mW - rsw) * 0.5f, mH * 0.45f, fontSmall, 150, 150, 150);

        // REMATCH button
        if (!localRematch) {
            float rbx = mW * 0.2f;
            float rby = mH * 0.53f;
            float rbw = mW * 0.28f;
            float rbh = mH * 0.08f;
            bool hoverRem = (mMouseX >= rbx && mMouseX <= rbx + rbw &&
                mMouseY >= rby && mMouseY <= rby + rbh);
            DrawRect2D(rbx, rby, rbw, rbh, 0, hoverRem ? 200 : 150, 0);
            const char* remTxt = "REMATCH";
            int rtw, rth;
            TTF_SizeText(fontMedium, remTxt, &rtw, &rth);
            DrawText(remTxt, rbx + (rbw - rtw) * 0.5f, rby + (rbh - rth) * 0.5f,
                fontMedium, 255, 255, 255);
        }

        // QUIT button
        float qbx = mW * 0.52f;
        float qby = mH * 0.53f;
        float qbw = mW * 0.28f;
        float qbh = mH * 0.08f;
        bool hoverQuit = (mMouseX >= qbx && mMouseX <= qbx + qbw &&
            mMouseY >= qby && mMouseY <= qby + qbh);
        DrawRect2D(qbx, qby, qbw, qbh, hoverQuit ? 200 : 150, 0, 0);
        const char* quitTxt = "QUIT";
        int qtw, qth;
        TTF_SizeText(fontMedium, quitTxt, &qtw, &qth);
        DrawText(quitTxt, qbx + (qbw - qtw) * 0.5f, qby + (qbh - qth) * 0.5f,
            fontMedium, 255, 255, 255);
    }
    else {
        char back[64];
        sprintf(back, "Returning to menu in %.0fs...", roundEndTimer);
        TTF_SizeText(fontSmall, back, &tw, &th);
        DrawText(back, (mW - tw) * 0.5f, mH * 0.55f, fontSmall, 150, 150, 150);
    }

    End2D();
    SDL_SetWindowTitle(gScreen, "ASTEROID 3D");
}

void Game::DrawGame() {
    glLoadIdentity();
    gluLookAt(0, 65, 0.1, 0, 0, 0, 0, 1, 0);

    // Grid
    glLineWidth(1);
    glColor3ub(20, 20, 20); // for color 
    int n = (int)WORLD_SIZE;
    for (int i = -n; i <= n; i += 10) {
        glBegin(GL_LINES);
        glVertex3f((float)-n, 0, (float)i);
        glVertex3f((float)n, 0, (float)i);
        glVertex3f((float)i, 0, (float)-n);
        glVertex3f((float)i, 0, (float)n);
        glEnd();
    }



    // Player 0
    bool vis0 = true;
    if (players[0].invulnTime > 0.0f)
        vis0 = ((int)(players[0].invulnTime / PLAYER_FLASH_RATE) % 2 == 0);
    if (players[0].alive && vis0) {
        glColor3ub(0, 255, 255);
        glPushMatrix();
        glTranslatef(players[0].x, 0.3f, players[0].z);
        glRotatef(players[0].rotation, 0, 1, 0);
        glScalef(3.0f, 3.0f, 3.0f);
        glBegin(GL_TRIANGLES);
        glVertex3f(1.0f, 0.0f, 0.0f);
        glVertex3f(-0.5f, 0.0f, 0.6f);
        glVertex3f(-0.5f, 0.0f, -0.6f);
        glEnd();
        glBegin(GL_TRIANGLES);
        glVertex3f(1.0f, 0.2f, 0.0f);
        glVertex3f(-0.5f, 0.2f, -0.6f);
        glVertex3f(-0.5f, 0.2f, 0.6f);
        glEnd();
        glColor3ub(0, 200, 200);
        glBegin(GL_QUADS);
        glVertex3f(1.0f, 0.0f, 0.0f); glVertex3f(1.0f, 0.2f, 0.0f);
        glVertex3f(-0.5f, 0.2f, 0.6f); glVertex3f(-0.5f, 0.0f, 0.6f);
        glVertex3f(1.0f, 0.0f, 0.0f); glVertex3f(1.0f, 0.2f, 0.0f);
        glVertex3f(-0.5f, 0.2f, -0.6f); glVertex3f(-0.5f, 0.0f, -0.6f);
        glVertex3f(-0.5f, 0.0f, 0.6f); glVertex3f(-0.5f, 0.2f, 0.6f);
        glVertex3f(-0.5f, 0.2f, -0.6f); glVertex3f(-0.5f, 0.0f, -0.6f);
        glEnd();
        glPopMatrix();
    }

    // Player 1
    bool vis1 = true;
    if (players[1].invulnTime > 0.0f)
        vis1 = ((int)(players[1].invulnTime / PLAYER_FLASH_RATE) % 2 == 0);
    if (players[1].alive && vis1) {
        glColor3ub(255, 50, 50);
        glPushMatrix();
        glTranslatef(players[1].x, 0.3f, players[1].z);
        glRotatef(players[1].rotation, 0, 1, 0);
        glScalef(3.0f, 3.0f, 3.0f);
        glBegin(GL_TRIANGLES);
        glVertex3f(1.0f, 0.0f, 0.0f);
        glVertex3f(-0.5f, 0.0f, 0.6f);
        glVertex3f(-0.5f, 0.0f, -0.6f);
        glEnd();
        glBegin(GL_TRIANGLES);
        glVertex3f(1.0f, 0.2f, 0.0f);
        glVertex3f(-0.5f, 0.2f, -0.6f);
        glVertex3f(-0.5f, 0.2f, 0.6f);
        glEnd();
        glColor3ub(200, 40, 40);
        glBegin(GL_QUADS);
        glVertex3f(1.0f, 0.0f, 0.0f); glVertex3f(1.0f, 0.2f, 0.0f);
        glVertex3f(-0.5f, 0.2f, 0.6f); glVertex3f(-0.5f, 0.0f, 0.6f);
        glVertex3f(1.0f, 0.0f, 0.0f); glVertex3f(1.0f, 0.2f, 0.0f);
        glVertex3f(-0.5f, 0.2f, -0.6f); glVertex3f(-0.5f, 0.0f, -0.6f);
        glVertex3f(-0.5f, 0.0f, 0.6f); glVertex3f(-0.5f, 0.2f, 0.6f);
        glVertex3f(-0.5f, 0.2f, -0.6f); glVertex3f(-0.5f, 0.0f, -0.6f);
        glEnd();
        glPopMatrix();
    }

    // Projectiles
    glColor3ub(255, 255, 0);
    for (int i = 0; i < (int)projectiles.size(); i++) {
        glPushMatrix();
        glTranslatef(projectiles[i].x, 0.3f, projectiles[i].z);
        gluSphere(mQuadratic, 0.3, 8, 8);
        glPopMatrix();
    }

    // Asteroids
    for (int i = 0; i < (int)asteroids.size(); i++) {

        Asteroid& a = asteroids[i];

        if (a.size == 0)      glColor3ub(100, 100, 100);

        else if (a.size == 1) glColor3ub(80, 80, 80);

        else                  glColor3ub(60, 60, 60);

        glPushMatrix();
        glTranslatef(a.x, 0.5f, a.z);
        glRotatef(a.rotation, 0.3f, 1.0f, 0.2f);
        float s = a.radius;
        glScalef(s, s * 0.5f, s);
        glBegin(GL_QUADS);
        glVertex3f(-1, -1, -1); glVertex3f(1, -1, -1); glVertex3f(1, -1, 1); glVertex3f(-1, -1, 1);
        glVertex3f(-1, 1, -1); glVertex3f(-1, 1, 1); glVertex3f(1, 1, 1); glVertex3f(1, 1, -1);
        glVertex3f(-1, -1, 1); glVertex3f(1, -1, 1); glVertex3f(1, 1, 1); glVertex3f(-1, 1, 1);
        glVertex3f(-1, -1, -1); glVertex3f(-1, 1, -1); glVertex3f(1, 1, -1); glVertex3f(1, -1, -1);
        glVertex3f(-1, -1, -1); glVertex3f(-1, -1, 1); glVertex3f(-1, 1, 1); glVertex3f(-1, 1, -1);
        glVertex3f(1, -1, -1); glVertex3f(1, 1, -1); glVertex3f(1, 1, 1); glVertex3f(1, -1, 1);
        glEnd();

        // Wireframe outline
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glColor3ub(200, 200, 200);
        glLineWidth(2);
        glBegin(GL_QUADS);
        glVertex3f(-1, -1, -1); glVertex3f(1, -1, -1); glVertex3f(1, -1, 1); glVertex3f(-1, -1, 1);
        glVertex3f(-1, 1, -1); glVertex3f(-1, 1, 1); glVertex3f(1, 1, 1); glVertex3f(1, 1, -1);
        glVertex3f(-1, -1, 1); glVertex3f(1, -1, 1); glVertex3f(1, 1, 1); glVertex3f(-1, 1, 1);
        glVertex3f(-1, -1, -1); glVertex3f(-1, 1, -1); glVertex3f(1, 1, -1); glVertex3f(1, -1, -1);
        glVertex3f(-1, -1, -1); glVertex3f(-1, -1, 1); glVertex3f(-1, 1, 1); glVertex3f(-1, 1, -1);
        glVertex3f(1, -1, -1); glVertex3f(1, 1, -1); glVertex3f(1, 1, 1); glVertex3f(1, -1, 1);
        glEnd();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glLineWidth(1);

        glPopMatrix();
    }

    // HUD
    Begin2D(mW, mH);

    // Center dashed line
    glColor3ub(50, 50, 50);
    glLineWidth(2);
    float dashLen = mH * 0.02f;
    float gapLen = mH * 0.015f;
    float centerX = mW * 0.5f;
    float yy = 0.0f;
    glBegin(GL_LINES);
    while (yy < mH) {
        glVertex2f(centerX, yy);
        glVertex2f(centerX, yy + dashLen);
        yy += dashLen + gapLen;
    }
    glEnd();
    glLineWidth(1);

    float hs = 16;
    float margin = 20;

    // ===== TOP LEFT - Player 1 Name =====
    DrawText(player1Name, margin, margin, fontMedium, 0, 255, 255);

    // ===== TOP RIGHT - Player 2 Name =====
    int nw2, nh2;
    TTF_SizeText(fontMedium, player2Name, &nw2, &nh2);
    DrawText(player2Name, mW - nw2 - margin, margin, fontMedium, 255, 50, 50);

    // ===== LEFT SIDE - Score + Lives =====
    float leftY = mH * 0.4f;

    // Score label
    DrawText("SCORE", margin, leftY, fontSmall, 100, 100, 100);

    // Score number
    char s1[32];
    sprintf(s1, "%d", players[0].score);
    DrawText(s1, margin, leftY + 42, fontLarge, 0, 255, 255);

    // Lives label
    DrawText("LIVES", margin, leftY + 145, fontSmall, 100, 100, 100);

    // P1 hearts
    float hy1 = leftY + 190;
    for (int i = 0; i < PLAYER_START_LIVES; i++) {
        if (i < players[0].lives)
            glColor3ub(0, 255, 255);
        else
            glColor3ub(40, 40, 40);
        float cx2 = margin + i * (hs * 2.5f) + hs;
        float cy2 = hy1 + hs;
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx2, cy2 + hs * 0.9f);
        for (int a = 0; a <= 20; a++) {
            float angle = (float)a / 20.0f * 3.14159f;
            glVertex2f(cx2 - hs * 0.5f - hs * 0.5f * cosf(angle),
                cy2 - hs * 0.5f * sinf(angle));
        }
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx2, cy2 + hs * 0.9f);
        for (int a = 0; a <= 20; a++) {
            float angle = (float)a / 20.0f * 3.14159f;
            glVertex2f(cx2 + hs * 0.5f + hs * 0.5f * cosf(angle),
                cy2 - hs * 0.5f * sinf(angle));
        }
        glEnd();
    }

    // ===== RIGHT SIDE - Score + Lives =====
    // Score label
    int sw, sh;
    TTF_SizeText(fontSmall, "SCORE", &sw, &sh);
    DrawText("SCORE", mW - sw - margin, leftY, fontSmall, 100, 100, 100);

    // Score number
    char s2[32];
    sprintf(s2, "%d", players[1].score);
    int s2w, s2h;
    TTF_SizeText(fontLarge, s2, &s2w, &s2h);
    DrawText(s2, mW - s2w - margin, leftY + 42, fontLarge, 255, 50, 50);

    // Lives label
    int lw, lh;
    TTF_SizeText(fontSmall, "LIVES", &lw, &lh);
    DrawText("LIVES", mW - lw - margin, leftY + 145, fontSmall, 100, 100, 100);

    // P2 hearts
    for (int i = 0; i < PLAYER_START_LIVES; i++) {
        if (i < players[1].lives)
            glColor3ub(255, 50, 50);
        else
            glColor3ub(40, 40, 40);
        float rx = mW - margin - PLAYER_START_LIVES * (hs * 2.5f);
        float cx2 = rx + i * (hs * 2.5f) + hs;
        float cy2 = hy1 + hs;
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx2, cy2 + hs * 0.9f);
        for (int a = 0; a <= 20; a++) {
            float angle = (float)a / 20.0f * 3.14159f;
            glVertex2f(cx2 - hs * 0.5f - hs * 0.5f * cosf(angle),
                cy2 - hs * 0.5f * sinf(angle));
        }
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx2, cy2 + hs * 0.9f);
        for (int a = 0; a <= 20; a++) {
            float angle = (float)a / 20.0f * 3.14159f;
            glVertex2f(cx2 + hs * 0.5f + hs * 0.5f * cosf(angle),
                cy2 - hs * 0.5f * sinf(angle));
        }
        glEnd();
    }

    // ===== CENTER - Timer + Round =====
    char timerStr[32];
    sprintf(timerStr, "%.0f", roundTimer);
    int tw3, th3;
    TTF_SizeText(fontLarge, timerStr, &tw3, &th3);
    DrawText(timerStr, (mW - tw3) * 0.5f, 10, fontLarge, 200, 200, 200);

    char rndStr[64];
    sprintf(rndStr, "Round %d  [%d-%d]", roundNumber, roundWins[0], roundWins[1]);
    int rw, rh;
    TTF_SizeText(fontSmall, rndStr, &rw, &rh);
    DrawText(rndStr, (mW - rw) * 0.5f, 110, fontSmall, 150, 150, 150);

    if (isMultiplayer) {
        char pingStr[32];
        sprintf(pingStr, "%.0fms", NetGetPing());
        int pw, ph;
        TTF_SizeText(fontSmall, pingStr, &pw, &ph);
        DrawText(pingStr, (mW - pw) * 0.5f, 150, fontSmall, 100, 100, 100);
    }

    End2D();
    SDL_SetWindowTitle(gScreen, "ASTEROID 3D");
}

void Game::Draw() {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    switch (currentState) {
    case STATE_MAIN_MENU: DrawMainMenu(); break;
    case STATE_RULES:     DrawRules();    break;
    case STATE_CREDITS:   DrawCredits();  break;
    case STATE_NICKNAME:  DrawNickname(); break;
    case STATE_LOBBY:     DrawLobby();    break;
    case STATE_PLAYING:   DrawGame();     break;
    case STATE_ROUND_END: DrawRoundEnd(); break;
    case STATE_MATCH_END: DrawMatchEnd(); break;
    }
    mCounter++;
}

void Game::NormalKeys(unsigned char key, int state) {
    (void)key; (void)state;
}

void Game::SpecialKeys(int key, int state) {

    (void)key; (void)state;

    if (currentState == STATE_MAIN_MENU) {
        if (key == SDLK_UP)   menuSelection = (menuSelection + 3) % 4;
        if (key == SDLK_DOWN) menuSelection = (menuSelection + 1) % 4;
        if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            switch (menuSelection) {
            case 0: SetState(STATE_NICKNAME); enteringNickname = true; nicknameLen = (int)strlen(nickname); break;
            case 1: SetState(STATE_RULES);   break;
            case 2: SetState(STATE_CREDITS); break;
            case 3: {
                SDL_Event q;
                q.type = SDL_QUIT;
                SDL_PushEvent(&q);
            } break;
            }
        }
    }
    else if (currentState == STATE_RULES || currentState == STATE_CREDITS) {
        if (key == SDLK_ESCAPE) SetState(STATE_MAIN_MENU);
    }
}
void Game::Mouse(int button, int state, int x, int y) {
    mMouseButton = button; mMouseState = state;
    mMouseX = x; mMouseY = y;

    if (currentState == STATE_MAIN_MENU && button == SDL_BUTTON_LEFT && state == SDL_RELEASED) {
        float bx = mW * 0.3f;
        float bw = mW * 0.4f;
        for (int i = 0; i < 4; i++) {
            float iy = mH * 0.35f + i * mH * 0.1f;
            float bh = mH * 0.075f;
            if (x >= bx && x <= bx + bw && y >= iy && y <= iy + bh) {
                switch (i) {
                case 0: SetState(STATE_NICKNAME); enteringNickname = true; nicknameLen = (int)strlen(nickname); break;
                case 1: SetState(STATE_RULES); break;
                case 2: SetState(STATE_CREDITS); break;
                case 3: {
                    SDL_Event q;
                    q.type = SDL_QUIT;
                    SDL_PushEvent(&q);
                } break;
                }
                break;
            }
        }
    }

    if (currentState == STATE_LOBBY && button == SDL_BUTTON_LEFT && state == SDL_RELEASED) {
        float bx = mW * 0.25f;
        float bw = mW * 0.5f;
        float bh = mH * 0.08f;
        float hostY = mH * 0.15f;
        float joinY = mH * 0.25f;

        if (x >= bx && x <= bx + bw && y >= hostY && y <= hostY + bh) {
            lobbyChoice = 0;
            joinActive = false;
            if (lobbyCode[0] == '\0') {
                isHost = true;
                isMultiplayer = true;
                lobbyReady = false;
                char hostname[256];
                char hostIP[20] = "127.0.0.1";
                gethostname(hostname, sizeof(hostname));
                struct addrinfo hints {}, * res;
                hints.ai_family = AF_INET;
                if (getaddrinfo(hostname, NULL, &hints, &res) == 0) {
                    sockaddr_in* addr = (sockaddr_in*)res->ai_addr;
                    inet_ntop(AF_INET, &addr->sin_addr, hostIP, sizeof(hostIP));
                    freeaddrinfo(res);
                }
                strcpy(lobbyCode, hostIP);
                NetConnect("127.0.0.1");
            }
        }

        if (x >= bx && x <= bx + bw && y >= joinY && y <= joinY + bh) {
            lobbyChoice = 1;
            isHost = false;
            strcpy(lobbyCode, "");
            joinActive = true;
        }

        if (isHost && opponentConnected && !localReady) {
            float rbx = mW * 0.3f;
            float rby = mH * 0.72f;
            float rbw = mW * 0.4f;
            float rbh = mH * 0.07f;
            if (x >= rbx && x <= rbx + rbw && y >= rby && y <= rby + rbh) {
                localReady = true;
                NetSendReady();
            }
        }

        if (!isHost && joinActive && opponentConnected && !localReady) {
            float rbx = mW * 0.3f;
            float rby = mH * 0.62f;
            float rbw = mW * 0.4f;
            float rbh = mH * 0.07f;
            if (x >= rbx && x <= rbx + rbw && y >= rby && y <= rby + rbh) {
                localReady = true;
                NetSendReady();
            }
        }

        if (isHost && localReady && remoteReady) {
            float sbx = mW * 0.3f;
            float sby = mH * 0.82f;
            float sbw = mW * 0.4f;
            float sbh = mH * 0.08f;
            if (x >= sbx && x <= sbx + sbw && y >= sby && y <= sby + sbh) {
                ResetMatch();
                SetState(STATE_PLAYING);
            }
        }
    }

    if (currentState == STATE_MATCH_END && isMultiplayer && button == SDL_BUTTON_LEFT && state == SDL_RELEASED) {
        if (opponentDisconnected) {
            float rbx = mW * 0.3f;
            float rby = mH * 0.55f;
            float rbw = mW * 0.4f;
            float rbh = mH * 0.08f;
            if (x >= rbx && x <= rbx + rbw && y >= rby && y <= rby + rbh) {
                SetState(STATE_MAIN_MENU);
            }
            return;
        }
        if (!localRematch) {
            float rbx = mW * 0.2f;
            float rby = mH * 0.53f;
            float rbw = mW * 0.28f;
            float rbh = mH * 0.08f;
            if (x >= rbx && x <= rbx + rbw && y >= rby && y <= rby + rbh) {
                localRematch = true;
                NetSendRematch();
            }
        }
        float qbx = mW * 0.52f;
        float qby = mH * 0.53f;
        float qbw = mW * 0.28f;
        float qbh = mH * 0.08f;
        if (x >= qbx && x <= qbx + qbw && y >= qby && y <= qby + qbh) {
            SetState(STATE_MAIN_MENU);
        }
    }
}

void Game::MouseMotion(int x, int y) {
    mMouseMotionX = x; mMouseMotionY = y;
    mMouseX = x; mMouseY = y;
}

void Game::UpdatePlayer(int id, float dt) {
    Player& p = players[id];
    if (!p.alive) return;

    InputState inp = BuildInputState(id);

    if (inp.rotateLeft)  p.rotation += PLAYER_ROTATION_SPEED * dt;
    if (inp.rotateRight) p.rotation -= PLAYER_ROTATION_SPEED * dt;

    float rad = p.rotation * (float)M_PI / 180.0f;
    float dirX = cosf(rad);
    float dirZ = -sinf(rad);

    if (inp.thrustForward) {
        p.vx += dirX * PLAYER_ACCELERATION * dt;
        p.vz += dirZ * PLAYER_ACCELERATION * dt;
    }

    p.vx -= p.vx * PLAYER_DRAG * dt;
    p.vz -= p.vz * PLAYER_DRAG * dt;

    float speed = sqrtf(p.vx * p.vx + p.vz * p.vz);
    if (speed > PLAYER_MAX_SPEED) {
        p.vx = (p.vx / speed) * PLAYER_MAX_SPEED;
        p.vz = (p.vz / speed) * PLAYER_MAX_SPEED;
    }

    p.x += p.vx * dt;
    p.z += p.vz * dt;

    if (p.x > WORLD_SIZE) p.x -= 2 * WORLD_SIZE;
    if (p.x < -WORLD_SIZE) p.x += 2 * WORLD_SIZE;
    if (p.z > WORLD_SIZE) p.z -= 2 * WORLD_SIZE;
    if (p.z < -WORLD_SIZE) p.z += 2 * WORLD_SIZE;

    p.shootCooldown -= dt;
    if (p.shootCooldown < 0.0f) p.shootCooldown = 0.0f;
    if (inp.shoot) SpawnProjectile(id);

    p.invulnTime -= dt;
    if (p.invulnTime < 0.0f) p.invulnTime = 0.0f;
}

void Game::UpdatePlaying(float dt) {
    if (dt <= 0.0f || dt > 0.1f) return;

    UpdatePlayer(0, dt);
    UpdatePlayer(1, dt);

    // Projectile movement
    for (int i = 0; i < (int)projectiles.size(); i++) {
        Projectile& pr = projectiles[i];
        pr.x += pr.vx * dt;
        pr.z += pr.vz * dt;
        if (pr.x > WORLD_SIZE || pr.x < -WORLD_SIZE ||
            pr.z > WORLD_SIZE || pr.z < -WORLD_SIZE) {
            projectiles.erase(projectiles.begin() + i);
            i--;
        }
    }

    // Asteroid movement
    for (int i = 0; i < (int)asteroids.size(); i++) {
        Asteroid& a = asteroids[i];
        a.x += a.vx * dt;
        a.z += a.vz * dt;
        a.rotation += a.rotationspeed * dt;
        if (a.x > WORLD_SIZE) a.x -= 2 * WORLD_SIZE;
        if (a.x < -WORLD_SIZE) a.x += 2 * WORLD_SIZE;
        if (a.z > WORLD_SIZE) a.z -= 2 * WORLD_SIZE;
        if (a.z < -WORLD_SIZE) a.z += 2 * WORLD_SIZE;
    }

    // Asteroid respawn (timer based)
    asteroidSpawnTimer += dt;
    if (asteroidSpawnTimer >= ASTEROID_SPAWN_INTERVAL) {
        asteroidSpawnTimer = 0.0f;
        if ((int)asteroids.size() < ASTEROID_MIN_COUNT &&
            (int)asteroids.size() < ASTEROID_MAX_COUNT) {
            float x, z;
            int edge = rand() % 4;
            if (edge == 0) { x = -WORLD_SIZE; z = (float)(rand() % (int)(2 * WORLD_SIZE)) - WORLD_SIZE; }
            else if (edge == 1) { x = WORLD_SIZE; z = (float)(rand() % (int)(2 * WORLD_SIZE)) - WORLD_SIZE; }
            else if (edge == 2) { x = (float)(rand() % (int)(2 * WORLD_SIZE)) - WORLD_SIZE; z = -WORLD_SIZE; }
            else { x = (float)(rand() % (int)(2 * WORLD_SIZE)) - WORLD_SIZE; z = WORLD_SIZE; }
            SpawnAsteroidBySize(0, x, z);
        }
    }

    // Projectile vs Asteroid (substep)
    for (int i = 0; i < (int)projectiles.size(); i++) {
        Projectile& pr = projectiles[i];
        bool removed = false;
        for (int j = 0; j < (int)asteroids.size(); j++) {
            Asteroid& a = asteroids[j];

            float px = pr.x - pr.vx * dt;
            float pz = pr.z - pr.vz * dt;
            float stepDist = sqrtf(pr.vx * pr.vx + pr.vz * pr.vz) * dt;
            int steps = (int)(stepDist / (a.radius * 0.5f)) + 1;
            if (steps > 20) steps = 20;

            bool hit = false;
            for (int s = 0; s <= steps; s++) {
                float frac = (steps == 0) ? 1.0f : (float)s / (float)steps;
                float cx = px + (pr.x - px) * frac;
                float cz = pz + (pr.z - pz) * frac;
                float dx = cx - a.x, dz = cz - a.z;
                if (sqrtf(dx * dx + dz * dz) < a.radius * 1.5f + PROJECTILE_RADIUS) {
                    hit = true;
                    break;
                }
            }

            if (hit) {
                if (a.size == 0) {
                    players[pr.ownerId].score += SCORE_BIG_SPLIT;
                    float ax = a.x, az = a.z;
                    asteroids.erase(asteroids.begin() + j);
                    SpawnAsteroidBySize(1, ax, az);
                    SpawnAsteroidBySize(1, ax, az);
                }
                else if (a.size == 1) {
                    players[pr.ownerId].score += SCORE_MID_SPLIT;
                    float ax = a.x, az = a.z;
                    asteroids.erase(asteroids.begin() + j);
                    SpawnAsteroidBySize(2, ax, az);
                    SpawnAsteroidBySize(2, ax, az);
                }
                else {
                    players[pr.ownerId].score += SCORE_SMALL_DESTROY;
                    asteroids.erase(asteroids.begin() + j);
                }
                projectiles.erase(projectiles.begin() + i);
                i--;
                removed = true;
                break;
            }
        }
    }

    // Projectile vs Player (substep to prevent tunneling)
    for (int i = 0; i < (int)projectiles.size(); i++) {
        Projectile& pr = projectiles[i];
        bool removed = false;
        for (int pid = 0; pid < 2; pid++) {
            if (pr.ownerId == pid) continue;
            Player& t = players[pid];
            if (!t.alive || t.invulnTime > 0.0f) continue;

            float px = pr.x - pr.vx * dt;
            float pz = pr.z - pr.vz * dt;
            float stepDist = sqrtf(pr.vx * pr.vx + pr.vz * pr.vz) * dt;
            int steps = (int)(stepDist / (PLAYER_RADIUS * 0.5f)) + 1;
            if (steps > 20) steps = 20;

            for (int s = 0; s <= steps; s++) {
                float frac = (steps == 0) ? 1.0f : (float)s / (float)steps;
                float cx = px + (pr.x - px) * frac;
                float cz = pz + (pr.z - pz) * frac;
                float dx = cx - t.x, dz = cz - t.z;
                if (sqrtf(dx * dx + dz * dz) < PLAYER_RADIUS + PROJECTILE_RADIUS) {
                    t.lives--;
                    t.invulnTime = PLAYER_INVULN_TIME;
                    if (t.lives <= 0) t.alive = false;
                    projectiles.erase(projectiles.begin() + i);
                    i--;
                    removed = true;
                    break;
                }
            }
            if (removed) break;
        }
    }

    // Asteroid vs Player
    for (int j = 0; j < (int)asteroids.size(); j++) {
        Asteroid& a = asteroids[j];
        for (int pid = 0; pid < 2; pid++) {
            Player& pl = players[pid];
            if (!pl.alive || pl.invulnTime > 0.0f) continue;
            float dx = pl.x - a.x, dz = pl.z - a.z;
            if (sqrtf(dx * dx + dz * dz) < a.radius * 1.5f + PLAYER_RADIUS) {
                pl.lives--;
                pl.invulnTime = PLAYER_INVULN_TIME;
                if (pl.lives <= 0) pl.alive = false;
                asteroids.erase(asteroids.begin() + j);
                j--;
                break;
            }
        }
    }

    // Player vs Player
    if (players[0].alive && players[1].alive &&
        players[0].invulnTime <= 0.0f && players[1].invulnTime <= 0.0f) {
        float dx = players[0].x - players[1].x, dz = players[0].z - players[1].z;
        if (sqrtf(dx * dx + dz * dz) < PLAYER_RADIUS * 2) {
            players[0].lives--; players[0].invulnTime = PLAYER_INVULN_TIME;
            if (players[0].lives <= 0) players[0].alive = false;
            players[1].lives--; players[1].invulnTime = PLAYER_INVULN_TIME;
            if (players[1].lives <= 0) players[1].alive = false;
        }
    }

    // Timer
    roundTimer -= dt;
    if (roundTimer < 0.0f) roundTimer = 0.0f;

    CheckRoundEnd();

}

void Game::CheckRoundEnd() {
    bool p0Dead = !players[0].alive;
    bool p1Dead = !players[1].alive;

    if (p0Dead || p1Dead) {
        if (p0Dead && p1Dead) roundWinner = -1;
        else if (p1Dead) { roundWinner = 0; roundWins[0]++; }
        else { roundWinner = 1; roundWins[1]++; }

        if (roundWins[0] >= ROUNDS_TO_WIN || roundWins[1] >= ROUNDS_TO_WIN)
            SetState(STATE_MATCH_END);
        else
            SetState(STATE_ROUND_END);
        return;
    }

    if (roundTimer <= 0.0f) {
        if (players[0].score > players[1].score) { roundWinner = 0; roundWins[0]++; }
        else if (players[1].score > players[0].score) { roundWinner = 1; roundWins[1]++; }
        else { roundWinner = -1; }

        if (roundWins[0] >= ROUNDS_TO_WIN || roundWins[1] >= ROUNDS_TO_WIN)
            SetState(STATE_MATCH_END);
        else
            SetState(STATE_ROUND_END);
    }
}

void Game::UpdateRoundEnd(float dt) {
    roundEndTimer -= dt;
    if (roundEndTimer <= 0.0f) {
        if (currentState == STATE_MATCH_END) {
            SetState(STATE_MAIN_MENU);
        }
        else {
            ResetRound();
            SetState(STATE_PLAYING);
        }
    }
}

InputState Game::BuildInputState(int id) {
    InputState inp;
    inp.playerId = id;

    if (isMultiplayer) {
        if (id == NetGetPlayerId()) {
            // Local player always uses WASD
            inp.rotateLeft = keys['a'];
            inp.rotateRight = keys['d'];
            inp.thrustForward = keys['w'];
            inp.shoot = keys[' '];
        }
        else {
            // Remote player input comes from network
            inp.thrustForward = remThrust;
            inp.rotateLeft = remLeft;
            inp.rotateRight = remRight;
            inp.shoot = remShoot;
        }
    }
    else {
        // Local singleplayer: P1 = WASD, P2 = IJKL
        if (id == 0) {
            inp.rotateLeft = keys['a'];
            inp.rotateRight = keys['d'];
            inp.thrustForward = keys['w'];
            inp.shoot = keys[' '];
        }
        else {
            inp.rotateLeft = keys['j'];
            inp.rotateRight = keys['l'];
            inp.thrustForward = keys['i'];
            inp.shoot = keys['k'];
        }
    }
    return inp;
}

void Game::SpawnProjectile(int playerId) {
    Player& p = players[playerId];
    if (p.shootCooldown > 0.0f || !p.alive) return;
    float rad = p.rotation * (float)M_PI / 180.0f;
    float dirX = cosf(rad), dirZ = -sinf(rad);
    Projectile proj;
    proj.id = nextProjectileId++;
    proj.ownerId = playerId;
    proj.x = p.x + dirX * 2.5f;
    proj.z = p.z + dirZ * 2.5f;
    proj.vx = dirX * PROJECTILE_SPEED;
    proj.vz = dirZ * PROJECTILE_SPEED;
    projectiles.push_back(proj);
    p.shootCooldown = SHOOT_COOLDOWN;
}

void Game::SpawnAsteroid(bool isLarge, float x, float z) {
    if ((int)asteroids.size() >= ASTEROID_MAX_COUNT) return;
    Asteroid a;
    a.id = nextAsteroidId++;
    a.x = x; a.z = z;
    if (isLarge) {
        a.size = 0;
        a.radius = ASTEROID_BIG_RADIUS;
    }
    else {
        a.size = 2;
        a.radius = ASTEROID_SMALL_RADIUS;
    }
    a.radius *= 0.95f + (rand() % 11) / 100.0f;
    float angle = (rand() % 360) * (float)M_PI / 180.0f;
    float spd = (a.size == 0) ? ASTEROID_BIG_SPEED : ASTEROID_SMALL_SPEED;
    a.vx = cosf(angle) * spd;
    a.vz = sinf(angle) * spd;
    a.rotation = (float)(rand() % 360);
    a.rotationspeed = 30.0f + (float)(rand() % 90);
    asteroids.push_back(a);
}

void Game::SpawnAsteroidBySize(int size, float x, float z) {
    if ((int)asteroids.size() >= ASTEROID_MAX_COUNT) return;
    Asteroid a;
    a.id = nextAsteroidId++;
    a.x = x; a.z = z;
    a.size = size;
    if (size == 0)      a.radius = ASTEROID_BIG_RADIUS;
    else if (size == 1) a.radius = ASTEROID_MID_RADIUS;
    else                a.radius = ASTEROID_SMALL_RADIUS;
    a.radius *= 0.95f + (rand() % 11) / 100.0f;
    float angle = (rand() % 360) * (float)M_PI / 180.0f;
    float spd;
    if (size == 0)      spd = ASTEROID_BIG_SPEED;
    else if (size == 1) spd = ASTEROID_MID_SPEED;
    else                spd = ASTEROID_SMALL_SPEED;
    a.vx = cosf(angle) * spd;
    a.vz = sinf(angle) * spd;
    a.rotation = (float)(rand() % 360);
    a.rotationspeed = 30.0f + (float)(rand() % 90);
    asteroids.push_back(a);
}

void Game::SpawnInitialAsteroids() {
    asteroids.clear();
    for (int i = 0; i < ASTEROID_INITIAL_COUNT; i++) {
        float x, z;
        int attempts = 0;
        bool valid;
        do {
            x = ((rand() % ((int)(ASTEROID_CENTER_ZONE * 20))) / 10.0f) - ASTEROID_CENTER_ZONE;
            z = ((rand() % ((int)(ASTEROID_CENTER_ZONE * 20))) / 10.0f) - ASTEROID_CENTER_ZONE;
            valid = true;
            float d0 = sqrtf((x - players[0].x) * (x - players[0].x) + (z - players[0].z) * (z - players[0].z));
            float d1 = sqrtf((x - players[1].x) * (x - players[1].x) + (z - players[1].z) * (z - players[1].z));
            if (d0 < ASTEROID_MIN_PLAYER_DIST || d1 < ASTEROID_MIN_PLAYER_DIST)
                valid = false;
            for (int j = 0; j < (int)asteroids.size(); j++) {
                float da = sqrtf((x - asteroids[j].x) * (x - asteroids[j].x) + (z - asteroids[j].z) * (z - asteroids[j].z));
                if (da < ASTEROID_MIN_SPACING) { valid = false; break; }
            }
            attempts++;
        } while (!valid && attempts < 50);
        SpawnAsteroidBySize(0, x, z);
    }
}
GameState Game::GetGameState() const {
    GameState gs;
    gs.currentState = currentState;
    gs.players[0] = players[0];
    gs.players[1] = players[1];
    gs.roundTimer = roundTimer;
    gs.roundNumber = roundNumber;
    gs.roundWins[0] = roundWins[0];
    gs.roundWins[1] = roundWins[1];
    gs.roundWinner = roundWinner;
    gs.roundEndTimer = roundEndTimer;
    return gs;
}

void Game::ApplyGameState(const GameState& gs) {
    currentState = gs.currentState;
    players[0] = gs.players[0];
    players[1] = gs.players[1];
    roundTimer = gs.roundTimer;
    roundNumber = gs.roundNumber;
    roundWins[0] = gs.roundWins[0];
    roundWins[1] = gs.roundWins[1];
    roundWinner = gs.roundWinner;
    roundEndTimer = gs.roundEndTimer;
}

void Game::Update(float dt) {

    // Global multiplayer disconnect check
    if (isMultiplayer && !opponentDisconnected && !NetIsConnected()) {
        if (currentState == STATE_PLAYING || currentState == STATE_ROUND_END) {
            opponentDisconnected = true;
            if (NetGetPlayerId() == 0)
                roundWins[0] = ROUNDS_TO_WIN;
            else
                roundWins[1] = ROUNDS_TO_WIN;
            SetState(STATE_MATCH_END);
            return;
        }
    }

    switch (currentState) {
    case STATE_LOBBY:
        if (isMultiplayer && NetIsConnected()) {
            // Send nickname once when server says START (both connected)
            if (NetIsStarted() && !nicknameSent) {
                nicknameSent = true;
                NetSendNickname(nickname);
            }

            // Receive opponent nickname
            if (!opponentConnected) {
                char recvName[32];
                if (NetGetNickname(recvName, 32)) {
                    opponentConnected = true;
                    if (NetGetPlayerId() == 0) {
                        // I'm host (player 0, left side)
                        strcpy(player1Name, nickname);
                        strcpy(player2Name, recvName);
                    }
                    else {
                        // I'm joiner (player 1, right side)
                        strcpy(player1Name, recvName);
                        strcpy(player2Name, nickname);
                    }
                }
            }

            // Check opponent ready
            if (opponentConnected && !remoteReady) {
                remoteReady = NetGetReady();
            }

            // Joiner: if host started the game, follow
            if (!isHost && NetGetPlayerId() == 1) {
                GameState gs;
                std::vector<Asteroid> ast;
                std::vector<Projectile> prj;
                if (NetGetState(gs, ast, prj)) {
                    if (gs.currentState == STATE_PLAYING) {
                        ApplyGameState(gs);
                        asteroids = ast;
                        projectiles = prj;
                    }
                }
            }
        }
        break;

    case STATE_PLAYING:
        if (isMultiplayer) {

            pingTimer += dt;
            if (pingTimer >= 1.0f) {
                pingTimer = 0.0f;
                NetSendPing();
            }

            if (NetGetPlayerId() == 0) {
                // HOST: read joiner input, run logic, send state at 30Hz
                NetGetInput(remThrust, remLeft, remRight, remShoot);
                UpdatePlaying(dt);
                netSendTimer += dt;
                if (netSendTimer >= 1.0f / 30.0f) {
                    netSendTimer = 0.0f;
                    GameState gs = GetGameState();
                    NetSendState(gs, asteroids, projectiles);
                }
            }
           else {
                // JOINER: send input at 30Hz
                netSendTimer += dt;
                if (netSendTimer >= 1.0f / 30.0f) {
                    netSendTimer = 0.0f;
                    InputState inp = BuildInputState(NetGetPlayerId());
                    NetSendInput(inp.thrustForward, inp.rotateLeft,
                        inp.rotateRight, inp.shoot);
                }
                // Local movement only (no collisions, no spawning)
                for (int i = 0; i < 2; i++) {
                    Player& p = players[i];
                    if (!p.alive) continue;
                    p.x += p.vx * dt;
                    p.z += p.vz * dt;
                    if (p.x > WORLD_SIZE) p.x -= 2 * WORLD_SIZE;
                    if (p.x < -WORLD_SIZE) p.x += 2 * WORLD_SIZE;
                    if (p.z > WORLD_SIZE) p.z -= 2 * WORLD_SIZE;
                    if (p.z < -WORLD_SIZE) p.z += 2 * WORLD_SIZE;
                }
                // Move projectiles visually
                for (int i = 0; i < (int)projectiles.size(); i++) {
                    projectiles[i].x += projectiles[i].vx * dt;
                    projectiles[i].z += projectiles[i].vz * dt;
                }
                // Move asteroids visually
                for (int i = 0; i < (int)asteroids.size(); i++) {
                    asteroids[i].x += asteroids[i].vx * dt;
                    asteroids[i].z += asteroids[i].vz * dt;
                    asteroids[i].rotation += asteroids[i].rotationspeed * dt;
                }
                // Apply server state when available (authoritative)
                GameState gs;
                std::vector<Asteroid> ast;
                std::vector<Projectile> prj;
                if (NetGetState(gs, ast, prj)) {
                    ApplyGameState(gs);
                    asteroids = ast;
                    projectiles = prj;
                }
            }
        }
        else {
            UpdatePlaying(dt);
        }
        break;

    case STATE_ROUND_END:
        if (isMultiplayer && NetGetPlayerId() != 0) {
            // Joiner: follow host state
            GameState gs;
            std::vector<Asteroid> ast;
            std::vector<Projectile> prj;
            if (NetGetState(gs, ast, prj)) {
                ApplyGameState(gs);
                asteroids = ast;
                projectiles = prj;
            }
            
        }
        else {
            UpdateRoundEnd(dt);
            // Host: send state during round end so joiner follows
            if (isMultiplayer && NetGetPlayerId() == 0) {
                netSendTimer += dt;
                if (netSendTimer >= 1.0f / 30.0f) {
                    netSendTimer = 0.0f;
                    GameState gs = GetGameState();
                    NetSendState(gs, asteroids, projectiles);
                }
            }
        }
        break;

    case STATE_MATCH_END:
        if (isMultiplayer) {

            if (!NetIsConnected() && !opponentDisconnected) {
                opponentDisconnected = true;
            }
            // Check remote rematch
            if (!remoteRematch) {
                remoteRematch = NetGetRematch();
            }
            // Both want rematch
            if (localRematch && remoteRematch) {
                localRematch = false;
                remoteRematch = false;
                ResetMatch();
                SetState(STATE_PLAYING);
                // Host sends state so joiner follows
                if (NetGetPlayerId() == 0) {
                    GameState gs = GetGameState();
                    NetSendState(gs, asteroids, projectiles);
                }
            }
            // Host still sends state at 30Hz so joiner sees the end screen
            if (NetGetPlayerId() == 0) {
                netSendTimer += dt;
                if (netSendTimer >= 1.0f / 30.0f) {
                    netSendTimer = 0.0f;
                    GameState gs = GetGameState();
                    NetSendState(gs, asteroids, projectiles);
                }
            }
            else {
                // Joiner receives state
                GameState gs;
                std::vector<Asteroid> ast;
                std::vector<Projectile> prj;
                if (NetGetState(gs, ast, prj)) {
                    if (gs.currentState == STATE_PLAYING) {
                        ApplyGameState(gs);
                        asteroids = ast;
                        projectiles = prj;
                    }
                }
            }
        }
        else {
            UpdateRoundEnd(dt);
        }
        break;

    default: break;
    }
}