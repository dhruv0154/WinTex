#pragma once

#include "DXContainer.h"
#include "DXControl.h"
#include "DXText.h"
#include "Texture.h"
#include <cstdint>
#include <list>

class CDXFrame : public CDXContainer
{
public:
    CDXFrame(const char* title, float w, float h);
    virtual ~CDXFrame() override;

    static void Init();
    static void Dispose();

    virtual void Render() override;

    virtual void MouseEnter() override {}
    virtual void MouseMove() override {}
    virtual void MouseLeave() override {}
    virtual void MouseButtonDown() override {}
    virtual void MouseButtonUp() override {}
    virtual void KeyDown() override {}
    virtual void KeyUp() override {}
    virtual void GotFocus() override {}
    virtual void LostFocus() override {}

    virtual void SetColours(int colour1, int colour2, int colour3, int colour4) override;

protected:
    CDXFrame() {}
    static CTexture _texBackground;
    CDXText* _pText{nullptr};
    int _textW{0};
};
