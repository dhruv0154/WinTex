#include "Shaders.h"
#include "Globals.h"
#include <cmath>

CDXShader* CShaders::_orthoShader = nullptr;
CDXShader* CShaders::_textureShader = nullptr;
CDXShader* CShaders::_texFontShader = nullptr;
CDXShader* CShaders::_texFontShader_AA = nullptr;
CDXShader* CShaders::_multiColouredFontShader = nullptr;
CDXShader* CShaders::_colourShader = nullptr;
CDXShader* CShaders::_transparentColourShader = nullptr;
CDXShader* CShaders::_yuvShader = nullptr;
CDXShader* CShaders::_basicShader = nullptr;

const char* vertexShaderProfile = "vs_5_0";
const char* pixelShaderProfile = "ps_5_0";

void CShaders::SelectOrthoShader()
{
	if (_orthoShader == nullptr)
	{
		D3D11_INPUT_ELEMENT_DESC tsied[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		_orthoShader = new CDXShader(&dx, 0, "OrthoVS", vertexShaderProfile, "TexturedPS", pixelShaderProfile, tsied, 4);
	}

	_orthoShader->Activate(&dx);
}

void CShaders::SelectTextureShader()
{
	if (_textureShader == nullptr)
	{
		D3D11_INPUT_ELEMENT_DESC tsied[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		_textureShader = new CDXShader(&dx, 0, "TexturedVS", vertexShaderProfile, "TexturedPS", pixelShaderProfile, tsied, 4);
	}

	_textureShader->Activate(&dx);
}

void CShaders::SelectTexFontShader()
{
	if (rintf(pConfig->FontScale) == pConfig->FontScale)
	{
		// Integer, use normal shader
		if (_texFontShader == nullptr)
		{
			D3D11_INPUT_ELEMENT_DESC tsied[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			_texFontShader = new CDXShader(&dx, 0, "OrthoVS", vertexShaderProfile, "TexFontPS", pixelShaderProfile, tsied, 2);
		}

		_texFontShader->Activate(&dx);
	}
	else
	{
		// Fraction, use AA shader
		if (_texFontShader_AA == nullptr)
		{
			D3D11_INPUT_ELEMENT_DESC tsied[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};
			_texFontShader_AA = new CDXShader(&dx, 0, "OrthoVS", vertexShaderProfile, "TexFontPS_AA", pixelShaderProfile, tsied, 2);
		}

		_texFontShader_AA->Activate(&dx);
	}
}

void CShaders::SelectMultiColouredFontShader()
{
	if (_multiColouredFontShader == nullptr)
	{
		D3D11_INPUT_ELEMENT_DESC tsied[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		_multiColouredFontShader = new CDXShader(&dx, 0, "MultiColouredFontVS", vertexShaderProfile, "MultiColouredFontPS", pixelShaderProfile, tsied, 3);
	}

	_multiColouredFontShader->Activate(&dx);
}

void CShaders::SelectMultiColouredFontShaderPD()
{
	if (_multiColouredFontShader == nullptr)
	{
		D3D11_INPUT_ELEMENT_DESC tsied[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		_multiColouredFontShader = new CDXShader(&dx, 0, "MultiColouredFontVS", vertexShaderProfile, "MultiColouredFontPSPD", pixelShaderProfile, tsied, 3);
	}

	_multiColouredFontShader->Activate(&dx);
}

void CShaders::SelectColourShader()
{
	if (_colourShader == nullptr)
	{
		D3D11_INPUT_ELEMENT_DESC csied[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		_colourShader = new CDXShader(&dx, 0, "ColouredVS", vertexShaderProfile, "ColouredPS", pixelShaderProfile, csied, 2);
	}

	_colourShader->Activate(&dx);
}

void CShaders::SelectTransparentColourShader()
{
	if (_transparentColourShader == nullptr)
	{
		D3D11_INPUT_ELEMENT_DESC csied[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		_transparentColourShader = new CDXShader(&dx, 0, "TransparentVS", vertexShaderProfile, "TransparentPS", pixelShaderProfile, csied, 3);
	}

	_transparentColourShader->Activate(&dx);
}

void CShaders::SelectYUVShader()
{
	if (_yuvShader == nullptr)
	{
		D3D11_INPUT_ELEMENT_DESC tsied[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		_yuvShader = new CDXShader(&dx, 0, "OrthoVS", vertexShaderProfile, "YUVPS", pixelShaderProfile, tsied, 2);
	}

	_yuvShader->Activate(&dx);
}

void CShaders::SelectBasicShader()
{
	if (_basicShader == nullptr)
	{
		D3D11_INPUT_ELEMENT_DESC tsied[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		_basicShader = new CDXShader(&dx, 0, "BasicVS", vertexShaderProfile, "BasicPS", pixelShaderProfile, tsied, 1);
	}

	_basicShader->Activate(&dx);
}

void CShaders::Dispose()
{
	if (_orthoShader != nullptr)
    {
        delete _orthoShader;
        _orthoShader = nullptr;
    }

    if (_textureShader != nullptr)
    {
        delete _textureShader;
        _textureShader = nullptr;
    }

    if (_texFontShader != nullptr)
    {
        delete _texFontShader;
        _texFontShader = nullptr;
    }
    
    if (_texFontShader_AA != nullptr)
    {
        delete _texFontShader_AA;
        _texFontShader_AA = nullptr;
    }

    if (_multiColouredFontShader != nullptr)
    {
        delete _multiColouredFontShader;
        _multiColouredFontShader = nullptr;
    }

    if (_colourShader != nullptr)
    {
        delete _colourShader;
        _colourShader = nullptr;
    }

    if (_transparentColourShader != nullptr)
    {
        delete _transparentColourShader;
        _transparentColourShader = nullptr;
    }

    if (_yuvShader != nullptr)
    {
        delete _yuvShader;
        _yuvShader = nullptr;
    }

    if (_basicShader != nullptr)
    {
        delete _basicShader;
        _basicShader = nullptr;
    }
}
