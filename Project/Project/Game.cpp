#define _CRT_SECURE_NO_WARNINGS
#include "Game.h"

Game::Game() {
    mCounter = 0;
    mW = 1280; mH = 720.f;
    mMouseX = mMouseY = 0;
    mMouseButton = mMouseState = 0;
    mQuadratic = gluNewQuadric();
    gluQuadricNormals(mQuadratic, GLU_SMOOTH);
    memset(keys, false, sizeof(keys));
    deltatime = 0.f;
    roundTimer = ROUND_TIME;
    nextProjectileId = 0;
    nextAsteroidId = 0;

    // Player 0
    players[0].id = 0;
    players[0].x = -5.0f;  players[0].z = 0.0f;
    players[0].rotation = 0.0f;
    players[0].vx = 0.0f;  players[0].vz = 0.0f;
    players[0].lives = PLAYER_START_LIVES;
    players[0].x = -WORLD_SIZE * 0.7f;
    players[0].z = 0.0f;
    players[0].score = 0;
    players[0].shootCooldown = 0.0f;
    players[0].invulnTime = 0.0f;
    players[0].alive = true;

    // Player 1
    players[1].id = 1;
    players[1].x = 5.0f;   players[1].z = 0.0f;
    players[1].rotation = 180.0f;
    players[1].vx = 0.0f;  players[1].vz = 0.0f;
    players[1].lives = PLAYER_START_LIVES;
    players[1].x = WORLD_SIZE * 0.7f;
    players[1].z = 0.0f;
    players[1].score = 0;
    players[1].shootCooldown = 0.0f;
    players[1].invulnTime = 0.0f;
    players[1].alive = true;

    SpawnInitialAsteroids();
}

Game::~Game() {
    gluDeleteQuadric(mQuadratic);
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
    double fov = 40, nearX = .1, farX = 2500.;
    double ratio = double(mW) / double(mH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, ratio, nearX, farX);
    glMatrixMode(GL_MODELVIEW);
}

void Game::Draw(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(0, 75, 0.1, 0, 0, 0, 0, 1, 0);

    // Grid
    glLineWidth(1);
    glColor3ub(40, 40, 40);
    int n = (int)WORLD_SIZE;
    for (int i = -n; i <= n; i += 5) {
        glBegin(GL_LINES);
        glVertex3f((float)-n, 0, (float)i);
        glVertex3f((float)n, 0, (float)i);
        glVertex3f((float)i, 0, (float)-n);
        glVertex3f((float)i, 0, (float)n);
        glEnd();
    }

    // World border
    glColor3ub(80, 80, 80);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-WORLD_SIZE, 0.1f, -WORLD_SIZE);
    glVertex3f(WORLD_SIZE, 0.1f, -WORLD_SIZE);
    glVertex3f(WORLD_SIZE, 0.1f, WORLD_SIZE);
    glVertex3f(-WORLD_SIZE, 0.1f, WORLD_SIZE);
    glEnd();
    glLineWidth(1);

    // Player 0
    bool vis0 = true;
    if (players[0].invulnTime > 0.0f)
        vis0 = ((int)(players[0].invulnTime * 10) % 2 == 0);
    if (players[0].alive && vis0) {
        glColor3ub(0, 255, 128);
        glPushMatrix();
        glTranslatef(players[0].x, 0.3f, players[0].z);
        glRotatef(players[0].rotation, 0, 1, 0);
        glScalef(2.0f, 2.0f, 2.0f);
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
        glColor3ub(0, 200, 100);
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
        vis1 = ((int)(players[1].invulnTime * 10) % 2 == 0);
    if (players[1].alive && vis1) {
        glColor3ub(255, 100, 50);
        glPushMatrix();
        glTranslatef(players[1].x, 0.3f, players[1].z);
        glRotatef(players[1].rotation, 0, 1, 0);
        glScalef(2.0f, 2.0f, 2.0f);
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
        glColor3ub(200, 80, 40);
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
        if (a.isLarge) glColor3ub(160, 120, 80);
        else glColor3ub(130, 100, 70);
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
        glColor3ub(220, 200, 160);
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
    char title[256];
    sprintf(title, "P1 Score:%d Lives:%d | P2 Score:%d Lives:%d | Time:%.0f",
        players[0].score, players[0].lives,
        players[1].score, players[1].lives, roundTimer);
    SDL_SetWindowTitle(gScreen, title);

    mCounter++;
}

void Game::NormalKeys(unsigned char key, int state) {}
void Game::SpecialKeys(int key, int state) {}
void Game::Mouse(int button, int state, int x, int y) {
    mMouseButton = button; mMouseState = state;
    mMouseX = x; mMouseY = y;
}
void Game::MouseMotion(int x, int y) {
    mMouseMotionX = x; mMouseMotionY = y;
}

void Game::UpdatePlayer(int id, float dt) {
    Player& p = players[id];
    if (!p.alive) return;

    bool kLeft, kRight, kThrust, kShoot;
    if (id == 0) {
        kLeft = keys['a']; kRight = keys['d'];
        kThrust = keys['w']; kShoot = keys[' '];
    }
    else {
        kLeft = keys['j']; kRight = keys['l'];
        kThrust = keys['i']; kShoot = keys['k'];
    }

    if (kLeft)  p.rotation += PLAYER_ROTATION_SPEED * dt;
    if (kRight) p.rotation -= PLAYER_ROTATION_SPEED * dt;

    float rad = p.rotation * (float)M_PI / 180.0f;
    float dirX = cosf(rad);
    float dirZ = -sinf(rad);

    if (kThrust) {
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
    if (kShoot) SpawnProjectile(id);

    p.invulnTime -= dt;
    if (p.invulnTime < 0.0f) p.invulnTime = 0.0f;
}

void Game::Update(float dt) {
    deltatime = dt;
    if (dt <= 0.0f || dt > 0.1f) return;

    UpdatePlayer(0, dt);
    UpdatePlayer(1, dt);

    // Projectiles
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

    // Asteroids
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

    // Respawn
    if ((int)asteroids.size() < ASTEROID_MIN_COUNT) {
        float x, z;
        int edge = rand() % 4;
        if (edge == 0) { x = -WORLD_SIZE; z = (float)(rand() % (int)(2 * WORLD_SIZE)) - WORLD_SIZE; }
        else if (edge == 1) { x = WORLD_SIZE;  z = (float)(rand() % (int)(2 * WORLD_SIZE)) - WORLD_SIZE; }
        else if (edge == 2) { x = (float)(rand() % (int)(2 * WORLD_SIZE)) - WORLD_SIZE; z = -WORLD_SIZE; }
        else { x = (float)(rand() % (int)(2 * WORLD_SIZE)) - WORLD_SIZE; z = WORLD_SIZE; }
        SpawnAsteroid(true, x, z);
    }

    // Projectile vs Asteroid
    for (int i = 0; i < (int)projectiles.size(); i++) {
        Projectile& pr = projectiles[i];
        for (int j = 0; j < (int)asteroids.size(); j++) {
            Asteroid& a = asteroids[j];
            float dx = pr.x - a.x, dz = pr.z - a.z;
            float dist = sqrtf(dx * dx + dz * dz);
            if (dist < a.radius * 1.3f + PROJECTILE_RADIUS) {
                
                players[pr.ownerId].score += a.isLarge ? SCORE_LARGE_SPLIT : SCORE_SMALL_DESTROY;
                if (a.isLarge) {
                    float ax = a.x, az = a.z;
                    asteroids.erase(asteroids.begin() + j);
                    SpawnAsteroid(false, ax, az);
                    SpawnAsteroid(false, ax, az);
                }
                else {
                    asteroids.erase(asteroids.begin() + j);
                }
                projectiles.erase(projectiles.begin() + i);
                i--;
                break;
            }
        }
    }

    // Projectile vs Player
    for (int i = 0; i < (int)projectiles.size(); i++) {
        Projectile& pr = projectiles[i];
        for (int pid = 0; pid < 2; pid++) {
            if (pr.ownerId == pid) continue;
            Player& t = players[pid];
            if (!t.alive || t.invulnTime > 0.0f) continue;
            float dx = pr.x - t.x, dz = pr.z - t.z;
            if (sqrtf(dx * dx + dz * dz) < PLAYER_RADIUS + PROJECTILE_RADIUS) {
                t.lives--;
                t.invulnTime = PLAYER_INVULN_TIME;
                if (t.lives <= 0) t.alive = false;
                projectiles.erase(projectiles.begin() + i);
                i--;
                break;
            }
        }
    }

    // Asteroid vs Player
    for (int j = 0; j < (int)asteroids.size(); j++) {
        Asteroid& a = asteroids[j];
        for (int pid = 0; pid < 2; pid++) {
            Player& pl = players[pid];
            if (!pl.alive || pl.invulnTime > 0.0f) continue;
            float dx = pl.x - a.x, dz = pl.z - a.z;
            if (sqrtf(dx * dx + dz * dz) < a.radius * 1.3f + PLAYER_RADIUS) {
            
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
    Asteroid a;
    a.id = nextAsteroidId++;
    a.x = x; a.z = z;
    a.isLarge = isLarge;
    a.radius = isLarge ? ASTEROID_LARGE_RADIUS : ASTEROID_SMALL_RADIUS;
    a.radius *= 0.95f + (rand() % 11) / 100.0f;
    float angle = (rand() % 360) * (float)M_PI / 180.0f;
    float spd = isLarge ? ASTEROID_LARGE_SPEED : ASTEROID_SMALL_SPEED;
    a.vx = cosf(angle) * spd;
    a.vz = sinf(angle) * spd;
    a.rotation = (float)(rand() % 360);
    a.rotationspeed = 30.0f + (float)(rand() % 90);
    asteroids.push_back(a);
}

void Game::SpawnInitialAsteroids() {
    asteroids.clear();
    for (int i = 0; i < ASTEROID_INITIAL_COUNT; i++) {
        float x = -10.0f + (rand() % 200) / 10.0f;
        float z = -10.0f + (rand() % 200) / 10.0f;
        SpawnAsteroid(true, x, z);
    }
}