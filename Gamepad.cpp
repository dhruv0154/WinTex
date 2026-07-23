#include "Gamepad.h"

// Linux Stub
CGamepadController* CGamepadController::GamepadController = nullptr;

CGamepadController::CGamepadController(void* hWnd) { _hWnd = hWnd; }
CGamepadController::~CGamepadController() {}
void CGamepadController::Init(void* hWnd) { GamepadController = new CGamepadController(hWnd); }
void CGamepadController::Dispose() { delete GamepadController; }
void CGamepadController::Update() {}
std::string CGamepadController::GetName(int offset, int data) { return "Unknown"; }