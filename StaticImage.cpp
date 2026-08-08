#include "StaticImage.h"

CStaticImage::CStaticImage(int factor)
{
	_rate = 0;
}

CStaticImage::~CStaticImage()
{
}

bool CStaticImage::Init(uint8_t* pData, int length)
{
	if (!CAnimBase::Init(pData, length)) return false;

	if (_texture.Init(pData, length, "IMAGE"))
	{
		_width = _texture.Width();
		_height = _texture.Height();

		CreateBuffers(_width, _height);
		return true;
	}

	return false;
}