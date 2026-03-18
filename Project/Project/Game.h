#pragma once
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <vector>
#include "Constants.h"
#include "Entities.h"
#include <gl/GLU.h>
#include <SDL_ttf.h>

#define For(i,N) for (int (i) = 0; (i) < (N); (i)++)

extern SDL_Window* gScreen;

class Game {
public:
    Game(void);
    ~Game(void);

    // State machine
    void SetState(GameStateEnum s);
    void ResetRound();
    void ResetMatch();

    // Update
    void Update(float dt);
    void UpdatePlaying(float dt);
    void UpdateRoundEnd(float dt);
    void UpdatePlayer(int id, float dt);
    void CheckRoundEnd();

    // Spawning
    void SpawnProjectile(int playerId);
    void SpawnAsteroid(bool isLarge, float x, float z);
    void SpawnAsteroidBySize(int size, float x, float z);
    void SpawnInitialAsteroids();

    // Networking helpers
    InputState BuildInputState(int id);
    GameState  GetGameState() const;
    void       ApplyGameState(const GameState& gs);

    // Rendering
    void InitGFX();
    void ChangeSize(int w, int h);
    void Draw();
    void DrawGame();
    void DrawMainMenu();
    void DrawNickname();
    void DrawLobby();
    void DrawRoundEnd();
    void DrawMatchEnd();
    void DrawRules();
    void DrawCredits();

    // Input
    void NormalKeys(unsigned char key, int state);
    void SpecialKeys(int key, int state);
    void Mouse(int button, int state, int x, int y);
    void MouseMotion(int x, int y);

    // Game data
    Player players[2];
    std::vector<Asteroid> asteroids;
    std::vector<Projectile> projectiles;

    // State
    GameStateEnum currentState;
    int   roundNumber;
    int   roundWins[2];
    int   roundWinner;
    float roundEndTimer;
    float roundTimer;
    float asteroidSpawnTimer;
    int   nextProjectileId;
    int   nextAsteroidId;

    // Menu
    int menuSelection;
    char nickname[32];
    char player1Name[32];
    char player2Name[32];
    bool enteringNickname;
    int lastMouseX, lastMouseY;
    int nicknameLen;
    int lobbyChoice;       // 0=HOST, 1=JOIN
    char lobbyCode[8];
    char lobbyCodeInput[8];
    int lobbyCodeLen;
    bool isHost;
    bool joinActive;
    bool lobbyReady;

    // Input
    bool keys[256];

    // Window / GL
    float mW, mH;
    int mCounter;
    int mMouseX, mMouseY, mMouseMotionX, mMouseMotionY;
    int mMouseButton, mMouseState;
    GLUquadricObj* mQuadratic;

    TTF_Font* fontLarge;
    TTF_Font* fontMedium;
    TTF_Font* fontSmall;
    void DrawText(const char* text, float x, float y, TTF_Font* font,
        unsigned char r, unsigned char g, unsigned char b);
};