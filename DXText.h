#pragma once

#include "DXBase.h"
#include "DXFont.h"
#include "ShaderStructs.h"
#include <string>
#include <vector>
#include <cstdint>

class CDXText : public CDXBase
{
public:
    enum class Alignment
    {
        Left = 0,
        Center = 1,
        Right = 2,
        Justify = 3,
        JustifyAlways = 4,
    };

    CDXText();
    virtual ~CDXText();

    std::string _string;
    std::wstring _wstring;

    void SetColours(int colour);
    void SetColours(int colour1, int colour2);
    void SetColours(int colour1, int colour2, int colour3, int colour4);

    virtual void Render(float x, float y, float z = -1.0f);

    void SetText(const char* text, Alignment alignment = Alignment::Left);
    void SetText(const char* text, Rect rect, Alignment alignment = Alignment::Left);
    void SetText(const wchar_t* text, Alignment alignment = Alignment::Left);
    void SetText(const wchar_t* text, Rect rect, Alignment alignment = Alignment::Left);
    void ResetText();

    void SetTextUnmodified(const char* text);
    void SetTextUnmodified(const wchar_t* text);

    float PixelWidth(const char* text);
    int Lines() const { return _lines; }

    float Width() const;
    void Width(float w);
    float Height() const;

    class CWordList
    {
    public:
        CWordList();
        ~CWordList();

        void Add(const char* text, int chars, float pixels);
        CWordList* Next();
        float Pixels() const;
        const char* Text() const;
        int Chars() const;

        void SetNext(CWordList* pNext) { _next = pNext; }

    protected:
        CWordList(const char* text, int chars, float pixels);

        const char* _text{nullptr};
        int _chars{0};
        float _pixels{0.0f};

        CWordList* _next{nullptr};
        CWordList* _last{nullptr};
    };

protected:
    ID3D11Buffer* _vertexBuffer{nullptr};
    int _printableCharacters{0};
    int _lines{0};
    float _width{0.0f};

    int _colour1{0};
    int _colour2{0};
    int _colour3{0};
    int _colour4{0};
};