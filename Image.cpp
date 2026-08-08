#include "Image.h"
#include "Globals.h"
#include <cstring>
#include <algorithm>

CImage::CImage(DoubleData dd, int width, int height, int factor) : CAnimBase()
{
    Init(dd.File1.Data, dd.File2, width, height, factor);
}

CImage::CImage(uint8_t* palette, BinaryData bd, int width, int height, int factor) : CAnimBase()
{
    Init(palette, bd, width, height, factor);
}

CImage::~CImage()
{
}

bool CImage::Update()
{
    bool updated = CAnimBase::Update();
    _done = false;
    return updated;
}

void CImage::Init(uint8_t* palette, BinaryData bd, int width, int height, int factor)
{
    CAnimBase::Init(bd);

    _width = width;
    _height = height;

    if (palette != nullptr)
    {
        for (int c = 0; c < 256; c++)
        {
            double r = palette[c * 3 + 0];
            double g = palette[c * 3 + 1];
            double b = palette[c * 3 + 2];
            int ri = static_cast<uint8_t>((r * 255.0) / 63.0);
            int gi = static_cast<uint8_t>((g * 255.0) / 63.0);
            int bi = static_cast<uint8_t>((b * 255.0) / 63.0);
            int col = 0xff000000 | bi | (gi << 8) | (ri << 16);
            _pPalette[c] = col;
        }
    }

    CreateBuffers(width, height, factor);
    _texture.Init(_width, _height);

    // Copy raw indexed bitmap data into video output buffer
    if (_pVideoOutputBuffer != nullptr && bd.Data != nullptr)
    {
        std::memcpy(_pVideoOutputBuffer, bd.Data, std::min(width * height, static_cast<int>(bd.Length)));
    }

    _framePointer = 1;

    ID3D11Texture2D* pTex = _texture.GetTexture();
    if (pTex != nullptr)
    {
        D3D11_MAPPED_SUBRESOURCE subRes;
        if (dx.Map(pTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes) == 0)
        {
            int* pScr = reinterpret_cast<int*>(subRes.pData);
            for (int y = 0; y < _height; y++)
            {
                for (int x = 0; x < _width; x++)
                {
                    pScr[y * subRes.RowPitch / 4 + x] = _pPalette[_pVideoOutputBuffer[y * _width + x]];
                }
            }

            dx.Unmap(pTex, 0);
        }
    }
}