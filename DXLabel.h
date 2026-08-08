#pragma once
#include "DXControl.h"
#include "DXText.h"

class CDXLabel : public CDXControl
{
public:
    CDXLabel(const char* text, CDXText::Alignment alignment = CDXText::Alignment::Left);
    CDXLabel(const char* text, Rect rect, CDXText::Alignment alignment = CDXText::Alignment::Left);
    virtual ~CDXLabel() override;

    virtual void Render() override;

    void SetText(const char* text);
    virtual void SetColours(int colour1, int colour2, int colour3, int colour4) override;

private:
    CDXText* _text{nullptr};
};
