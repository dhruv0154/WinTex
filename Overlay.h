#pragma once

#include "DXControl.h"
#include <list>

class COverlay
{
public:
    COverlay();
    virtual ~COverlay() = default;

    virtual void KeyDown(int key, int lParam) { }
    virtual void Render() = 0;
    virtual void BeginAction() = 0;
    virtual void SetData(int p1, int p2) {}

    virtual void Cursor(float x, float y, bool relative);

    int GetDecision() const { return _decision; }
    void ClearDecision() { _decision = 0; }

protected:
    float _x;
    float _y;

    int _decision;

    std::list<CDXControl*> _hitTestControls;
};