#pragma once

#include "AnimBase.h"

class CBIC : public CAnimBase
{
public:
	CBIC(int factor = 1) : CAnimBase() { _factor = factor; };
	virtual bool Init(uint8_t* pData, int length);

protected:
	virtual bool DecodeFrame();

	int _embeddedAudioSize{};
	int _firstAudioFrame{};
	int _chunkTest{};

	int ProcessBICFrame(int offset, int chunkSize);

	int _factor;
};
