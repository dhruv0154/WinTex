#include "AnimatedCursor.h"
#include "Globals.h"
#include "DirectX.h"
#include <SDL2/SDL.h>

CAnimatedCursor::CAnimatedCursor()
{
    _x = 0.0f;
    _y = 0.0f;
    _hotspotX = 0.0f;
    _hotspotY = 0.0f;
    _lastChange = 0;
    _currentIcon = 0;
    _direction = 1;
    _interval = 50;
    _forwardInterval = 50;
    _reverseInterval = 50;
    _loopDelay = 0;
    _loopDelayCounter = 0;
    _type = CursorType::Arrow;
}

CAnimatedCursor::~CAnimatedCursor()
{
    Dispose();
}

void CAnimatedCursor::Dispose()
{
    for (auto* icon : _icons)
    {
        delete icon;
    }
    _icons.clear();
}

void CAnimatedCursor::Render()
{
    if (_icons.empty()) return;

    if (_currentIcon < 0 || static_cast<size_t>(_currentIcon) >= _icons.size())
    {
        _currentIcon = 0;
    }

    CDXBitmap* currentCursor = _icons[_currentIcon];
    if (currentCursor != nullptr)
    {
        currentCursor->SetPosition(_x, _y);
        currentCursor->Render();
    }

    if (_icons.size() > 1)
    {
        uint64_t now = SDL_GetTicks64();
        if ((now - _lastChange) > _interval)
        {
            _lastChange = now;
            _currentIcon += _direction;
            
            int iconCount = static_cast<int>(_icons.size());

            if (_currentIcon >= iconCount)
            {
                if (_type == CursorType::Grab || _type == CursorType::Loading)
                {
                    if (_loopDelayCounter < _loopDelay)
                    {
                        _loopDelayCounter++;
                        _currentIcon -= _direction;
                    }
                    else
                    {
                        _currentIcon = 0;
                        _loopDelayCounter = 0;
                    }
                }
                else
                {
                    _direction = -1;
                    _currentIcon += _direction;
                    _interval = _reverseInterval;
                }
            }
            else if (_currentIcon < 0)
            {
                if (_loopDelayCounter < _loopDelay)
                {
                    _loopDelayCounter++;
                    _currentIcon -= _direction;
                }
                else
                {
                    _direction = 1;
                    _currentIcon += _direction;
                    _interval = _forwardInterval;
                    _loopDelayCounter = 0;
                }
            }
        }
    }
}

void CAnimatedCursor::SetIcons(CursorType type, const std::list<CDXBitmap*>& icons)
{
    Dispose();

    _type = type;
    
    if (icons.empty()) return;

    _icons.assign(icons.begin(), icons.end());

    _direction = 1;
    _currentIcon = 0;

    if (type == CursorType::Arrow)
    {
        _hotspotX = 0.0f;
        _hotspotY = 0.0f;
    }
    else if (type == CursorType::Crosshair)
    {
        if (isUAKM)
        {
            _hotspotX = 7.0f;
            _hotspotY = 7.0f;
        }
        else
        {
            _hotspotX = 2.0f;
            _hotspotY = 3.0f;
        }
    }
    else if (type == CursorType::Look)
    {
        _loopDelay = 10;
        _hotspotX = 14.0f;
        _hotspotY = isUAKM ? 4.0f : 12.0f;
    }
    else if (type == CursorType::Move)
    {
        _hotspotX = 11.0f;
        _hotspotY = 29.0f;
        _loopDelay = 10;
    }
    else if (type == CursorType::Grab)
    {
        _hotspotX = 13.0f;
        _hotspotY = 25.0f;
        _loopDelay = 10;
    }
    else if (type == CursorType::OnOff)
    {
        _hotspotX = 10.0f;
        _hotspotY = 4.0f;
        _loopDelay = 10;
    }
    else if (type == CursorType::Talk)
    {
        _hotspotX = 10.0f;
        _hotspotY = 4.0f;
        _loopDelay = 10;
    }
    else if (type == CursorType::Open)
    {
        _reverseInterval = 10;
        _loopDelay = 10;

        if (isUAKM)
        {
            _hotspotX = 12.0f;
            _hotspotY = 4.0f;
        }
        else
        {
            _hotspotX = 14.0f;
            _hotspotY = 7.0f;
        }
    }
}
