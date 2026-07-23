#include "LZ.h"
#include "File.h"
#include "Utilities.h"
#include "PTF.h"
#include "DirectX.h"
#include "DXText.h"
#include "Globals.h"
#include "UAKMGame.h"
#include "PDGame.h"
#include "DXScreen.h"
#include "DXShader.h"
#include "Configuration.h"
#include "AnimationController.h"
#include "GameController.h"
#include "DXControls.h"
#include "MIDIPlayer.h"
#include "PDMIDIPlayer.h"
#include "ModuleController.h"
#include "ConstantBuffers.h"
#include "InputMapping.h"
#include "Gamepad.h"
#include <iostream>
#include <string>
#include <vector>
#include <SDL2/SDL.h>

#include "VideoModule.h"

#define VK_BACK       0x08
#define VK_TAB        0x09
#define VK_RETURN     0x0D
#define VK_ESCAPE     0x1B
#define VK_SPACE      0x20
#define VK_PRIOR      0x21
#define VK_NEXT       0x22
#define VK_END        0x23
#define VK_HOME       0x24
#define VK_LEFT       0x25
#define VK_UP         0x26
#define VK_RIGHT      0x27
#define VK_DOWN       0x28
#define VK_INSERT     0x2D
#define VK_DELETE     0x2E
#define VK_SHIFT      0x10
#define VK_CONTROL    0x11
#define VK_MENU       0x12
#define VK_F1         0x70
#define VK_F2         0x71
#define VK_F3         0x72
#define VK_F4         0x73
#define VK_F5         0x74
#define VK_F6         0x75
#define VK_F7         0x76
#define VK_F8         0x77
#define VK_F9         0x78
#define VK_F10        0x79
#define VK_F11        0x7A
#define VK_F12        0x7B

bool _runDXThread = true;
std::vector<SDL_GameController*> _gamepads;

int MapSDLKeyToVK(SDL_Keycode sym) {
    if (sym >= SDLK_a && sym <= SDLK_z) return 'A' + (sym - SDLK_a);
    if (sym >= SDLK_0 && sym <= SDLK_9) return '0' + (sym - SDLK_0);
    if (sym == SDLK_UP) return VK_UP;
    if (sym == SDLK_DOWN) return VK_DOWN;
    if (sym == SDLK_LEFT) return VK_LEFT;
    if (sym == SDLK_RIGHT) return VK_RIGHT;
    if (sym == SDLK_SPACE) return VK_SPACE;
    if (sym == SDLK_RETURN) return VK_RETURN;
    if (sym == SDLK_ESCAPE) return VK_ESCAPE;
    if (sym == SDLK_BACKSPACE) return VK_BACK;
    if (sym == SDLK_TAB) return VK_TAB;
    if (sym == SDLK_LSHIFT || sym == SDLK_RSHIFT) return VK_SHIFT;
    if (sym == SDLK_LCTRL || sym == SDLK_RCTRL) return VK_CONTROL;
    if (sym == SDLK_LALT || sym == SDLK_RALT) return VK_MENU;
    if (sym == SDLK_INSERT) return VK_INSERT;
    if (sym == SDLK_DELETE) return VK_DELETE;
    if (sym == SDLK_HOME) return VK_HOME;
    if (sym == SDLK_END) return VK_END;
    if (sym == SDLK_PAGEUP) return VK_PRIOR;
    if (sym == SDLK_PAGEDOWN) return VK_NEXT;
    if (sym >= SDLK_F1 && sym <= SDLK_F12) return VK_F1 + (sym - SDLK_F1);
    return 0;
}

int MapSDLScancodeToWinScan(SDL_Scancode sc) {
    switch (sc) {
        case SDL_SCANCODE_W: return 0x11;
        case SDL_SCANCODE_A: return 0x1E;
        case SDL_SCANCODE_S: return 0x1F;
        case SDL_SCANCODE_D: return 0x20;
        case SDL_SCANCODE_Q: return 0x10;
        case SDL_SCANCODE_E: return 0x12;
        case SDL_SCANCODE_R: return 0x13;
        case SDL_SCANCODE_T: return 0x14;
        case SDL_SCANCODE_Y: return 0x15;
        case SDL_SCANCODE_U: return 0x16;
        case SDL_SCANCODE_I: return 0x17;
        case SDL_SCANCODE_O: return 0x18;
        case SDL_SCANCODE_P: return 0x19;
        case SDL_SCANCODE_F: return 0x21;
        case SDL_SCANCODE_G: return 0x22;
        case SDL_SCANCODE_H: return 0x23;
        case SDL_SCANCODE_J: return 0x24;
        case SDL_SCANCODE_K: return 0x25;
        case SDL_SCANCODE_L: return 0x26;
        case SDL_SCANCODE_Z: return 0x2C;
        case SDL_SCANCODE_X: return 0x2D;
        case SDL_SCANCODE_C: return 0x2E;
        case SDL_SCANCODE_V: return 0x2F;
        case SDL_SCANCODE_B: return 0x30;
        case SDL_SCANCODE_N: return 0x31;
        case SDL_SCANCODE_M: return 0x32;
        case SDL_SCANCODE_LSHIFT: return 0x2A;
        case SDL_SCANCODE_RSHIFT: return 0x36;
        case SDL_SCANCODE_LCTRL: return 0x1D;
        case SDL_SCANCODE_RCTRL: return 0x1D; // Windows maps both to 1D but with extended bit for right
        case SDL_SCANCODE_LALT: return 0x38;
        case SDL_SCANCODE_RALT: return 0x38;
        case SDL_SCANCODE_ESCAPE: return 0x01;
        case SDL_SCANCODE_RETURN: return 0x1C;
        case SDL_SCANCODE_SPACE: return 0x39;
        case SDL_SCANCODE_UP: return 0x48;
        case SDL_SCANCODE_DOWN: return 0x50;
        case SDL_SCANCODE_LEFT: return 0x4B;
        case SDL_SCANCODE_RIGHT: return 0x4D;
        case SDL_SCANCODE_TAB: return 0x0F;
        case SDL_SCANCODE_BACKSPACE: return 0x0E;
        case SDL_SCANCODE_INSERT: return 0x52;
        case SDL_SCANCODE_DELETE: return 0x53;
        case SDL_SCANCODE_HOME: return 0x47;
        case SDL_SCANCODE_END: return 0x4F;
        case SDL_SCANCODE_PAGEUP: return 0x49;
        case SDL_SCANCODE_PAGEDOWN: return 0x51;
        default: return 0;
    }
}

void Dispose()
{
	for (auto pad : _gamepads) {
        SDL_GameControllerClose(pad);
    }
    _gamepads.clear();
	CGamepadController::Dispose();
	CDXControls::Dispose();
	CConstantBuffers::Dispose();
	CShaders::Dispose();
	dx.Dispose();
}


int main(int argc, char** argv)
{
    if (argc > 1) {
        gamePath = argv[1];
    }

    bool uakm = false, pd = false;
    std::string windowTitle = "";

    if (CFile::Exists("TEX3.EXE")) {
        uakm = true;
        isUAKM = true;
        windowTitle = "Tex Murphy: Under a Killing Moon";
        pConfig = new CConfiguration("Under A Killing Moon");
    }
    else if (CFile::Exists("TEX4.EXE")) {
        pd = true;
        isUAKM = false;
        windowTitle = "Tex Murphy: The Pandora Directive";
        pConfig = new CConfiguration("The Pandora Directive");
    }
    else {
        std::cerr << "Original game executables not found!" << std::endl;
        return 1;
    }

    if (!dx.Init(nullptr, pConfig->Width, pConfig->Height, !pConfig->FullScreen, pConfig->AnisotropicFilter)) {
        return 1;
    }
    SDL_SetWindowTitle((SDL_Window*)_hWnd, windowTitle.c_str());
    SDL_ShowCursor(SDL_DISABLE);

    CDXControls::Init();
    CConstantBuffers::Setup2D(dx);

    if (uakm) pMIDI = new CMIDIPlayer();
    else pMIDI = new CPDMIDIPlayer();

    CGameController::Init();
    CAnimationController::Init();
    CGamepadController::Init(nullptr);

    CGameBase* pGame = nullptr;
    if (uakm) pGame = new CUAKMGame();
    else if (pd) pGame = new CPDGame();

	if (pGame)
    {
        CGameController::StartGame(pGame);
        
        CInputMapping::LoadControlsMap();
        CModuleController::MainThreadId = 1; // Dummy ID for Linux

        // Game Loop
        Uint32 lastTime = SDL_GetTicks();
        SDL_Event event;

        while (_runDXThread)
        {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    _runDXThread = false;
                }
                else if (event.type == SDL_WINDOWEVENT) {
                    if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                        CModuleController::GotFocus();
                    }
                    else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                        CModuleController::LostFocus();
                    }
                }
                else if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                         // _runDXThread = FALSE; // Don't quit on ESC, let the game handle it
                    } 
                    
                    int vk = MapSDLKeyToVK(event.key.keysym.sym);
                    int scan = MapSDLScancodeToWinScan(event.key.keysym.scancode);
                    // Construct lParam like Windows: 
                    // 0-15: Repeat count (1)
                    // 16-23: Scan code
                    // 24: Extended key
                    // 29: Context code
                    // 30: Previous key state
                    // 31: Transition state
                    uint32_t lParam = (scan << 16) | 1; 
                    if (vk != 0) CModuleController::KeyDown(vk, lParam);
                }
                else if (event.type == SDL_KEYUP) {
                    int vk = MapSDLKeyToVK(event.key.keysym.sym);
                    int scan = MapSDLScancodeToWinScan(event.key.keysym.scancode);
                    uint32_t lParam = (scan << 16) | 0xC0000001; // Transition state 1, Previous state 1
                    if (vk != 0) CModuleController::KeyUp(vk, lParam);
                }
                else if (event.type == SDL_MOUSEMOTION) {
                    Point2D pt;
                    if (SDL_GetRelativeMouseMode()) {
                        pt.x = (dx.GetWidth() / 2) + event.motion.xrel;
                        pt.y = (dx.GetHeight() / 2) + event.motion.yrel;
                    } else {
                        pt.x = event.motion.x;
                        pt.y = event.motion.y;
                    }
                    CModuleController::MouseMove(pt);
                }
                else if (event.type == SDL_MOUSEBUTTONDOWN) {
                    Point2D pt;
                    pt.x = event.button.x;
                    pt.y = event.button.y;
                    int btn = (event.button.button == SDL_BUTTON_LEFT) ? -1 : (event.button.button == SDL_BUTTON_MIDDLE) ? 0 : 1;
                    CModuleController::MouseDown(pt, btn);
                }
                else if (event.type == SDL_MOUSEBUTTONUP) {
                    Point2D pt;
                    pt.x = event.button.x;
                    pt.y = event.button.y;
                    int btn = (event.button.button == SDL_BUTTON_LEFT) ? -1 : (event.button.button == SDL_BUTTON_MIDDLE) ? 0 : 1;
                    CModuleController::MouseUp(pt, btn);
                }
                else if (event.type == SDL_MOUSEWHEEL) {
                    // Windows WM_MOUSEWHEEL uses increments of 120 (WHEEL_DELTA)
                    // SDL uses steps.
                    CModuleController::MouseWheel(event.wheel.y * 120);
                }
                else if (event.type == SDL_CONTROLLERDEVICEADDED) {
                    SDL_GameController* pad = SDL_GameControllerOpen(event.cdevice.which);
                    if (pad) {
                        _gamepads.push_back(pad);
                        std::cout << "Gamepad added: " << SDL_GameControllerName(pad) << std::endl;
                    }
                }
                else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
                    SDL_GameController* pad = SDL_GameControllerFromInstanceID(event.cdevice.which);
                    if (pad) {
                        SDL_GameControllerClose(pad);
                        for (auto it = _gamepads.begin(); it != _gamepads.end(); ++it) {
                            if (*it == pad) {
                                _gamepads.erase(it);
                                break;
                            }
                        }
                         std::cout << "Gamepad removed" << std::endl;
                    }
                }
                else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
                    InputSource source = InputSource::JoystickButton;
                    int offset = 48 + event.cbutton.button; // Map SDL buttons starting at DI offset 48
                    int data = (event.type == SDL_CONTROLLERBUTTONDOWN) ? 0x80 : 0x00;
                    if (data & 0x80) data = (data & 0x7f) | 0x80000000; // Match Windows behavior for button press
                    
                    CModuleController::GamepadInput(source, offset, data);
                }
                else if (event.type == SDL_CONTROLLERAXISMOTION) {
                    InputSource source = InputSource::JoystickAxis;
                    int offset = -1;
                    switch (event.caxis.axis) {
                        case SDL_CONTROLLER_AXIS_LEFTX: offset = 0; break;
                        case SDL_CONTROLLER_AXIS_LEFTY: offset = 4; break;
                        case SDL_CONTROLLER_AXIS_RIGHTX: offset = 12; break; // lRx
                        case SDL_CONTROLLER_AXIS_RIGHTY: offset = 16; break; // lRy
                        case SDL_CONTROLLER_AXIS_TRIGGERLEFT: offset = 8; break; // lZ
                        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: offset = 20; break; // lRz
                    }
                    
                    if (offset != -1) {
                         // Scale -32768..32767 to -1000..1000
                         int val = (int)(event.caxis.value * 1000.0f / 32767.0f);
                         CModuleController::GamepadInput(source, offset, val);
                    }
                }
            }

            Uint32 currentTime = SDL_GetTicks();
            Uint32 deltaTime = currentTime - lastTime;
            lastTime = currentTime;

            CGameController::Tick(deltaTime);
			CGamepadController::GamepadController->Update();
            CModuleController::Render();
            
            // Small sleep to prevent 100% CPU usage
            SDL_Delay(1);
        }
        CAnimationController::Clear();
        if (pGame) delete pGame;
    }
    
    Dispose();
    return 0;
}
