#pragma once

#include "DXContainer.h"
#include "DXTabItem.h"

class CDXTabControl : public CDXContainer
{
public:
    CDXTabControl(float w, float h);
    virtual ~CDXTabControl() override;

    virtual void AddChild(CDXControl* pCtrl, float x, float y);

    virtual void Render() override;
    virtual void MouseButtonDown() override;
    virtual CDXControl* HitTest(float x, float y) override;

    void Select(CDXTabItem* pItem);

private:
    CDXTabItem* _selectedItem{nullptr};
};