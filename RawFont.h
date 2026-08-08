#pragma once

#include "Texture.h"
#include "BinaryData.h"
#include "Structs.h"
#include <cstdint>
#include <unordered_map>

class CRawFont : public CTexture
{
public:
    CRawFont();
    CRawFont(int resource);
    virtual ~CRawFont();

    void Init(int resource);
    void Init(BinaryData& data);
    void Render(uint8_t* screen, int screenWidth, int screenHeight, int x, int y, uint8_t character, int colourBase, bool center = false);
    
    Rect Render(uint8_t* screen, int screenWidth, int screenHeight, int x, int y, const char* text, std::unordered_map<int, int> colourMap, int horizontalAdjustment = 0, int verticalAdjustment = 0, bool ignoreReturn = false);

    int Measure(const char* text, int horizontalAdjustment = 0);

    int GetHeight() { return _fontHeight; }
    int GetWidth(char character) { return _fontMap[character][0]; }

protected:
    uint8_t* _pFontData;
    int _fontDataSize;
    int _bitsPerPixel;
    int _fontHeight;

    std::unordered_map<char, uint8_t*> _fontMap;

    void MapAndCreateTexture(uint8_t* pFontData, int fontDataSize, bool createTexture = false);
};