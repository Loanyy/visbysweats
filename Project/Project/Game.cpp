//---------------------------------------------------------------------
//---------------------------------------------------------------------
//  (c) Mikael Fridenfalk
//  All rights reserved
//  Template for use in the course:
//  Linear Algebra, Trigonometry and Geometry, 7.5 c
//  Uppsala University, Sweden
//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include "Game.h"
//---------------------------------------------------------------------
//---------------------------------------------------------------------
Game::Game(){
	mCounter = 0;
	mW = 1280, mH = 720.f;
	mMouseX = mMouseY = 0;
    mMouseButton = mMouseState = 0;
    mQuadratic = gluNewQuadric();   
    gluQuadricNormals(mQuadratic, GLU_SMOOTH);
	memset(keys, false, sizeof(keys));
	deltatime = 0.f;

    //player 0 init
	players[0].id = 0;
    players[0].x = -5.0f;
    players[0].z = 0.0f;
	players[0].rotation = 0.0f;
    players[0].vx = 0.0f;
	players[0].vz = 0.0f;
	players[0].lives = PLAYER_START_LIVES;
	players[0].score = 0;
	players[0].shootCooldown = 0.0f;
	players[0].invulnTime = 0.0f;
	players[0].alive = true;

}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
Game::~Game(){
    gluDeleteQuadric(mQuadratic);
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Game::InitGFX(){
    
    glClearColor(.0f,.0f,.0f,1.f);
    
    //---------------------------------------
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
    
    glEnable(GL_LIGHTING);
    glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    //---------------------------------------
    
    GLfloat ambient[] = {0.3, 0.3, 0.3, 1.0};
    GLfloat diffuse[] = {0.8, 0.8, 0.8, 1.0};
    GLfloat position[] = {2.0, 2.0, 2.0, 0.0};
    
    GLfloat front_mat_shininess[] = {60.0};
    GLfloat front_mat_specular[]  = {0.2, 0.2, 0.2, 1.0};
    GLfloat front_mat_diffuse[]   = {0.5, 0.5, 0.28, 1.0};
    GLfloat back_mat_shininess[]  = {60.0};
    GLfloat back_mat_specular[]   = {0.5, 0.5, 0.2, 1.0};
    GLfloat back_mat_diffuse[]    = {1.0, 0.9, 0.2, 1.0};
    
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    
    glMaterialfv(GL_FRONT, GL_SHININESS, front_mat_shininess);
    glMaterialfv(GL_FRONT, GL_SPECULAR, front_mat_specular);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, front_mat_diffuse);
    glMaterialfv(GL_BACK, GL_SHININESS, back_mat_shininess);
    glMaterialfv(GL_BACK, GL_SPECULAR, back_mat_specular);
    glMaterialfv(GL_BACK, GL_DIFFUSE, back_mat_diffuse);

    const GLfloat lmodel_twoside[] = {GL_TRUE};
    glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
    //---------------------------------------
    
    glEnable(GL_LIGHT0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //---------------------------------------
    
    glEnable(GL_MULTISAMPLE_ARB);//glEnable(GL_MULTISAMPLE);
    //---------------------------------------

}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Game::ChangeSize(int w, int h){
    mW = w, mH = h;
    //Reset Viewport
	glViewport(0,0,mW,mH);
    //Set Perspective
    double fov = 30, nearX = .1, farX = 2500.;
    double ratio = double(mW)/double(mH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, ratio, nearX, farX);
    //glRotatef(90,0,0,1);
    glMatrixMode(GL_MODELVIEW);
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Game::Draw(void){
    
    //--------------------------------------------Clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
	gluLookAt(0, 90, 0.1, 0, 0, 0, 0, 1, 0);  // eye, center, up
    
    //-----------------------
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
    
    //-----------------------Origin
    glColor3ub(32,32,32);
    gluSphere(mQuadratic,.02,32,32);
    //-----------------------x-axis
    glColor3ub(255,0,0);
    glPushMatrix();
    glTranslatef(1,0,0);
    glRotatef(90,0,1,0);
    gluCylinder(mQuadratic,.06,0.,0.,32,32);
    gluCylinder(mQuadratic,.06,0.,.3,32,32);
    glTranslatef(0,0,-1);
    gluCylinder(mQuadratic,.01,.01,1.,32,32);
    glPopMatrix();
    //-----------------------y-axis
    glColor3ub(0,255,0);
    glPushMatrix();
    glTranslatef(0,1,0);
    glRotatef(-90,1,0,0);
    gluCylinder(mQuadratic,.06,0.,0.,32,32);
    gluCylinder(mQuadratic,.06,0.,.3,32,32);
    glTranslatef(0,0,-1);
    gluCylinder(mQuadratic,.01,.01,1.,32,32);
    glPopMatrix();
    //-----------------------z-axis
    glColor3ub(0,0,255);
    glPushMatrix();
    glTranslatef(0,0,1);
    gluCylinder(mQuadratic,.06,0.,0.,32,32);
    gluCylinder(mQuadratic,.06,0.,.3,32,32);
    glTranslatef(0,0,-1);
    gluCylinder(mQuadratic,.01,.01,1.,32,32);
    glPopMatrix();

    //Player0

	glColor3ub(0, 255, 128);
	glPushMatrix();
	glTranslatef(players[0].x, 0.3f, players[0].z);
	glRotatef(players[0].rotation, 0, 1, 0);//
	glScalef(2.0f, 2.0f, 2.0f);

        //simple arrow shape
    glBegin(GL_TRIANGLES);
	    //front
	    glVertex3f(1.0f, 0.0f, 0.0f);
        //left wing
	    glVertex3f(-0.5f, 0.0f, 0.6f);
	    //right wing
	    glVertex3f(-0.5f, 0.0f, -0.6f);

	glEnd();
    
        //top face
    glBegin(GL_TRIANGLES);
	    glVertex3f(1.0f, 0.2f, 0.0f);
	    glVertex3f(-0.5f, 0.2f, -0.6f);
	    glVertex3f(-0.5f, 0.2f, 0.6f);
        glEnd();
		glPopMatrix();

	//--------------------------------------------	
    mCounter++;
    //--------------------------------------------	
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Game::NormalKeys(unsigned char key, int state){
	if (key >= SDLK_0 && key <= SDLK_9){}
	if (key == SDLK_RETURN){}//Return
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Game::SpecialKeys(int key, int state){
    if (key == SDLK_LEFT){}
    if (key == SDLK_RIGHT){}
    if (key == SDLK_UP){}
    if (key == SDLK_DOWN){}
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Game::Mouse(int button, int state, int x, int y){
	mMouseButton = button;//SDL_BUTTON_LEFT/SDL_BUTTON_MIDDLE/SDL_BUTTON_RIGHT
	mMouseState = state;//SDL_PRESSED/SDL_RELEASED
	mMouseX = x; mMouseY = y;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Game::MouseMotion(int x, int y){
    mMouseMotionX = x; mMouseMotionY = y;
};
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Game::Update(float dt) { // GAME UPDATE FULL OGRENILCEK.
    deltatime = dt;
	if (dt <= 0.0f || dt > 0.1f) return; // skip bad frames


	Player& p = players[0];
	if (!p.alive) return;

    //rotation
    if (keys['a'] || keys [SDLK_LEFT & 0xFF])
		p.rotation += PLAYER_ROTATION_SPEED * dt;
    if (keys['d'] || keys[SDLK_RIGHT & 0xFF])
        p.rotation -= PLAYER_ROTATION_SPEED * dt;

	// direction from rotation (rotation 0 = facing -x)
	float rad = p.rotation * M_PI / 180.0f;
	float dirX = cosf(rad);
	float dirZ = -sinf(rad);

    //thrust 
    if (keys['w'] || keys[SDLK_UP & 0xFF]){
        p.vx += dirX * PLAYER_ACCELERATION * dt;
        p.vz += dirZ * PLAYER_ACCELERATION * dt;
	}

    //drag
	p.vx -= p.vx * PLAYER_DRAG * dt;
    p.vz -= p.vz * PLAYER_DRAG * dt;
	
    //speed clamp
	float speed = sqrtf(p.vx * p.vx + p.vz * p.vz);

    //move
	p.x += p.vx * dt;
    p.z += p.vz * dt;

	//wrap around
	if (p.x > WORLD_SIZE) p.x -= 2 * WORLD_SIZE;
	if (p.x < -WORLD_SIZE) p.x += 2 * WORLD_SIZE;
	if (p.z > WORLD_SIZE) p.z -= 2 * WORLD_SIZE;
	if (p.z < -WORLD_SIZE) p.z += 2 * WORLD_SIZE;

}
