#pragma once

#include "DXText.h"
#include "ShaderStructs.h"
#include <string>
#include <list>
#include <vector>
#include <memory>
#include <cstdint>

class CDXMultiColouredText : public CDXText
{
public:
    CDXMultiColouredText();
    virtual ~CDXMultiColouredText() override;

    void SetColours(int colour1, int colour2, int colour3, int colour4 = -1);
    virtual void Render(float x, float y, float z = -1.0f) override;
    
    void SetText(const char* text, Rect rect);
    void SetTextPD(const char* text, Rect rect);
    void SetTextPD2(const char* text, Rect rect);

    class CMCWordList : public CWordList
    {
    public:
        CMCWordList() : CWordList(), Colour(0) {}
        CMCWordList(const char* text, int chars, float pixels, int col);
        virtual ~CMCWordList() = default;

        void Add(const char* text, int chars, float pixels, int col);

        int Colour{0};

        float Red() const { return static_cast<float>((Colour >> 16) & 0xff) / 255.0f; }
        float Green() const { return static_cast<float>((Colour >> 8) & 0xff) / 255.0f; }
        float Blue() const { return static_cast<float>(Colour & 0xff) / 255.0f; }
    };

    class CMCChar
    {
    public:
        char Char{0};
        int Colour{0};
        float Pixels{0.0f};

        float Red() const { return static_cast<float>((Colour >> 16) & 0xff) / 255.0f; }
        float Green() const { return static_cast<float>((Colour >> 8) & 0xff) / 255.0f; }
        float Blue() const { return static_cast<float>(Colour & 0xff) / 255.0f; }
    };

    class CMCWord
    {
    public:
        std::list<CMCChar> Characters;
        float Pixels{0.0f};
    };
};
