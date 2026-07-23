#pragma once

#include <list>
#include <string>
#include <cstdint>
#include "Utilities.h"
#include "Enums.h"

#define UM_GAMEPAD (0x0400 + 0x2000)

class CControllerData
{
public:
    InputSource Source;
    uint32_t Offset;
    uint32_t Type;
    uint32_t Data;
};

class CGamepadController
{
public:
    CGamepadController(void* hWnd);
    ~CGamepadController();

    static void Init(void* hWnd);
    static void Dispose();

    void Update();

    static CGamepadController* GamepadController;

    std::string GetName(int offset, int data);

private:
    void* _hWnd;
};