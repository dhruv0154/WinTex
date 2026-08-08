#pragma once

#include "Mutex.h"
#include <cstdint>

class CSimpleAnimPlayer
{
public:
    CSimpleAnimPlayer();
    virtual ~CSimpleAnimPlayer();

    void Init(uint8_t* pData);
    void Merge(uint8_t* pData);

    bool DecodeFrame(uint8_t* pScreen, int x, int y, int w);

protected:
    int _currentFrame;
    bool _firstFrameFull;
    int _animationFrames;
    int _animationWidth;
    int _animationHeight;
    uint8_t* _animationPointer;
    uint8_t* _dataPointer;
    CMutex _lock;
};