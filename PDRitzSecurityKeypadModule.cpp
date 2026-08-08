#include "PDRitzSecurityKeypadModule.h"
#include "Utilities.h"
#include "GameController.h"
#include "ConstantBuffers.h"
#include "Shaders.h"
#include <chrono>

#define IMG_GREEN       22
#define IMG_RED         24
#define SND_KEY         26
#define SND_BAD         27
#define SND_GOOD        28

#define VK_RETURN       0x0D

signed char DoorCode[] = { 4, 8, 2, 7, -1 };

int KeyHitTestCoordinates[] = { 348,422,195,269,
								94,168,195,269,
								94,168,280,354,
								94,168,365,439,
								179,253,195,269,
								179,253,280,354,
								179,253,365,439,
								263,337,195,269,
								263,337,280,354,
								263,337,365,439,
								348,422,280,439,
								420,449,580,639 };

int KeyRenderCoordinates[] = { 193,345,193,91,278,91,363,91,193,176,278,176,363,176,193,260,278,260,363,260,278,345,338,58,213,58 };

CPDRitzSecurityKeypadModule::CPDRitzSecurityKeypadModule() : CFullScreenModule(ModuleType::CodePanel)
{
	ResetCode();

	for (int i = 0; i < 11; i++)
	{
		_keyTimes[i] = 0;
	}

	_updateTexture = false;

	_codeCorrect = false;
	_blinkFrame = 0;
	_blinkFrameTime = 0;

	_soundStartTime = 0;
}

CPDRitzSecurityKeypadModule::~CPDRitzSecurityKeypadModule()
{
	Dispose();
}

void CPDRitzSecurityKeypadModule::Render()
{
	uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	if (_soundStartTime > 0 && (now - _soundStartTime >= 200))
	{
		_sound.Play(_files[SND_KEY]);
		_soundStartTime = 0;
	}

	for (int key = 0; key < 11; key++)
	{
		if (_keyTimes[key] > 0 && (now - _keyTimes[key]) >= 200)
		{
			// Render original back
			RenderItem(key * 2, KeyRenderCoordinates[key * 2], KeyRenderCoordinates[key * 2 + 1]);
			_keyTimes[key] = 0;
			_updateTexture = true;
			_soundStartTime = now;

			if (_keyPos < 5)
			{
				if (key == 10)
				{
					_enteredCode[_keyPos] = -1;
				}
				else
				{
					_enteredCode[_keyPos] = key;
				}
				_keyPos++;
			}

			if (key != 10)
			{
				_inputEnabled = true;
			}
			else
			{
				_codeCorrect = true;
				for (int i = 0; i < 5; i++)
				{
					if (_enteredCode[i] != DoorCode[i])
					{
						_codeCorrect = false;
						break;
					}
				}

				_blinkFrame = 18;
				_blinkFrameTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

				if (_codeCorrect)
				{
					CGameController::SetParameter(205, 1);
				}
			}

			break;
		}
	}

	if (_blinkFrame > 0)
	{
		if ((now - _blinkFrameTime) > 166)
		{
			_blinkFrame--;
			if (_blinkFrame > 3 && _blinkFrame < 16)
			{
				if ((_blinkFrame & 1) == 0)
				{
					_sound.Play(_files[_codeCorrect ? SND_GOOD : SND_BAD]);
				}

				RenderItem((_codeCorrect ? IMG_GREEN : IMG_RED) + (_blinkFrame & 1), KeyRenderCoordinates[(12 - _codeCorrect) * 2 + 0], KeyRenderCoordinates[(12 - _codeCorrect) * 2 + 1]);

				_updateTexture = true;
			}

			_blinkFrameTime = now;

			if (_blinkFrame == 0)
			{
				if (_codeCorrect)
				{
					return CModuleController::Pop(this);
				}
				else
				{
					ResetCode();
					_inputEnabled = true;
				}
			}
		}
	}

	if (_updateTexture)
	{
		UpdateTexture();
		_updateTexture = false;
	}

	if (_vertexBuffer != NULL)
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

		CModuleController::Cursors[(int)CAnimatedCursor::CursorType::Crosshair].SetPosition(_cursorPosX, _cursorPosY);
		CModuleController::Cursors[(int)CAnimatedCursor::CursorType::Crosshair].Render();

		dx.EnableZBuffer();
	}
}

void CPDRitzSecurityKeypadModule::Initialize()
{
	CFullScreenModule::Initialize();

	DoubleData dd = LoadDoubleEntry("SPECIAL.AP", 57);
	if (dd.File1.Data != NULL)
	{
		_screen = dd.File2.Data;

		uint8_t* pPal = dd.File1.Data;
		ReadPalette(pPal);

		delete[] pPal;
	}

	BinaryData bd = LoadEntry("SPECIAL.AP", 59);
	if (bd.Data != NULL)
	{
		_data = bd.Data;
		int count = GetInt(_data, 0, 2) - 1;
		for (int i = 0; i < count; i++)
		{
			_files[i] = _data + GetInt(_data, 2 + i * 4, 4);
		}
	}

	_cursorMinX = static_cast<int>(_left + 3 * _scale);
	_cursorMaxX = static_cast<int>(_left + 628 * _scale);
	_cursorMinY = static_cast<int>(-_top + 4 * _scale);
	_cursorMaxY = static_cast<int>(-_top + 468 * _scale);

	_updateTexture = true;
}

void CPDRitzSecurityKeypadModule::Dispose()
{
	CModuleController::Cursors[(int)CAnimatedCursor::CursorType::Crosshair].SetPosition(dx.GetWidth() / 2.0f, dx.GetHeight() / 2.0f);
}

void CPDRitzSecurityKeypadModule::KeyDown(int key, int lParam)
{
	if (_inputEnabled)
	{
		if (key >= '0' && key <= '9')
		{
			// Enter code
			Key(static_cast<int>(key - '0'));
		}
		else if (key == VK_RETURN)
		{
			// Try code
			Key(10);
		}
	}
}

void CPDRitzSecurityKeypadModule::BeginAction()
{
	if (_inputEnabled)
	{
		int x = static_cast<int>((_cursorPosX - _left) / _scale);
		int y = static_cast<int>((_cursorPosY - _top) / _scale);

		// Check hit area
		for (int key = 0; key < 12; key++)
		{
			if (x >= KeyHitTestCoordinates[key * 4 + 2] && x < KeyHitTestCoordinates[key * 4 + 3] && y >= KeyHitTestCoordinates[key * 4] && y < KeyHitTestCoordinates[key * 4 + 1])
			{
				Key(key);
				break;
			}
		}
	}
}

void CPDRitzSecurityKeypadModule::ResetCode()
{
	for (int i = 0; i < 5; i++)
	{
		_enteredCode[i] = -1;
	}

	_keyPos = 0;
}

void CPDRitzSecurityKeypadModule::Key(int key)
{
	if (key < 11)
	{
		_inputEnabled = false;
		_keyTimes[key] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		RenderItem(key * 2 + 1, KeyRenderCoordinates[key * 2], KeyRenderCoordinates[key * 2 + 1]);
		_updateTexture = true;
	}
	else if (key == 11)
	{
		CModuleController::Pop(this);
	}
}
