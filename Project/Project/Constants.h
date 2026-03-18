#pragma once

// World
const float WORLD_SIZE = 35.0f;

// Player
const float PLAYER_SPAWN_X = 25.0f;
const float PLAYER_ROTATION_SPEED = 180.0f;
const float PLAYER_ACCELERATION = 20.0f;
const float PLAYER_MAX_SPEED = 14.0f;
const float PLAYER_DRAG = 2.0f;
const float PLAYER_RADIUS = 1.0f;
const int   PLAYER_START_LIVES = 3;
const float PLAYER_INVULN_TIME = 1.5f;
const float PLAYER_FLASH_RATE = 0.1f;

// Projectile
const float PROJECTILE_SPEED = 30.0f;
const float SHOOT_COOLDOWN = 0.6f;
const float PROJECTILE_RADIUS = 0.1f;

// Asteroid
const float ASTEROID_BIG_RADIUS = 5.0f;
const float ASTEROID_MID_RADIUS = 3.0f;
const float ASTEROID_SMALL_RADIUS = 1.5f;
const float ASTEROID_BIG_SPEED = 2.0f;
const float ASTEROID_MID_SPEED = 3.5f;
const float ASTEROID_SMALL_SPEED = 5.0f;
const int   ASTEROID_INITIAL_COUNT = 4;
const int   ASTEROID_MIN_COUNT = 4;
const int   ASTEROID_MAX_COUNT = 25;
const float ASTEROID_SPAWN_INTERVAL = 2.5f;
const float ASTEROID_CENTER_ZONE = 12.0f;
const float ASTEROID_MIN_PLAYER_DIST = 10.0f;
const float ASTEROID_MIN_SPACING = 6.0f;

// Scoring
const int   SCORE_BIG_SPLIT = 5;
const int   SCORE_MID_SPLIT = 10;
const int   SCORE_SMALL_DESTROY = 20;

// Match
const float ROUND_TIME = 30.0f;
const int   ROUNDS_TO_WIN = 2;
const float ROUND_END_PAUSE = 3.0f;