#pragma once
#include "AnimBase.h"
#include <cstdint>

class CStaticImage : public CAnimBase
{
public:
	CStaticImage(int factor = 1);
	virtual ~CStaticImage();

	virtual bool Init(uint8_t* pData, int length);

protected:
	virtual bool DecodeFrame() { return false; }
};