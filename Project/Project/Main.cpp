//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include "Game.h"
Game G;

//----------------------------------
SDL_Window * gScreen;
//----------------------------------

//---------------------------------------------------------------------
//---------------------------------------------------------------------
static void initAttributes(){
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,    8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,  8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,  8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);//Depth Buffer 24-bit
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,4);
    SDL_GL_SetSwapInterval(1);//Enable Vsync
    //----------------------------------
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
static void createSurface() {
    SDL_Init(SDL_INIT_EVERYTHING);
    initAttributes();
    gScreen = SDL_CreateWindow("GFX",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        0, 0,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_GL_CreateContext(gScreen);
    int fw, fh;
    SDL_GetWindowSize(gScreen, &fw, &fh);
    G.mW = (float)fw;
    G.mH = (float)fh;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
static void mainLoop (){
    SDL_Event event;
    bool quit = false;
    int key, state;

    Uint32 LastTime = SDL_GetTicks();
    float deltatime = 0.0f;

    while (!quit) {
        
        Uint32 CurrentTime = SDL_GetTicks();
        deltatime = (CurrentTime - LastTime) / 1000.0f;
        LastTime = CurrentTime;

		while (SDL_PollEvent(&event)) {

			

			switch (event.type) {
					//Mouse
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					G.Mouse((int)event.button.button,
							(int)event.button.state,
							(int)event.button.x,
							(int)event.button.y);
                    break;
                case SDL_MOUSEMOTION:
                    G.MouseMotion((int)event.button.x,
                                   (int)event.button.y);
                    break;
					//Keys
				case SDL_KEYDOWN:
                    key   = event.key.keysym.sym;
                    state = SDL_GetModState();
					//---------------------------------

                    if (key == SDLK_ESCAPE) {

                        if (G.currentState == STATE_MAIN_MENU) quit = true;
                        else G.SetState(STATE_MAIN_MENU);
                        break;
                    }

                    if (G.currentState == STATE_NICKNAME) {
                        if (key == SDLK_BACKSPACE && G.nicknameLen > 0) {
                            G.nicknameLen--;
                            G.nickname[G.nicknameLen] = '\0';
                        }
                        if ((key == SDLK_RETURN || key == SDLK_KP_ENTER) && G.nicknameLen > 0) {
                            G.enteringNickname = false;
                            strcpy_s(G.player1Name, G.nickname);
                            G.SetState(STATE_LOBBY);
                        }
                        break;
                    }
                    if (G.currentState == STATE_LOBBY) {
                        if (key == SDLK_BACKSPACE && G.lobbyChoice == 1 && G.lobbyCodeLen > 0) {
                            G.lobbyCodeLen--;
                            G.lobbyCodeInput[G.lobbyCodeLen] = '\0';
                        }
                        if (key == SDLK_UP || key == SDLK_DOWN) {
                            G.lobbyChoice = 1 - G.lobbyChoice;
                            if (G.lobbyChoice == 1) {
                                G.isHost = false;
                                strcpy_s(G.lobbyCode, "");
                                G.joinActive = true;
                            }
                            else {
                                G.joinActive = false;
                            }
                        }
                        if ((key == SDLK_RETURN || key == SDLK_KP_ENTER)) {
                            if (G.lobbyChoice == 0 && G.lobbyCode[0] == '\0') {

                                G.joinActive = false;
                                G.isHost = true;
                                sprintf_s(G.lobbyCode, "%c%c%c%c%c%c",
                                    'A' + rand() % 26, 'A' + rand() % 26, 'A' + rand() % 26,
                                    '0' + rand() % 10, '0' + rand() % 10, '0' + rand() % 10);
                                G.lobbyReady = false;
								// TODO: network starting code will go here, and set lobbyReady to true when the opponent has connected
                            }
                            else {
                                if (G.lobbyCodeLen == 6) {
                                    G.isHost = false;
									// TODO: network joining code will go here, using G.lobbyCodeInput as the code to join, 
                                    // and set lobbyReady to true when successfully joined
                                }
                            }
                        }
                        break;
                    }

                    if (key < 256) G.keys[key] = true;
                    G.SpecialKeys(key, state);
                    if (key < 128) G.NormalKeys(key, state);

					break;

                case SDL_TEXTINPUT:
                    if (G.currentState == STATE_NICKNAME && G.enteringNickname) {
                        char c = event.text.text[0];
                        if (G.nicknameLen < 15 && c >= 32 && c < 127) {
                            G.nickname[G.nicknameLen++] = c;
                            G.nickname[G.nicknameLen] = '\0';
                        }
                    }
                    else if (G.currentState == STATE_LOBBY && G.lobbyChoice == 1) {
                        char c = event.text.text[0];
                        if (G.lobbyCodeLen < 6 && ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) {
                            if (c >= 'a' && c <= 'z') c -= 32;
                            G.lobbyCodeInput[G.lobbyCodeLen++] = c;
                            G.lobbyCodeInput[G.lobbyCodeLen] = '\0';
                        }
                    }
                    break;

			    case SDL_KEYUP:

                    key   = event.key.keysym.sym;
                    if (key < 256) G.keys[key] = false;

                    break;

				case SDL_QUIT: quit = true; break;
				default: break;
			}
        }
        
		G.Update(deltatime);
		G.Draw();
        SDL_GL_SwapWindow(gScreen);
        SDL_Delay(8);
        //----------------------------------
	}
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
int main(int argc, char* argv[]) {
    createSurface();
    G.InitGFX();
    G.ChangeSize((int)G.mW, (int)G.mH);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    mainLoop();
    return 0;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------



