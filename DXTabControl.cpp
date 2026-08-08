#include "DXTabControl.h"
#include "Globals.h"
#include "DXScreen.h"

CDXTabControl::CDXTabControl(float w, float h) : CDXContainer()
{
    _selectedItem = nullptr;
    _w = w;
    _h = h;
    _type = ControlType::Control;
}

CDXTabControl::~CDXTabControl()
{
}

void CDXTabControl::Render()
{
    if (_childElements.empty()) return;

    size_t elementCount = _childElements.size();
    float elementWidth = _w / static_cast<float>(elementCount);
    float x = _x;
    float y = _y + 5.0f + TexFont.Height() * pConfig->FontScale;

    for (auto* child : _childElements)
    {
        if (child != nullptr)
        {
            CDXTabItem* tabItem = static_cast<CDXTabItem*>(child);
            tabItem->Render(_x, _y, x, y, (_selectedItem == tabItem));
            x += elementWidth;
        }
    }
}

void CDXTabControl::MouseButtonDown()
{
}

CDXControl* CDXTabControl::HitTest(float x, float y)
{
    if (!_visible || !_enabled || _childElements.empty())
    {
        return nullptr;
    }

    size_t elementCount = _childElements.size();
    float elementWidth = _w / static_cast<float>(elementCount);
    float cx = _x;
    float scaledFontHeight = TexFont.Height() * pConfig->FontScale;
    float cy = _y + 5.0f + scaledFontHeight;

    for (auto* child : _childElements)
    {
        if (child == nullptr) continue;

        float nx = cx + elementWidth;

        if (x >= cx && x <= nx && y >= cy && y <= (cy + scaledFontHeight + 8.0f))
        {
            return child;
        }
        CDXTabItem* tabItem = static_cast<CDXTabItem*>(child);
        if (tabItem == _selectedItem)
        {
            CDXControl* pHit = tabItem->HitTest(x, y);
            if (pHit != nullptr)
            {
                return pHit;
            }
        }

        cx = nx;
    }

    return nullptr;
}

void CDXTabControl::AddChild(CDXControl* pCtrl, float x, float y)
{
    if (pCtrl != nullptr)
    {
        pCtrl->SetPosition(_x + x, _y + y);
        _childElements.push_back(pCtrl);

        if (_selectedItem == nullptr)
        {
            _selectedItem = static_cast<CDXTabItem*>(pCtrl);
        }
    }
}

void CDXTabControl::Select(CDXTabItem* pItem)
{
    _selectedItem = pItem;
}
