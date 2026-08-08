#pragma once

#include "Map.h"
#include "InputMapping.h"
#include "Enums.h"
#include "DXBase.h"
#include <cstdint>
#include <string>

class ID3D11Buffer;
struct float4;

class CControllerData;

class CModuleBase {
public:
    CModuleBase(ModuleType type);
    virtual ~CModuleBase() {};

    ModuleType Type;

    void Init();
    virtual void Resize(int width, int height) = 0;
    virtual void GotFocus();
    virtual void LostFocus();
    virtual void Pause() {}
    virtual void Resume() {}
    virtual void Dispose() {}
    virtual void Render() {}
    virtual void MouseMove(Point pt) {}
    virtual void MouseDown(Point pt, int btn) {}
    virtual void MouseUp(Point pt, int btn) {}
    virtual void MouseWheel(int scroll) {}
    virtual void KeyDown(uint32_t key, uint32_t lParam) {}
    virtual void KeyUp(uint32_t key, uint32_t lParam) {}

    virtual void GamepadInput(InputSource source, int offset, int data) {}

    // Input related
    virtual void CheckInput();
    virtual void Cursor(float x, float y, bool relative);
    virtual void BeginAction() {}
    virtual void EndAction() {}
    virtual void Back() {}
    virtual void Cycle() {}
    virtual void MoveForward(float v) {}
    virtual void MoveBack(float v) {}
    virtual void MoveLeft(float v) {}
    virtual void MoveRight(float v) {}
    virtual void MoveUp(float y) {}
    virtual void MoveDown(float y) {}
    virtual void Run(bool run) {}
    virtual void Next() {}
    virtual void Prev() {}
    virtual void Inventory() {}
    virtual void Travel() {}
    virtual void Hints() {}
    virtual void SetCursorClipping();
    virtual void SetCursorClipping(int x1, int y1, int x2, int y2);
    virtual void RefreshCursorClipping();
    virtual void UnsetCursorClipping();

    float _cursorPosX = 0.0f;
    float _cursorPosY = 0.0f;

protected:
    virtual void Initialize() {}

    void CenterMouse();

    void CreateTexturedRectangle(float top, float left, float bottom, float right, ID3D11Buffer** ppBuffer, const char* pName);
    void CreateColouredRectangle(float top, float left, float bottom, float right, float4 colour, ID3D11Buffer** ppBuffer, const char* pName);

    int _cursorMinX;
    int _cursorMaxX;
    int _cursorMinY;
    int _cursorMaxY;

    bool _initialized;

    Rect _oldClippingArea;
    Rect _currentClippingArea;
    bool _cursorIsClipped;
    bool _hasFocus{ true };
};