#pragma once

#include "DXControl.h"
#include "DXText.h"
#include "Texture.h"
#include <cstdint>

class CDXCheckBox : public CDXControl
{
public:
    CDXCheckBox(const char* text, bool* pValue, float width);
    virtual ~CDXCheckBox() override;

    static void Init();
    static void Dispose();

    virtual void Render() override;

    virtual void MouseEnter() override;
    virtual void MouseLeave() override;
    virtual void MouseButtonUp() override;

    void SetCheck(bool check) { if (_pChecked) *_pChecked = check; }
    bool GetCheck() const { return _pChecked ? *_pChecked : false; }

    virtual void SetColours(int colour1, int colour2, int colour3, int colour4) override;

protected:
    static CTexture _cbTexBackground;
    static CTexture _cbTexMouseOver;
    static CTexture _cbTexChecked;
    CDXText* _pText{nullptr};

    float _textX{0.0f};
    float _textY{0.0f};

    bool* _pChecked{nullptr};
};
