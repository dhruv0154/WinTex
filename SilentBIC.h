#pragma once

#include "BIC.h"
#include <cstdint>

class CSilentBIC : public CBIC
{
public:
    CSilentBIC(int factor = 1) : CBIC(factor) { }
    virtual bool Init(uint8_t* pData, int length) override;

protected:
    virtual bool DecodeFrame() override;
};