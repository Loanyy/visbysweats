#pragma once
#include <vector>
#include <cmath>
#include "Constants.h"

enum GameStateEnum {
    STATE_MAIN_MENU = 0,
    STATE_RULES = 1,
    STATE_CREDITS = 2,
    STATE_NICKNAME = 3,
    STATE_LOBBY = 4,
    STATE_PLAYING = 5,
    STATE_ROUND_END = 6,
    STATE_MATCH_END = 7,
};

struct Player {
    int id;
    float x, z;
    float rotation;
    float vx, vz;
    int lives;
    int score;
    float shootCooldown;
    float invulnTime;
    bool alive;
};

struct Asteroid {
    int id;
    float x, z;
    float vx, vz;
    float rotation;
    float rotationspeed;
    float radius;
    int size; // 0 = big, 1 = mid, 2 = small 
};

struct Projectile {
    int id;
    int ownerId;
    float x, z;
    float vx, vz;
};

struct InputState {
    int  playerId;
    bool thrustForward;
    bool rotateLeft;
    bool rotateRight;
    bool shoot;
};

struct GameState {
    GameStateEnum currentState;
    Player        players[2];
    float         roundTimer;
    int           roundNumber;
    int           roundWins[2];
    int           roundWinner;
    float         roundEndTimer;
};