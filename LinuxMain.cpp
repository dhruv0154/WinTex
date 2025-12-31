#include "Platform.h"

#ifdef PLATFORM_LINUX

#include "Globals.h"
#include "Configuration.h"
#include "GameController.h"
#include "AnimationController.h"
#include "UAKMGame.h"
#include "PDGame.h"
#include "DXScreen.h"
#include "DXControls.h"
#include "DXSound.h"
#include "MIDIPlayer.h"
#include "PDMIDIPlayer.h"
#include "ModuleController.h"
#include "ConstantBuffers.h"
#include <iostream>
#include <unistd.h> 
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

BOOL _runDXThread = TRUE;

int main(int argc, char** argv)
{
    std::cout << "Starting WinTex Linux Port..." << std::endl;

    BOOL uakm = TRUE; // Default to UAKM for now
    isUAKM = TRUE;
    
    std::cout << "Initializing Configuration..." << std::endl;
    pConfig = new CConfiguration(L"Under A Killing Moon");
    
    std::cout << "Initializing DX..." << std::endl;
    // Init DX
    // Force windowed mode for now
    if (!dx.Init(NULL, 1280, 720, TRUE, FALSE)) {
        std::cerr << "Failed to init DX" << std::endl;
        return 1;
    }
    
    std::cout << "Initializing DXControls..." << std::endl;
    CDXControls::Init();
    
    // Initial 2D Setup
    std::cout << "Setting up 2D matrices..." << std::endl;
    CConstantBuffers::Setup2D(dx);

    std::cout << "Initializing MIDI..." << std::endl;
    
    if (uakm)
        pMIDI = new CMIDIPlayer();
    else
        pMIDI = new CPDMIDIPlayer();

    CGameController::Init();
    CAnimationController::Init();
    
    // CGamepadController::Init(_hWnd); // Skipping for now

    CGameBase* pGame = NULL;
    if (uakm)
        pGame = new CUAKMGame();

    if (pGame)
    {
        CGameController::StartGame(pGame);
        
        // CInputMapping::LoadControlsMap(); // Skipping for now
        
        // Game Loop
        Uint32 lastTime = SDL_GetTicks();
        SDL_Event event;

        std::cout << "Entering Game Loop..." << std::endl;
        int frameCount = 0;

        while (_runDXThread)
        {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    std::cout << "SDL_QUIT received!" << std::endl;
                    _runDXThread = FALSE;
                }
                else if (event.type == SDL_KEYDOWN) {
                    // Simple key handling for testing
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                         _runDXThread = FALSE;
                    }
                    CModuleController::KeyDown(event.key.keysym.sym, 0); // TODO: proper mapping
                }
                else if (event.type == SDL_MOUSEMOTION) {
                    POINT pt;
                    pt.x = event.motion.x;
                    pt.y = event.motion.y;
                    CModuleController::MouseMove(pt);
                }
                else if (event.type == SDL_MOUSEBUTTONDOWN) {
                    POINT pt;
                    pt.x = event.button.x;
                    pt.y = event.button.y;
                    CModuleController::MouseDown(pt, event.button.button);
                }
                else if (event.type == SDL_MOUSEBUTTONUP) {
                    POINT pt;
                    pt.x = event.button.x;
                    pt.y = event.button.y;
                    CModuleController::MouseUp(pt, event.button.button);
                }
            }

            Uint32 currentTime = SDL_GetTicks();
            Uint32 deltaTime = currentTime - lastTime;
            lastTime = currentTime;

            CGameController::Tick(deltaTime);
            CModuleController::Render();
            
            if (frameCount++ % 10 == 0) {
                 unsigned char pixel[4];
                 glReadPixels(dx.GetWidth() / 2, dx.GetHeight() / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
                 std::cout << "Frame " << frameCount << " Size: " << dx.GetWidth() << "x" << dx.GetHeight() 
                           << " Center Pixel: R=" << (int)pixel[0] << " G=" << (int)pixel[1] 
                           << " B=" << (int)pixel[2] << " A=" << (int)pixel[3] << std::endl;
            }
            
            // Small sleep to prevent 100% CPU usage if vsync is off or not working
            SDL_Delay(1);
        }
        
        std::cout << "Exited Game Loop. _runDXThread=" << _runDXThread << std::endl;

        if (pGame) delete pGame;
    }
    
    dx.Dispose();
    
    std::cout << "Exiting..." << std::endl;
    return 0;
}

#endif
