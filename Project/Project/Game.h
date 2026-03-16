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

#define For(i,N) for (int (i) = 0; (i) < (N); (i)++)

extern SDL_Window* gScreen;

class Game {
public:
    Game(void);
    ~Game(void);

    void Update(float dt);
    void UpdatePlayer(int id, float dt);
    void SpawnProjectile(int playerId);
    void SpawnAsteroid(bool isLarge, float x, float z);
    void SpawnInitialAsteroids();

    void InitGFX();
    void ChangeSize(int w, int h);
    void Draw();

    void NormalKeys(unsigned char key, int state);
    void SpecialKeys(int key, int state);
    void Mouse(int button, int state, int x, int y);
    void MouseMotion(int x, int y);

    Player players[2];
    std::vector<Asteroid> asteroids;
    std::vector<Projectile> projectiles;

    bool keys[256];
    float deltatime;
    float roundTimer;
    int nextProjectileId;
    int nextAsteroidId;

    float mW, mH;
    int mCounter;
    int mMouseX, mMouseY, mMouseMotionX, mMouseMotionY;
    int mMouseButton, mMouseState;
    GLUquadricObj* mQuadratic;
};