#pragma once

#include "AnimBase.h"
#include "DoubleData.h"
#include <cstdint>

class CImage : public CAnimBase
{
public:
    CImage(DoubleData dd, int width, int height, int factor = 1);
    CImage(uint8_t* palette, BinaryData bd, int width, int height, int factor = 1);
    virtual ~CImage();

    virtual bool Update() override;

protected:
    virtual bool DecodeFrame() override { return (_frame == 0); }
    virtual void Init(uint8_t* palette, BinaryData bd, int width, int height, int factor);
};