#include "UAKMColonelsSafeModule.h"
#include "GameController.h"
#include "UAKMGame.h"
#include "ConstantBuffers.h"
#include "DoubleData.h"
#include "Utilities.h"
#include "Shaders.h"
#include <chrono>

#define IMG_DIAL_1  0
#define IMG_DIAL_2  1
#define IMG_DIAL_3  2
#define IMG_GREEN   3
#define IMG_HAND    4
#define SND_1       5

uint8_t ColonelsSafeCorrectCode[] = { 5, 7, 1 };
int ColonelsSafeDialOffsets[] = { 169, 285, 396 };

CUAKMColonelsSafeModule::CUAKMColonelsSafeModule(int parameter) : CFullScreenModule(ModuleType::AAASafe)
{
	_parameter = parameter;

	_frameTime = 0;

	ResetCode();
}

CUAKMColonelsSafeModule::~CUAKMColonelsSafeModule()
{
	Dispose();
}

void CUAKMColonelsSafeModule::Render()
{
	uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	if (_currentFrame > 0 && (now - _frameTime) > 50)
	{
		int x = ColonelsSafeDialOffsets[_dial];
		int y = 273;

		Render(3 - _currentFrame, x, y);
		_frameTime = now;
		_currentFrame--;

		if (_currentFrame == 0)
		{
			_sound.Play(_files[SND_1]);

			bool correct = true;
			for (int i = 0; i < 3; i++)
			{
				if (ColonelsSafeCorrectCode[i] != _enteredCode[i])
				{
					correct = false;
					break;
				}
			}

			if (correct)
			{
				_currentFrame = -1;
				CGameController::SetParameter(_parameter, 1);

				if (CGameController::GetData(UAKM_SAVE_COLONELS_SAFE_CODE) == 0)
				{
					for (int i = 284; i < 287; i++)
					{
						CGameController::SetHintState(i, 1, 1);
					}
				}
			}
			else
			{
				_inputEnabled = true;
			}
		}

		UpdateTexture();
	}
	else if (_currentFrame == -1 && (now - _frameTime) > 1000)
	{
		// Mark dials green
		for (int i = 0; i < 3; i++)
		{
			Render(3, ColonelsSafeDialOffsets[i] + 25, 300);
		}

		UpdateTexture();
		_currentFrame--;
		_frameTime = now;
	}
	else if (_currentFrame == -2 && (now - _frameTime) > 1000)
	{
		return CModuleController::Pop(this);
	}

	if (_vertexBuffer != nullptr)
	{
		dx.DisableZBuffer();

		uint32_t stride = sizeof(TEXTURED_VERTEX);
		uint32_t offset = 0;
		dx.SetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
		dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		CShaders::SelectOrthoShader();
		float16 wm = Math::Identity();
		CConstantBuffers::SetWorld(dx, &wm);
		ID3D11ShaderResourceView* pRV = _texture.GetTextureRV();
		dx.SetShaderResources(0, 1, &pRV);
		dx.Draw(4, 0);

		dx.SetVertexBuffers(0, 1, &_iconVertexBuffer, &stride, &offset);
		dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		wm = Math::Translation(_cursorPosX, -_cursorPosY, -0.5f);
		CConstantBuffers::SetWorld(dx, &wm);
		pRV = _iconTexture.GetTextureRV();
		dx.SetShaderResources(0, 1, &pRV);
		dx.Draw(4, 0);

		dx.EnableZBuffer();
	}
}

void CUAKMColonelsSafeModule::Initialize()
{
	CFullScreenModule::Initialize();

	CGameController::SetParameter(_parameter, 0);

	DoubleData dd = LoadDoubleEntry("SPECIAL.AP", 0);
	if (dd.File1.Data != nullptr)
	{
		_screen = dd.File2.Data;

		uint8_t* pPal = dd.File1.Data;
		ReadPalette(pPal);

		delete[] pPal;
	}

	UpdateTexture();

	// Load extra files
	BinaryData bd = LoadEntry("SPECIAL.AP", 2);
	if (bd.Data != nullptr)
	{
		_data = bd.Data;
		int count = GetInt(_data, 0, 2) - 1;
		for (int i = 0; i < count; i++)
		{
			_files[i] = _data + GetInt(_data, 2 + i * 4, 4);
		}
	}

	CreateTexturedRectangle(0.0f, 0.0f, -16.0f, 16.0f, &_iconVertexBuffer, "SafeIconVertexBuffer");
	_iconTexture.Init(_files[IMG_HAND], 0, 0, &_palette[0], 0, "SafeIconTexture");
}

void CUAKMColonelsSafeModule::Render(int entry, int offset_x, int offset_y)
{
	uint8_t* pImg = _files[entry];

	int w = GetInt(pImg, 2, 2);
	int h = GetInt(pImg, 4, 2);
	int inPtr = 16;

	for (int y = 0; y < h; y++)
	{
		int c1 = GetInt(pImg, inPtr, 2);
		int c2 = GetInt(pImg, inPtr + 2, 2);

		for (int x = 0; x < w; x++)
		{
			int pix = (x >= c1 && x < (c1 + c2)) ? pImg[inPtr + 4 + x - c1] : 0;
			if (pix != 0)
			{
				_screen[(y + offset_y) * 640 + offset_x + x] = pix;
			}
		}

		inPtr += 4 + c2;
	}
}

void CUAKMColonelsSafeModule::ResetCode()
{
	for (int i = 0; i < 4; i++)
	{
		_enteredCode[i] = 0;
	}

	_dialChanged = false;
}

void CUAKMColonelsSafeModule::TurnDial(int dial)
{
	if (dial >= 0 && dial < 3)
	{
		_dial = dial;
		_inputEnabled = false;
		_currentFrame = 3;
		_frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

		if (_enteredCode[dial + 1] != 0)
		{
			ResetCode();
			CGameController::SetData(UAKM_SAVE_COLONELS_SAFE_CODE, 1);
		}

		_enteredCode[dial]++;
		_dialChanged = true;
	}
}

void CUAKMColonelsSafeModule::BeginAction()
{
	int x = static_cast<int>((_cursorPosX - _left) / _scale);
	int y = static_cast<int>((_cursorPosY - _top) / _scale);

	if (x >= 581 && x <= 639 && y >= 442 && y <= 479)
	{
		return CModuleController::Pop(this);
	}
	else if (_inputEnabled && y >= 273 && y <= 352)
	{
		if (x >= 169 && x <= 248)
		{
			TurnDial(0);
		}
		else if (x >= 284 && x <= 363)
		{
			TurnDial(1);
		}
		else if (x >= 395 && x <= 474)
		{
			TurnDial(2);
		}
	}
}