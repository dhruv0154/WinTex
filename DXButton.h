#pragma once

#include "DXControl.h"
#include "DXText.h"
#include "Texture.h"
#include "DXSound.h"
#include <cstdint>

class CDXButton : public CDXControl
{
public:
    CDXButton(const char* text, float w, float h, void(*onClick)(void*) = nullptr, void* data = nullptr);
    virtual ~CDXButton() override;

    static void Init();
    static void Dispose();

    virtual void Render() override;

    virtual void MouseEnter() override;
    virtual void MouseMove() override;
    virtual void MouseLeave() override;
    virtual void MouseButtonDown() override;
    virtual void MouseButtonUp() override;
    virtual void KeyDown() override;
    virtual void KeyUp() override;
    virtual void GotFocus() override;
    virtual void LostFocus() override;

    virtual void Click();

    virtual void SetMouseOver(bool mouseOver) override;

    static void SetButtonColours(int colour1, int colour2, int colour3, int colour4);

protected:
    void(*_clicked)(void* data);
    void* _data{nullptr};

    static CTexture _texBackground;
    static CTexture _texMouseOver;
    CDXText* _pText;

    float _textX;
    float _textY;

    static CDXSound* _pSound;

    CDXButton() : CDXControl() { _textX = 0.0f; _textY = 0.0f; _clicked = nullptr; _pText = nullptr; }

    static int Colour1;
    static int Colour2;
    static int Colour3;
    static int Colour4;
};
