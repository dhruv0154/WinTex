#pragma once

#include "DXControl.h"
#include "Globals.h"
#include <list>
#include <cstdint>
#include <iterator>

class CDXBitmap;
class CDXButton;

class CDXContainer : public CDXControl
{
    friend class CDXControl;

public:
    CDXContainer();
    virtual ~CDXContainer() override;

    virtual void Render() override;

    bool IsModal() const;

    virtual CDXControl* GetCurrentMouseOver() override;

    void AddChild(CDXControl* pCtrl, float x, float y);
    void RemoveChild(CDXControl* pCtrl);
    virtual CDXControl* HitTest(float x, float y) override;

    virtual CDXBitmap* AddBitmap(const char* fileName, Alignment alignment = Alignment::Default);
    virtual CDXBitmap* AddBitmap(uint8_t* pImg, uint32_t size, Alignment alignment = Alignment::Default);
    virtual CDXButton* AddButton(const char* text, float x, float y, float w, float h, void(*onClick)(void* data));

    void ShowModal(CDXControl* pControl);
    CDXControl* GetModal();
    void PopModal();

    CDXControl* GetChild(int index)
    {
        if (index >= 0 && static_cast<size_t>(index) < _childElements.size())
        {
            auto it = _childElements.begin();
            std::advance(it, index);
            return *it;
        }

        return nullptr;
    }

    virtual void SetColours(int colour1, int colour2, int colour3, int colour4) override;

protected:
    std::list<CDXControl*> _childElements;
    std::list<CDXControl*> _modalElements;
};
