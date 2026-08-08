#include "DXLabel.h"
#include "DXScreen.h"

CDXLabel::CDXLabel(const char* text, CDXText::Alignment alignment) : CDXControl()
{
    _text = new CDXText();
    _text->SetText(text, alignment);
    _text->SetColours(0xffffffff);

    _type = ControlType::Label;
}

CDXLabel::CDXLabel(const char* text, Rect rect, CDXText::Alignment alignment) : CDXControl()
{
    _text = new CDXText();
    _text->SetText(text, rect, alignment);
    _text->SetColours(0xffffffff);

    _type = ControlType::Label;
}

CDXLabel::~CDXLabel()
{
    if (_text != nullptr)
    {
        delete _text;
        _text = nullptr;
    }
}

void CDXLabel::Render()
{
    // Draw text
    if (_text != nullptr)
    {
        _text->Render(_x, _y);
    }
}

void CDXLabel::SetText(const char* text)
{
    if (_text != nullptr)
    {
        _text->SetText(text);
    }
}

void CDXLabel::SetColours(int colour1, int colour2, int colour3, int colour4)
{
    if (_text != nullptr)
    {
        _text->SetColours(colour1, colour2, colour3, colour4);
    }
}