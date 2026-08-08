#include "Picture.h"

CPicture::CPicture()
{
}

CPicture::~CPicture()
{
}

bool CPicture::Init(uint8_t* pData, int length)
{
	int w = 0, h = 0;

	CreateBuffers(w, h);

	return false;
}

bool CPicture::DecodeFrame()
{
	return false;
}