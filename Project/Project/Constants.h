#pragma once

//World

const float WORLD_SIZE = 20.0f;  // play area: -50 to +50 in x and z

//Player

const float PLAYER_ROTATION_SPEED = 180.0f; // degrees per second
const float PLAYER_ACCELERATION = 15.0f; // units per second^2
const float PLAYER_MAX_SPEED = 10.0f;
const float PLAYER_DRAG = 2.0f;
const float PLAYER_RADIUS = 0.5f; // for collision
const int PLAYER_START_LIVES = 3;
const float PLAYER_INVULN_TIME = 1.5f; // seconds of invulnerability after hit

//Projectile

const float PROJECTILE_SPEED = 30.0f;
const float SHOOT_COOLDOWN = 1.0f;
const float PROJECTILE_RADIUS = 0.1f; // for collision

//Asteroid

const float ASTEROID_LARGE_RADIUS = 3.0f;
const float ASTEROID_SMALL_RADIUS = 1.5f;
const float ASTEROID_LARGE_SPEED = 3.0f;
const float ASTEROID_SMALL_SPEED = 5.0f;
const int ASTEROID_INITIAL_COUNT = 4;
const int ASTEROID_MIN_COUNT = 4;
const int ASTEROID_MAX_COUNT = 20;
const int SCORE_LARGE_SPLIT = 10;
const int SCORE_SMALL_DESTROY = 20;

//Match

const float ROUND_TIME = 60.0f; // seconds
const int ROUNDS_TO_WIN = 2;

