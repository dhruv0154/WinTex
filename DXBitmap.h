#pragma once

#include "DXControl.h"
#include "Texture.h"
#include <cstdint>

class CDXBitmap : public CDXControl
{
public:
    CDXBitmap();
    CDXBitmap(const char* fileName, Alignment alignment = Alignment::Default);
    CDXBitmap(uint8_t* pImage, uint32_t size, Alignment alignment = Alignment::Default);
    CDXBitmap(int width, int height, Alignment alignment = Alignment::Default);
    virtual ~CDXBitmap() override;

    virtual void Render() override;

    CTexture* GetTexture() { return &_texture; }

protected:
    CTexture _texture;

    void Init();
};
