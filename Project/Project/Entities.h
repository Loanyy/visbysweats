#pragma once
#include <vector>
#include <cmath>
#include "Constants.h"

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
	bool isLarge;
};

struct Projectile {
	int id;
	int ownerId;
	float x, z;
	float vx, vz;
};
