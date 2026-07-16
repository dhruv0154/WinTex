#pragma once

#include "BIC.h"

class CPTF : public CBIC
{
public:
	CPTF(int factor = 1) : CBIC(factor) { }
	virtual bool Init(uint8_t* pData, int length);
	virtual bool HasVideo() { return (_videoFramePointer != 0); }

protected:
	virtual bool ProcessFLCFrame(int inPtr, int chunkSize);
	virtual bool DecodeFrame();
};
