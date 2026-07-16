#pragma once
#include "AnimBase.h"
#include <cstdint>

class CWave : public CAnimBase
{
public:
	CWave();
	~CWave();

	virtual bool Init(uint8_t* pData, int length) override;
	virtual bool IsWave() override { return true; }
	virtual bool HasVideo() override { return false; }

protected:
	virtual bool DecodeFrame() override;

	uint64_t _timeOfStart;
};
