#pragma once

#include "DXBase.h"
#include "DirectX.h"

class CTexture : public CDXBase
{
public:
	CTexture();
	CTexture(CDirectX* pDX, int w, int h, uint8_t* input, int* palette, int rotate);

	~CTexture();

	bool Init(int width, int height, uint32_t usageFlags = D3D11_USAGE_DYNAMIC, uint32_t miscFlags = 0, ID3D11Device* pD3D = NULL);
	bool Init(const char* file);
	bool Init(uint8_t* pImage, uint32_t size, const char* name);
	bool Init(uint8_t* pImage, uint32_t size, uint32_t offset, int* pPalette, int transparentIndex, char* name, int sx = 0, int sy = 0, int sw = -1, int sh = -1, bool rawImage = false, int rawWidth = 0, int rawHeight = 0);
	void Dispose();

	ID3D11Texture2D* GetTexture() { return _texture; }
	ID3D11ShaderResourceView* GetTextureRV() { return _textureRV; }

	int Width() { return _width; }
	int Height() { return _height; }

protected:
	int _width;
	int _height;

	ID3D11Texture2D* _texture;
	ID3D11ShaderResourceView* _textureRV;
};
