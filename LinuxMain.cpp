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
#include <iostream>
#include <unistd.h> 

BOOL _runDXThread = TRUE;

int main(int argc, char** argv)
{
    std::cout << "Starting WinTex Linux Port..." << std::endl;

    BOOL uakm = TRUE; // Default to UAKM for now
    isUAKM = TRUE;
    pConfig = new CConfiguration(L"Under A Killing Moon");
    
    // Init DX Stub
    // Assuming dx.Init returns BOOL and arguments are (HWND, int, int, BOOL, BOOL) based on WinTex.cpp usage
    dx.Init(NULL, 1920, 1080, TRUE, FALSE); 
    CDXControls::Init();
    
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
        
        // Simple Game Loop
        while (_runDXThread)
        {
            CGameController::Tick(16);
            // Render would go here
            // sleep(1); // very slow tick for testing
            break; // Run once and exit for build verification
        }
        
        if (pGame) delete pGame;
    }
    
    std::cout << "Exiting..." << std::endl;
    return 0;
}

#endif
