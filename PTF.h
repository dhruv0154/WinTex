#pragma once

#include "BIC.h"
#include <cstdint>

class CPTF : public CBIC
{
public:
	CPTF(int factor = 1) : CBIC(factor) { }
	virtual bool Init(uint8_t* pData, int length) override;
	virtual bool HasVideo() override { return (_videoFramePointer != 0); }

protected:
	virtual bool ProcessFLCFrame(int inPtr, int chunkSize);
	virtual bool DecodeFrame() override;
};