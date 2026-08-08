#pragma once

#include "ModuleBase.h"
#include <list>
#include <cstdint>
#include "AnimatedCursor.h"
#include "Map.h"
#include "Point.h"
#include "Enums.h"

class CControllerData;

class CModuleController
{
public:
    static bool Init(CMap* map, CMap* dmap);

    static void GotFocus();
    static void LostFocus();

    static void Push(CModuleBase* pModule, bool overrideCurrent = false);
    static void Pop(CModuleBase* pModule);
    static void SendToBack(CModuleBase* pModule);
    static void SendToFront(CModuleBase* pModule);

    static CModuleBase* Get(ModuleType type);
    static void ClearExcept(CModuleBase* pModule);

    static void Render();
    
    static void MouseMove(Point pt);
    static void MouseDown(Point pt, int btn);
    static void MouseUp(Point pt, int btn);
    static void MouseWheel(int scroll);
    static void KeyDown(uint32_t key, uint32_t lParam);
    static void KeyUp(uint32_t key, uint32_t lParam);
    static void GamepadInput(InputSource source, int offset, int data);

    static void Resize(int width, int height);

    static CAnimatedCursor Cursors[13];

    static uint32_t MainThreadId;
    static uint32_t D3DThreadId;
    static uint32_t TimerThreadId;

    static CModuleBase* CurrentModule;
    static CModuleBase* NextModule;

    static CMap* pMap;
    static CMap* pDMap;

    static void Resume();

protected:
    static std::list<CModuleBase*> Modules;

    static void PauseModule(CModuleBase* pModule);
    static void ResumeModule(CModuleBase* pModule);
};