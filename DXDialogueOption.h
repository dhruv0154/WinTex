#pragma once

#include "DXButton.h"
#include "Structs.h"
#include <string>
#include <cstdint>

class CDXDialogueOption : public CDXButton
{
public:
    CDXDialogueOption();
    ~CDXDialogueOption();

    static void Init();
    static void Dispose();

    void SetClick(void(*Clicked)(void* data));
    void SetText(char* text, Size sz, float x);

    virtual void Render();

    void Clear();

    void Resize(int index, int width, int height);

    void SetValue(int value) { _value = value; }
    int GetValue() const { return _value; }

    static void SetGlobalColours(int colour1, int colour2, int colour3, int colour4) 
    { 
        _textColour1 = colour1; 
        _textColour2 = colour2; 
        _textColour3 = colour3; 
        _textColour4 = colour4; 
    }

    std::string _text;

protected:
    static CTexture _texBubble;
    Size _sz;
    int _value{ 0 };

    static int _textColour1;
    static int _textColour2;
    static int _textColour3;
    static int _textColour4;
};
