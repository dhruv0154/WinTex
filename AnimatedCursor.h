#pragma once

#include <list>
#include "DXBitmap.h"
#include <vector>
#include <cstdint>

class ID3D11Buffer;

class CAnimatedCursor
{
public:
    CAnimatedCursor();
    ~CAnimatedCursor();

    void Dispose();

    enum class CursorType
    {
        Arrow = 0,
        Note = 1,
        Crosshair = 2,
        Diskette = 3,
        Look = 4,
        Move = 5,
        Grab = 6,
        OnOff = 7,
        Talk = 8,
        Hint = 9,
        Open = 10,
        Loading = 11,
        Special = 12
    };

    void SetIcons(CursorType type, const std::list<CDXBitmap*>& icons);

    void SetPosition(float x, float y)
    {
        _x = x - _hotspotX;
        _y = y - _hotspotY;
    }

    void Render();

protected:
    std::vector<CDXBitmap*> _icons;
    
    float _x{0.0f};
    float _y{0.0f};
    float _hotspotX{0.0f};
    float _hotspotY{0.0f};

    uint64_t _lastChange{0};
    int _currentIcon{0};
    int _direction{1};
    uint32_t _interval{50};
    uint32_t _forwardInterval{50};
    uint32_t _reverseInterval{50};
    uint32_t _loopDelay{0};
    uint32_t _loopDelayCounter{0};

    CursorType _type{CursorType::Arrow};
};
