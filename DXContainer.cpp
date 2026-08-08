#include "DXContainer.h"
#include "Globals.h"
#include "Utilities.h"
#include "DXBitmap.h"
#include "DXButton.h"
#include <algorithm>

CDXContainer::CDXContainer() : CDXControl()
{
}

CDXContainer::~CDXContainer()
{
    for (auto* child : _childElements)
    {
        delete child;
    }
    _childElements.clear();
}

CDXBitmap* CDXContainer::AddBitmap(const char* fileName, Alignment alignment)
{
    CDXBitmap* pBmp = new CDXBitmap(fileName, alignment);
    AddChild(pBmp, 0.0f, 0.0f);
    return pBmp;
}

CDXBitmap* CDXContainer::AddBitmap(uint8_t* pImg, uint32_t size, Alignment alignment)
{
    CDXBitmap* pBmp = new CDXBitmap(pImg, size, alignment);
    AddChild(pBmp, 0.0f, 0.0f);
    return pBmp;
}

CDXButton* CDXContainer::AddButton(const char* text, float x, float y, float w, float h, void(*onClick)(void* data))
{
    CDXButton* pBtn = new CDXButton(text, w, h, onClick);
    AddChild(pBtn, x, y);
    return pBtn;
}

void CDXContainer::AddChild(CDXControl* pCtrl, float x, float y)
{
    if (pCtrl != nullptr)
    {
        // Position is relative to parent...
        pCtrl->SetPosition(_x + x, _y + y);
        _childElements.push_back(pCtrl);
    }
}

void CDXContainer::RemoveChild(CDXControl* pCtrl)
{
    _childElements.remove(pCtrl);
}

void CDXContainer::Render()
{
    dx.DisableZBuffer();
    dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (auto* child : _childElements)
    {
        if (child != nullptr && child->GetVisible())
        {
            child->Render();
        }
    }

    dx.EnableZBuffer();
}

CDXControl* CDXContainer::HitTest(float x, float y)
{
    if (!_visible || !_enabled)
    {
        return nullptr;
    }

    if (x >= _x && y >= _y && x < (_x + _w) && y < (_y + _h))
    {
        if (!_modalElements.empty())
        {
            CDXControl* pModal = _modalElements.front();
            if (pModal != nullptr)
            {
                CDXControl* pCtrl = pModal->HitTest(x, y);
                if (pCtrl != nullptr)
                {
                    CDXControl::ControlType type = pCtrl->GetType();
                    if (type == ControlType::Button || type == ControlType::ImageButton || 
                        type == ControlType::CheckBox || type == ControlType::SaveGameControl || 
                        type == ControlType::TabItem || type == ControlType::Control || 
                        type == ControlType::Slider)
                    {
                        return pCtrl;
                    }
                }
            }
        }
        else
        {
            for (auto it = _childElements.rbegin(); it != _childElements.rend(); ++it)
            {
                CDXControl* pCtrl = (*it)->HitTest(x, y);
                if (pCtrl != nullptr)
                {
                    CDXControl::ControlType type = pCtrl->GetType();
                    if (type == ControlType::Button || type == ControlType::ImageButton || 
                        type == ControlType::CheckBox || type == ControlType::SaveGameControl || 
                        type == ControlType::TabItem || type == ControlType::Control || 
                        type == ControlType::Slider)
                    {
                        return pCtrl;
                    }
                }
            }

            return this;
        }
    }

    return nullptr;
}

void CDXContainer::ShowModal(CDXControl* pControl)
{
    if (pControl != nullptr)
    {
        _modalElements.push_front(pControl);
        pControl->SetVisible(true);
    }
}

void CDXContainer::PopModal()
{
    if (!_modalElements.empty())
    {
        _modalElements.front()->SetVisible(false);
        _modalElements.pop_front();
    }
}

CDXControl* CDXContainer::GetCurrentMouseOver()
{
    if (!_modalElements.empty())
    {
        CDXControl* pModal = _modalElements.front();
        if (pModal != nullptr)
        {
            CDXControl* pCurrent = pModal->GetCurrentMouseOver();
            if (pCurrent != nullptr)
            {
                return pCurrent;
            }
        }

        return nullptr;
    }

    for (auto* child : _childElements)
    {
        if (child != nullptr)
        {
            CDXControl* pCurrent = child->GetCurrentMouseOver();
            if (pCurrent != nullptr)
            {
                return pCurrent;
            }
        }
    }

    return CDXControl::GetCurrentMouseOver();
}

bool CDXContainer::IsModal() const
{
    return !_modalElements.empty();
}

CDXControl* CDXContainer::GetModal()
{
    return !_modalElements.empty() ? _modalElements.front() : nullptr;
}

void CDXContainer::SetColours(int colour1, int colour2, int colour3, int colour4)
{
    for (auto* child : _childElements)
    {
        if (child != nullptr)
        {
            child->SetColours(colour1, colour2, colour3, colour4);
        }
    }

    for (auto* modal : _modalElements)
    {
        if (modal != nullptr)
        {
            modal->SetColours(colour1, colour2, colour3, colour4);
        }
    }
}
