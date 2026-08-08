#pragma once

#include "DXFrame.h"
#include <cstdint>

class CDXTabControl;

class CDXTabItem : public CDXFrame
{
public:
    CDXTabItem(CDXTabControl* pOwner, const char* title, float w, float h);
    virtual ~CDXTabItem() override;

    virtual void Render(float x, float y, float hx, float hy, bool selected);
    virtual void MouseButtonDown() override;
    virtual CDXControl* HitTest(float x, float y) override;

    static void Init();
    static void Dispose();

    void Select();

    virtual void SetColours(int colour1, int colour2, int colour3, int colour4) override;

private:
    static CTexture _texBackgroundTabItem;

    CDXTabControl* _pOwner{nullptr};

    int _colour1{0};
    int _colour2{-1};
    int _colour3{-1};
    int _colour4{0};
};