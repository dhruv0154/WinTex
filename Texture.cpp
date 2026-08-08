#include "Texture.h"
#include "Globals.h"
#include <cstring>
#include "Utilities.h"

CTexture::CTexture()
{
	_width = 0;
	_height = 0;

	_texture = nullptr;
	_textureRV = nullptr;
}

CTexture::CTexture(CDirectX* pDX, int w, int h, uint8_t* input, int* palette, int rotate)
{
	_texture = nullptr;
	_textureRV = nullptr;

	if (rotate != 0)
	{
		int t = w;
		w = h;
		h = t;
	}

	_width = w;
	_height = h;

	D3D11_TEXTURE2D_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.Width = w;
	desc.Height = h;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	if (dx.CreateTexture2D(&desc, nullptr, &_texture) == 0)
	{
		D3D11_MAPPED_SUBRESOURCE subRes;
		if (pDX->Map(_texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes) == 0)
		{
			int* pTex = (int*)subRes.pData;
			if (rotate == 0)
			{
				for (int y = 0; y < h; y++)
				{
					for (int x = 0; x < w; x++)
					{
						pTex[y * subRes.RowPitch / 4 + x] = palette[input[y * _width + x]];
					}
				}
			}
			else
			{
				for (int y = 0; y < h; y++)
				{
					for (int x = 0; x < w; x++)
					{
						pTex[y * subRes.RowPitch / 4 + x] = palette[input[x * _height + y]];
					}
				}
			}

			pDX->Unmap(_texture, 0);
		}

		dx.CreateShaderResourceView(_texture, nullptr, &_textureRV);
	}
	else
	{
		int debug = 0;
	}
}

CTexture::~CTexture()
{
	Dispose();
}

bool CTexture::Init(int width, int height, uint32_t usageFlags, uint32_t miscFlags, ID3D11Device* pD3D)
{
	Dispose();

	bool ret = false;

	_width = width;
	_height = height;

	if (width > 0 && height > 0)
	{
		// Create buffer and shader resource view
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.Usage = usageFlags;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		
		if (dx.CreateTexture2D(&desc, nullptr, &_texture) == 0)
		{
			if (dx.CreateShaderResourceView(_texture, nullptr, &_textureRV))
			{
				ret = true;
			}
		}
	}

	return ret;
}

bool CTexture::Init(const char* file)
{
	Dispose();

	bool ret = false;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = _width = 1; 
    desc.Height = _height = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

    if (dx.CreateTexture2D(&desc, nullptr, &_texture) == 0)
    {
        if (dx.CreateShaderResourceView(_texture, nullptr, &_textureRV) == 0)
        {
            ret = true;
        }
    }

    return ret;
}

bool CTexture::Init(uint8_t* pImage, uint32_t size, const char* name)
{
    Dispose();

    bool ret = false;

    D3DX11_IMAGE_LOAD_INFO li = {};
    li.MipLevels = 1;
    li.Usage = D3D11_USAGE_STAGING;

    if (D3DX11CreateTextureFromMemory(dx.GetDevice(), pImage, size, &li, nullptr, reinterpret_cast<ID3D11Resource**>(&_texture), nullptr) == 0)
    {
        D3D11_TEXTURE2D_DESC desc;
        _texture->GetDesc(&desc);
        _width = desc.Width;
        _height = desc.Height;

        if (dx.CreateShaderResourceView(_texture, nullptr, &_textureRV) == 0)
        {
            ret = true;
        }
    }

    return ret;
}

bool CTexture::Init(uint8_t* pData, uint32_t size, uint32_t offset, int* pPalette, int transparentIndex, char* name, int sx, int sy, int sw, int sh, bool rawImage, int rawWidth, int rawHeight)
{
	Dispose();

	bool ret = false;
	uint8_t* pImage = pData + offset;

	int imageHeight = rawImage ? rawHeight : GetInt(pImage, 4, 2);
	int width = sw < 0 ? rawImage ? rawWidth : GetInt(pImage, 2, 2) : sw;
	int height = sh < 0 ? rawImage ? rawHeight : imageHeight : sh;

	_width = width;
	_height = height;

	if (width > 0 && height > 0)
	{
		// Create buffer and shader resource view
		D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		if (dx.CreateTexture2D(&desc, nullptr, &_texture) == 0)
		{

			D3D11_MAPPED_SUBRESOURCE subRes;
			if (dx.Map(_texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes) == 0)
			{
				int inPtr = 16;
				int* pTex = (int*)subRes.pData;
				memset(pTex, 0, height * subRes.RowPitch);

				if (rawImage)
				{
					inPtr += rawWidth * sy - 16;
					for (int y = 0; y < height; y++)
					{
						for (int x = 0; x < width; x++)
						{
							int pix = pImage[inPtr + sx + x];
							pTex[y * subRes.RowPitch / 4 + x] = (pix != transparentIndex) ? pPalette[pix] : 0;
						}

						inPtr += rawWidth;
					}
				}
				else
				{
					for (int y = 0; y < imageHeight; y++)
					{
						int c1 = GetInt(pImage, inPtr, 2);
						int c2 = GetInt(pImage, inPtr + 2, 2);

						if (y >= sy && y < (sy + height))
						{
							for (int x = 0; x < width; x++)
							{
								int pix = (x >= c1 && x < (c1 + c2)) ? pImage[inPtr + 4 + x - c1] : 0;
								pTex[(y - sy) * subRes.RowPitch / 4 + x + sx] = (pix != transparentIndex) ? pPalette[pix] : 0;
							}
						}

						inPtr += 4 + c2;
					}
				}

				dx.Unmap(_texture, 0);

				if (dx.CreateShaderResourceView(_texture, nullptr, &_textureRV) == 0)
				{
					ret = true;
				}
			}
		}
	}

	return ret;
}

void CTexture::Dispose()
{
	if (_texture != nullptr)
	{
		_texture->Release();
		_texture = nullptr;
	}

	if (_textureRV != nullptr)
	{
		_textureRV->Release();
		_textureRV = nullptr;
	}
}
