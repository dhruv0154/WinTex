#pragma once

#include "DXButton.h"
#include "Texture.h"
#include <cstdint>

class CDXImageButton : public CDXButton
{
public:
    CDXImageButton(int img, void(*onClick)(void* data) = nullptr);
    virtual ~CDXImageButton() override;

    static void Init();
    static void Dispose();

    virtual void Render() override;

protected:
    static CTexture _ibTexBackground;
    static CTexture _ibTexMouseOver;
};
