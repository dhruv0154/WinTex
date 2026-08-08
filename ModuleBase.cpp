#include "ModuleBase.h"
#include "Globals.h"
#include "DirectX.h"
#include "ShaderStructs.h"
#include "Gamepad.h"
#include "Utilities.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>

CModuleBase::CModuleBase(ModuleType type)
{
    _cursorMinX = 0;
    _cursorMaxX = dx.GetWidth() - 1;
    _cursorMinY = 0;
    _cursorMaxY = dx.GetHeight() - 1;

    Type = type;
    _initialized = false;
    _cursorIsClipped = false;
    _hasFocus = true;
}

void CModuleBase::SetCursorClipping()
{
    int w = 0, h = 0;
    SDL_Window* pWindow = static_cast<SDL_Window*>(_hWnd);
    if (pWindow)
    {
        SDL_GetWindowSize(pWindow, &w, &h);
    }
    else
    {
        w = dx.GetWidth();
        h = dx.GetHeight();
    }
    SetCursorClipping(0, 0, w, h);
}

void CModuleBase::SetCursorClipping(int x1, int y1, int x2, int y2)
{
    if (!_cursorIsClipped) {
        _oldClippingArea = { 0, 0, dx.GetWidth(), dx.GetHeight() };
    }

    _currentClippingArea = { x1, y1, x2, y2 };

    SDL_Window* pWindow = static_cast<SDL_Window*>(_hWnd);
    if (pWindow)
    {
        SDL_Rect sdlRect{ x1, y1, x2 - x1, y2 - y1 };
        SDL_SetWindowMouseRect(pWindow, &sdlRect);
    }

    _cursorIsClipped = true;
}

void CModuleBase::RefreshCursorClipping()
{
    if (_cursorIsClipped) {
        SDL_Window* pWindow = static_cast<SDL_Window*>(_hWnd);
        if (pWindow)
        {
            SDL_Rect sdlRect{ _currentClippingArea.Left, _currentClippingArea.Top, 
                              _currentClippingArea.Right - _currentClippingArea.Left, 
                              _currentClippingArea.Bottom - _currentClippingArea.Top };
            SDL_SetWindowMouseRect(pWindow, &sdlRect);
        }
    }
}

void CModuleBase::UnsetCursorClipping()
{
    if (_cursorIsClipped) {
        SDL_Window* pWindow = static_cast<SDL_Window*>(_hWnd);
        if (pWindow)
        {
            SDL_SetWindowMouseRect(pWindow, nullptr);
        }
        _cursorIsClipped = false;
    }
}

void CModuleBase::CenterMouse()
{
    SDL_Window* pWindow = static_cast<SDL_Window*>(_hWnd);
    if (pWindow && (SDL_GetWindowFlags(pWindow) & (SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS)))
    {
        int cx = dx.GetWidth() / 2;
        int cy = dx.GetHeight() / 2;
        SDL_WarpMouseInWindow(pWindow, cx, cy);

        CInputMapping::IgnoreNextMouseInput = true;
    }
}

void CModuleBase::Init()
{
    if (!_initialized)
    {
        _initialized = true;
        Initialize();
    }
}

void CModuleBase::GotFocus()
{
    SDL_ShowCursor(SDL_DISABLE);
    _hasFocus = true;
    RefreshCursorClipping();
}

void CModuleBase::LostFocus()
{
    SDL_ShowCursor(SDL_ENABLE);
    _hasFocus = false;
    UnsetCursorClipping();
}

void CModuleBase::CreateTexturedRectangle(float top, float left, float bottom, float right, ID3D11Buffer** ppBuffer, const char* pName)
{
    TEXTURED_VERTEX* vertices = new TEXTURED_VERTEX[4];
    if (vertices != nullptr)
    {
        vertices[0].position = float3(right, top, -1.0f);
        vertices[0].texture = float2(1.0f, 0.0f);
        vertices[0].object = float2(0.0f, 0.0f);
        vertices[0].objectParameters = float4(1.0f, 1.0f, 1.0f, 1.0f);

        vertices[1].position = float3(right, bottom, -1.0f);
        vertices[1].texture = float2(1.0f, 1.0f);
        vertices[1].object = float2(0.0f, 0.0f);
        vertices[1].objectParameters = float4(1.0f, 1.0f, 1.0f, 1.0f);

        vertices[2].position = float3(left, top, -1.0f);
        vertices[2].texture = float2(0.0f, 0.0f);
        vertices[2].object = float2(0.0f, 0.0f);
        vertices[2].objectParameters = float4(1.0f, 1.0f, 1.0f, 1.0f);

        vertices[3].position = float3(left, bottom, -1.0f);
        vertices[3].texture = float2(0.0f, 1.0f);
        vertices[3].object = float2(0.0f, 0.0f);
        vertices[3].objectParameters = float4(1.0f, 1.0f, 1.0f, 1.0f);

        D3D11_BUFFER_DESC vertexBufferDesc = {};
        vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        vertexBufferDesc.ByteWidth = sizeof(TEXTURED_VERTEX) * 4;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA vertexData = {};
        vertexData.pSysMem = vertices;

        dx.CreateBuffer(&vertexBufferDesc, &vertexData, ppBuffer, pName);
        delete[] vertices;
    }
}

void CModuleBase::CreateColouredRectangle(float top, float left, float bottom, float right, float4 colour, ID3D11Buffer** ppBuffer, const char* pName)
{
    COLOURED_VERTEX_ORTHO* vertices = new COLOURED_VERTEX_ORTHO[4];
    if (vertices != nullptr)
    {
        vertices[0].position = float4(right, top, 0.0f, 1.0f);
        vertices[0].colour = colour;

        vertices[1].position = float4(right, bottom, 0.0f, 1.0f);
        vertices[1].colour = colour;

        vertices[2].position = float4(left, top, 0.0f, 1.0f);
        vertices[2].colour = colour;

        vertices[3].position = float4(left, bottom, 0.0f, 1.0f);
        vertices[3].colour = colour;

        D3D11_BUFFER_DESC vertexBufferDesc = {};
        vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        vertexBufferDesc.ByteWidth = sizeof(COLOURED_VERTEX_ORTHO) * 4;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA vertexData = {};
        vertexData.pSysMem = vertices;

        dx.CreateBuffer(&vertexBufferDesc, &vertexData, ppBuffer, pName);
        delete[] vertices;
    }
}

double Gain(double x)
{
    // Assuming a 10% dead zone
    x = std::max(0.0, x - 0.1) / 0.9;
    double a = 20.0;
    double y = (std::pow(a, x) - 1.0) / (a - 1.0);
    return y * 1.0;
}

void CModuleBase::CheckInput()
{
    for (auto& action : CInputMapping::ControlsMap)
    {
        if (action.second.IsActive)
        {
            switch (action.first)
            {
                case InputAction::Cursor:
                {
                    int x = (action.second.CurrentData >> 16) & 0xffff;
                    if (x & 0x8000) x |= ~0xffff;
                    int y = action.second.CurrentData & 0xffff;
                    if (y & 0x8000) y |= ~0xffff;

                    float fx = static_cast<float>(x);
                    float fy = static_cast<float>(y);

                    if (action.second.CurrentSource == InputSource::JoystickAxis)
                    {
                        fx = ((fx < 0) ? static_cast<float>(-Gain(-fx / 1000.0f)) : static_cast<float>(Gain(fx / 1000.0f))) * 10.0f;
                        fy = ((fy < 0) ? static_cast<float>(-Gain(-fy / 1000.0f)) : static_cast<float>(Gain(fy / 1000.0f))) * 10.0f;
                    }

                    Cursor(fx, fy, (action.second.CurrentSource != InputSource::Mouse));
                    break;
                }
                case InputAction::Action:
                {
                    if (action.second.CurrentData != 0)
                    {
                        BeginAction();
                    }
                    else
                    {
                        EndAction();
                    }
                    break;
                }
                case InputAction::Back:
                {
                    if (action.second.CurrentData != 0)
                    {
                        Back();
                    }
                    break;
                }
                case InputAction::Cycle:
                {
                    if (action.second.CurrentData != 0)
                    {
                        Cycle();
                    }
                    break;
                }
                case InputAction::MoveForward:
                {
                    if (action.second.CurrentSource == InputSource::JoystickAxis)
                    {
                        int x = (action.second.CurrentData >> 16) & 0xffff;
                        if (x & 0x8000) x |= ~0xffff;
                        int y = action.second.CurrentData & 0xffff;
                        if (y & 0x8000) y |= ~0xffff;

                        float fx = static_cast<float>(x);
                        float fy = static_cast<float>(y);

                        fx = (fx < 0) ? static_cast<float>(-Gain(-fx / 1000.0f)) : static_cast<float>(Gain(fx / 1000.0f));
                        fy = (fy < 0) ? static_cast<float>(-Gain(-fy / 1000.0f)) : static_cast<float>(Gain(fy / 1000.0f));

                        MoveForward(-fy);
                        MoveLeft(-fx);
                    }
                    else
                    {
                        float v = static_cast<float>(action.second.CurrentData);
                        MoveForward(v);
                    }
                    break;
                }
                case InputAction::MoveBack:
                {
                    if (action.second.CurrentSource != InputSource::JoystickAxis)
                    {
                        float v = static_cast<float>(action.second.CurrentData);
                        MoveBack(v);
                    }
                    break;
                }
                case InputAction::MoveLeft:
                {
                    if (action.second.CurrentSource != InputSource::JoystickAxis)
                    {
                        float v = static_cast<float>(action.second.CurrentData);
                        MoveLeft(v);
                    }
                    break;
                }
                case InputAction::MoveRight:
                {
                    if (action.second.CurrentSource != InputSource::JoystickAxis)
                    {
                        float v = static_cast<float>(action.second.CurrentData);
                        MoveRight(v);
                    }
                    break;
                }
                case InputAction::MoveUp:
                {
                    float v = static_cast<float>(action.second.CurrentData);
                    MoveUp(v);
                    break;
                }
                case InputAction::MoveDown:
                {
                    float v = static_cast<float>(action.second.CurrentData);
                    MoveDown(v);
                    break;
                }
                case InputAction::Run:
                {
                    Run(action.second.CurrentData != 0);
                    break;
                }
                case InputAction::Next:
                {
                    if (action.second.CurrentData != 0)
                    {
                        Next();
                    }
                    break;
                }
                case InputAction::Prev:
                {
                    if (action.second.CurrentData != 0)
                    {
                        Prev();
                    }
                    break;
                }
                case InputAction::Inventory:
                {
                    if (action.second.CurrentData != 0)
                    {
                        Inventory();
                    }
                    break;
                }
                case InputAction::Travel:
                {
                    if (action.second.CurrentData != 0)
                    {
                        Travel();
                    }
                    break;
                }
                case InputAction::Hints:
                {
                    Hints();
                    break;
                }
            }

            if (action.second.CurrentSource != InputSource::JoystickAxis || action.second.CurrentData == 0)
            {
                CInputMapping::ControlsMap[action.first].IsActive = false;
            }
        }
    }
}

void CModuleBase::Cursor(float x, float y, bool relative)
{
    _cursorPosX = std::min(std::max(relative ? _cursorPosX + x : x, static_cast<float>(_cursorMinX)), static_cast<float>(_cursorMaxX));
    _cursorPosY = std::min(std::max(relative ? _cursorPosY + y : y, static_cast<float>(_cursorMinY)), static_cast<float>(_cursorMaxY));
}
