#include "RawFont.h"
#include "Utilities.h"
#include "DirectX.h"
#include <cstring>

CRawFont::CRawFont() : CTexture()
{
    _pFontData = nullptr;
    _fontDataSize = 0;
    _bitsPerPixel = 0;
    _fontHeight = 0;
}

CRawFont::CRawFont(int resource) : CTexture()
{
    _pFontData = nullptr;
    _fontDataSize = 0;
    _bitsPerPixel = 0;
    _fontHeight = 0;

    Init(resource);
}

CRawFont::~CRawFont()
{
    if (_pFontData != nullptr)
    {
        delete[] _pFontData;
    }

    _pFontData = nullptr;
    _fontDataSize = 0;
    _bitsPerPixel = 0;
    _fontHeight = 0;

    _fontMap.clear();
}

void CRawFont::Init(int resource)
{
    uint32_t fontDataSize = 0;
    uint8_t* pFontData = GetResource(resource, "BIN", &fontDataSize);
    MapAndCreateTexture(pFontData, fontDataSize);
    _pFontData = nullptr; // Prevent double-deleting resource memory managed by the archive loader
}

void CRawFont::Init(BinaryData& data)
{
    MapAndCreateTexture(data.Data, data.Length);
}

void CRawFont::MapAndCreateTexture(uint8_t* pFontData, int fontDataSize, bool createTexture)
{
    if (pFontData != nullptr)
    {
        _pFontData = pFontData;
        _fontDataSize = fontDataSize;
        int charCount = pFontData[0];
        _bitsPerPixel = pFontData[1];
        _fontHeight = pFontData[2];
        int widest = 0;

        for (int i = 0; i < charCount; i++)
        {
            int offset = GetInt(pFontData + 3, i * 4, 4);
            uint8_t* pChar = pFontData + offset;
            _fontMap[' ' + i] = pChar;

            // Find widest character in the font set
            if (pChar[0] > widest)
            {
                widest = pChar[0];
            }
        }

        if (createTexture)
        {
            int textureWidth = charCount * widest;
            int textureHeight = _fontHeight;

            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = textureWidth;
            desc.Height = textureHeight * 4;    // Shader currently supports 4-color fonts
            desc.MipLevels = desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            desc.MiscFlags = 0;

            ID3D11Texture2D* pTexture = nullptr;
            if (dx.CreateTexture2D(&desc, nullptr, &pTexture) == 0)
            {
                D3D11_MAPPED_SUBRESOURCE mappedResource;
                if (dx.Map(pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource) == 0)
                {
                    uint8_t* pData = reinterpret_cast<uint8_t*>(mappedResource.pData);
                    int textureOffset = 0;

                    for (int i = 0; i < charCount; i++)
                    {
                        int offset = GetInt(pFontData + 3, i * 4, 4);
                        uint8_t* pChar = pFontData + offset;
                        int charWidth = *(pChar++);
                        int bytesPerRow = (charWidth * _bitsPerPixel + 7) / 8;

                        for (int fy = 0; fy < _fontHeight; fy++)
                        {
                            int bitOffset = 0;

                            for (int fx = 0; fx < charWidth; fx++)
                            {
                                int pixel = ReadBits(pChar, _bitsPerPixel, bitOffset);
                                if (pixel > 0 && pixel < 5)
                                {
                                    pData[textureOffset + ((fy + (pixel - 1) * _fontHeight) * desc.Width + fx) * 4 + 0] = 255;
                                    pData[textureOffset + ((fy + (pixel - 1) * _fontHeight) * desc.Width + fx) * 4 + 1] = 255;
                                    pData[textureOffset + ((fy + (pixel - 1) * _fontHeight) * desc.Width + fx) * 4 + 2] = 255;
                                    pData[textureOffset + ((fy + (pixel - 1) * _fontHeight) * desc.Width + fx) * 4 + 3] = 255;
                                }
                            }

                            pChar += bytesPerRow;
                        }

                        textureOffset += widest * 4;
                    }

                    dx.Unmap(pTexture, 0);
                }
            }
        }
    }
}

void CRawFont::Render(uint8_t* screen, int screenWidth, int screenHeight, int x, int y, uint8_t character, int colourBase, bool center)
{
    uint8_t* pCharData = _fontMap[character];
    if (pCharData != nullptr)
    {
        int charWidth = *(pCharData++);
        int bytesPerRow = (charWidth * _bitsPerPixel + 7) / 8;

        if (center)
        {
            x -= charWidth / 2;
            y -= _fontHeight / 2;
        }

        for (int fy = 0; fy < _fontHeight; fy++)
        {
            int bitOffset = 0;

            for (int fx = 0; fx < charWidth; fx++)
            {
                int pixel = ReadBits(pCharData, _bitsPerPixel, bitOffset);
                if (pixel != 0)
                {
                    screen[(y + fy) * screenWidth + x + fx] = static_cast<uint8_t>(pixel + colourBase);
                }
            }

            pCharData += bytesPerRow;
        }
    }
}

Rect CRawFont::Render(uint8_t* screen, int screenWidth, int screenHeight, int x, int y, const char* text, std::unordered_map<int, int> colourMap, int horizontalAdjustment, int verticalAdjustment, bool ignoreReturn)
{
    Rect box{ x, y, 0, 0 };
    int originalX = x;

    while (*text != 0)
    {
        char character = *(text++);
        if ((character == '\r' || character == '\n') && ignoreReturn)
        {
            character = ' ';
        }

        if (character == '\r' || character == '\n')
        {
            // Advance to next line
            x = originalX;
            y += _fontHeight + verticalAdjustment;
        }
        else
        {
            uint8_t* pCharData = _fontMap[character];
            if (pCharData != nullptr)
            {
                int charWidth = pCharData[0];
                int bytesPerRow = (charWidth * _bitsPerPixel + 7) / 8;
                int byteOffset = 1;

                for (int fy = 0; fy < _fontHeight; fy++)
                {
                    int bitOffset = byteOffset * 8;

                    for (int fx = 0; fx < charWidth; fx++)
                    {
                        int pixel = ReadBits(pCharData, _bitsPerPixel, bitOffset);
                        auto it = colourMap.find(pixel);
                        if (it != colourMap.end() && it->second != 0)
                        {
                            screen[(y + fy) * screenWidth + x + fx] = static_cast<uint8_t>(it->second);
                        }
                    }

                    byteOffset += bytesPerRow;
                }

                x += charWidth + horizontalAdjustment;
                if (x > box.Right)
                {
                    box.Right = x;
                }
            }
        }
    }

    box.Bottom = y + _fontHeight - verticalAdjustment;
    return box;
}

int CRawFont::Measure(const char* text, int horizontalAdjustment)
{
    int x = 0;

    while (*text != 0)
    {
        char character = *(text++);
        uint8_t* pCharData = _fontMap[character];
        if (pCharData != nullptr)
        {
            x += pCharData[0] + horizontalAdjustment;
        }
    }

    return x;
}