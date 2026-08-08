#pragma once

#include "AnimBase.h"
#include <cstdint>

class CPicture : public CAnimBase
{
public:
	CPicture();
	virtual ~CPicture();

	virtual bool Init(uint8_t* pData, int length);

protected:
	virtual bool DecodeFrame();
};