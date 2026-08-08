#include "UAKMSafeModule.h"
#include "Globals.h"
#include "GameController.h"
#include "Utilities.h"
#include "UAKMGame.h"
#include "AmbientAudio.h"
#include "ConstantBuffers.h"
#include "Shaders.h"
#include <chrono>
#include <cstring>
#include <algorithm>

#define VK_RETURN       0x0D
#define VK_MULTIPLY     0x6A
#define VK_NUMPAD0      0x60
#define VK_NUMPAD1      0x61
#define VK_NUMPAD9      0x69
#define VK_HOME         0x24

int startupAnimOffsets[] = { 450, 450, 468, 487, 501, 517, 527 };

int handleAnimOffsets[] = { 62, 1, 53, 5, 37, 0, 34, 6, 26, 6, 22, 6, 13, 6, 5, 0, 5, 3, 5, 6 };

int keyLocations[] = { 493, 280, 461, 200, 493, 200, 524, 200, 461, 226, 493, 226, 524, 226, 461, 253, 493, 253, 524, 253, 461, 280, 524, 280, 493, 307, 493, 334 };

signed char eddieChingsSafeCode[] = { 1, 0, 1, 4, 1, 2, -1, -1 };
signed char grsSafeCode[] = { 1, 4, 2, 2, 3, 5, -1, -1 };

uint8_t safeLightColours[] = { 161, 171, 150, 178, 188, 150, 195, 205, 150, 211, 222, 179, 227, 238, 160 };

int keyCoordinates[] = {
335,358,592,639,
201,221,462,489,
201,221,494,520,
201,221,526,553,
227,248,462,489,
227,248,494,520,
227,248,526,553,
253,274,462,489,
253,274,494,520,
253,274,526,553,
280,301,462,489,
280,301,494,520,
280,301,526,553,
307,331,494,553,
334,358,494,553 };

CUAKMSafeModule::CUAKMSafeModule(int parameter, bool alternatePalette) : CModuleBase(ModuleType::UltraSafe)
{
	_rollingLightTime = 0;
	_openSafeSequence = -1;

	_parameter = parameter;
	_alternatePalette = alternatePalette;

	_screen = nullptr;

	float width = 640.0f, height = 400.0f;
	float screenWidth = (float)dx.GetWidth();
	float screenHeight = (float)dx.GetHeight();

	float sx = screenWidth / width;
	float sy = screenHeight / height;
	_scale = std::min(sx, sy);
	float sw = width * _scale;
	float sh = height * _scale;
	float ox = (screenWidth - sw);
	float oy = (screenHeight - sh);

	_left = (float)(ox / 2.0f);
	_top = (float)(-oy / 2.0f);
	_right = _left + width * _scale;
	_bottom = _top - height * _scale;

	_cursorMinX = static_cast<int>(_left + 462 * _scale);
	_cursorMaxX = static_cast<int>(_left + 627 * _scale);
	_cursorMinY = static_cast<int>(-_top + 171 * _scale);
	_cursorMaxY = static_cast<int>(-_top + 317 * _scale);

	_vertexBuffer = nullptr;
	_handVertexBuffer = nullptr;

	_pImages = nullptr;
	_pSounds = nullptr;

	_ready = false;
	_frameDelay = 0;
	_frameTime = 0;

	_flashingLightOn = false;
	_rollingLightPosition = 0;

	_textureDirty = true;

	for (int i = 0; i < 14; i++)
	{
		_keyDown[i] = -1;
		_frameTimes[i] = 0;
	}

	Start();
}

CUAKMSafeModule::~CUAKMSafeModule()
{
	Dispose();
}

void CUAKMSafeModule::Initialize()
{
	_cursorPosX = dx.GetWidth() / 2.0f;
	_cursorPosY = dx.GetHeight() / 2.0f;

	CGameController::SetParameter(_parameter, 0);

	BinaryData bdPal = LoadEntry("SPECIAL.AP", _alternatePalette ? 57 : 53);
	if (bdPal.Data != nullptr)
	{
		for (int c = 0; c < 256; c++)
		{
			double r = bdPal.Data[c * 3 + 0];
			double g = bdPal.Data[c * 3 + 1];
			double b = bdPal.Data[c * 3 + 2];
			int ri = (uint8_t)((r * 255.0) / 63.0);
			int gi = (uint8_t)((g * 255.0) / 63.0);
			int bi = (uint8_t)((b * 255.0) / 63.0);
			int col = 0xff000000 | bi | (gi << 8) | (ri << 16);
			_palette[c] = col;
		}

		delete[] bdPal.Data;
	}

	BinaryData bdScr = LoadEntry("SPECIAL.AP", 54);
	_screen = bdScr.Data;

	BinaryData buttons = LoadEntry("SPECIAL.AP", 55);
	_pImages = buttons.Data;
	int count = GetInt(_pImages, 0, 2) - 1;
	for (int i = 0; i < count; i++)
	{
		_safeImageOffsets[i] = _pImages + GetInt(_pImages, 2 + i * 4, 4);
	}

	BinaryData soundsAP = LoadEntry("SPECIAL.AP", 56);
	_pSounds = soundsAP.Data;
	count = GetInt(_pSounds, 0, 2) - 1;
	for (int i = 0; i < count; i++)
	{
		_safeSoundOffsets[i] = _pSounds + GetInt(_pSounds, 2 + i * 4, 4);
	}

	_texture.Init(640, 400);

	CreateTexturedRectangle(_top, _left, _bottom, _right, &_vertexBuffer, "SafeVertexBuffer");

	_handTexture.Init(_safeImageOffsets[47], 0, 0, &_palette[0], 0, "SafeHandTexture");
	CreateTexturedRectangle(0.0f, 0.0f, -16.0f, 16.0f, &_handVertexBuffer, "SafeHandVertexBuffer");

	if (_alternatePalette)
	{
		PartialRender(48, 307, 41, false);
	}

	_textureDirty = true;

	_frameDelay = (uint64_t)(30 * TIMER_SCALE);
	_frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	_startupFrame = 2;
}

void CUAKMSafeModule::Dispose()
{
	if (_screen != nullptr)
	{
		delete[] _screen;
		_screen = nullptr;
	}

	_safeImageOffsets.clear();
	_safeSoundOffsets.clear();

	if (_pImages != nullptr)
	{
		delete[] _pImages;
		_pImages = nullptr;
	}

	if (_pSounds != nullptr)
	{
		delete[] _pSounds;
		_pSounds = nullptr;
	}

	if (_vertexBuffer != nullptr)
	{
		_vertexBuffer->Release();
		_vertexBuffer = nullptr;
	}

	if (_handVertexBuffer != nullptr)
	{
		_handVertexBuffer->Release();
		_handVertexBuffer = nullptr;
	}
}

void CUAKMSafeModule::Render()
{
	if (_vertexBuffer != nullptr)
	{
		dx.DisableZBuffer();

		CConstantBuffers::Setup2D(dx);

		uint64_t tick = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

		if (_startupFrame < 9)
		{
			uint64_t diff = tick - _frameTime;
			if (diff >= _frameDelay)
			{
				PartialRender(_startupFrame, startupAnimOffsets[_startupFrame - 2], 150, true);
				_startupFrame++;
				_frameTime = tick;
				_frameDelay = (uint64_t)(2 * TIMER_SCALE);
			}
		}
		else if (_openSafeSequence >= 0)
		{
			uint64_t diff = tick - _frameTime;
			if (diff >= _frameDelay)
			{
				_frameTime = tick;
				_openSafeSequence++;

				if (_openSafeSequence >= 1 && _openSafeSequence <= 5)
				{
					if (_openSafeSequence == 4 && !_codeCorrect)
					{
						return Exit();
					}

					_frameDelay = (uint64_t)(20 * TIMER_SCALE);

					int y1 = safeLightColours[(_openSafeSequence - 1) * 3] - 30;
					int y2 = safeLightColours[(_openSafeSequence - 1) * 3 + 1] - 30;
					uint8_t colour = safeLightColours[(_openSafeSequence - 1) * 3 + 2];
					int x1 = 344;
					int x2 = 365;
					for (int y = y1; y < y2; y++)
					{
						for (int x = x1; x < x2; x++)
						{
							_screen[y * 640 + x] = colour;
						}
					}
				}
				else if (_openSafeSequence == 30)
				{
					CGameController::SetParameter(_parameter, 1);
					return Exit();
				}
				else if (_openSafeSequence > 5 && _openSafeSequence < 16)
				{
					int frame = _openSafeSequence - 6;
					int x = handleAnimOffsets[frame * 2] + 290;
					int y = handleAnimOffsets[frame * 2 + 1] + 235;
					PartialRender(9 + frame, x, y, true);
					_frameDelay = (uint64_t)(2 * TIMER_SCALE);
				}
			}
		}
		else
		{
			bool keyWasDown = false;
			for (int i = 0; i < 14; i++)
			{
				if (_keyDown[i] == 1)
				{
					auto duration = tick - _frameTimes[i];
					if (duration >= (uint64_t)(12 * TIMER_SCALE))
					{
						PartialRender(19 + i * 2, keyLocations[i * 2], keyLocations[i * 2 + 1] - 30, true);

						if (i >= 0 && i < 12)
						{
							Number(i);
							_keyDown[i] = -1;
						}
						else if (i == 12)
						{
							Start();
							_keyDown[i] = -1;
						}
						else if (i == 13)
						{
							Enter();
						}
					}

					keyWasDown = true;
				}
			}

			if (!keyWasDown && !_ready)
			{
				_ready = true;
			}
		}

		if (_startupFrame == 9)
		{
			bool newState = (tick / 250) & 1;
			if (newState != _flashingLightOn)
			{
				PartialRender(newState ? 1 : 0, 461, 84, false);
				_flashingLightOn = newState;
				_textureDirty = true;
			}

			if (tick - _rollingLightTime >= (uint64_t)TIMER_SCALE)
			{
				_rollingLightTime = tick;

				for (int y = 0; y < 7; y++)
				{
					for (int x = 0; x < 3; x++)
					{
						_screen[(335 + y) * 640 + 468 + _rollingLightPosition + x] = 0;
					}
				}

				_rollingLightPosition += 2;
				if (_rollingLightPosition >= 78)
				{
					_rollingLightPosition = 0;
				}

				for (int y = 0; y < 7; y++)
				{
					for (int x = 0; x < 3; x++)
					{
						_screen[(335 + y) * 640 + 468 + _rollingLightPosition + x] = 0x96;
					}
				}

				_textureDirty = true;
			}
		}

		if (_textureDirty)
		{
			UpdateTexture();
		}

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

		if (_ready || _openSafeSequence >= 0)
		{
			dx.SetVertexBuffers(0, 1, &_handVertexBuffer, &stride, &offset);
			wm = Math::Translation(_cursorPosX, -_cursorPosY, 0.0f);
			CConstantBuffers::SetWorld(dx, &wm);
			pRV = _handTexture.GetTextureRV();
			dx.SetShaderResources(0, 1, &pRV);
			dx.Draw(4, 0);
		}

		dx.EnableZBuffer();
	}
}

void CUAKMSafeModule::KeyDown(int key, int lParam)
{
	if (key == '0' || key == VK_NUMPAD0)
	{
		Press(0, 11);
	}
	else if (key == VK_MULTIPLY)
	{
		Press(11, 11);
	}
	else if (key >= '1' && key <= '9')
	{
		Press(static_cast<int>(key - '0'), static_cast<int>(key - '1'));
	}
	else if (key >= VK_NUMPAD1 && key <= VK_NUMPAD9)
	{
		Press(static_cast<int>(key - VK_NUMPAD0), static_cast<int>(key - VK_NUMPAD1));
	}
	else if (key == VK_HOME)
	{
		Press(12, 13);
	}
	else if (key == VK_RETURN)
	{
		Press(13, 14);
	}
}

void CUAKMSafeModule::UpdateTexture()
{
	ID3D11Texture2D* pTex = _texture.GetTexture();
	if (pTex != nullptr)
	{
		D3D11_MAPPED_SUBRESOURCE subRes;
		if (dx.Map(pTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes) == 0)
		{
			int width = _texture.Width();
			int height = _texture.Height();

			int* pScr = (int*)subRes.pData;
			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					pScr[y * subRes.RowPitch / 4 + x] = _palette[_screen[y * width + x]];
				}
			}

			dx.Unmap(pTex, 0);
		}
	}
}

void CUAKMSafeModule::PartialRender(int entry, int offsetX, int offsetY, bool updateTexture)
{
	uint8_t* data = _safeImageOffsets[entry];
	if (data != nullptr && GetInt(data, 0, 2) == 0x100)
	{
		int width = GetInt(data, 2, 2);
		int height = GetInt(data, 4, 2);
		int inPtr = 16;

		for (int y = 0; y < height; y++)
		{
			int c1 = GetInt(data, inPtr, 2);
			int c2 = GetInt(data, inPtr + 2, 2);
			memcpy(_screen + c1 + (y + offsetY) * 640 + offsetX, data + inPtr + 4, c2);
			inPtr += 4 + c2;
		}
	}

	_textureDirty = true;
}

void CUAKMSafeModule::Start()
{
	_codeCorrect = false;

	for (int i = 0; i < 8; i++)
	{
		_enteredCode[i] = -1;
	}

	_keyPos = 0;
}

void CUAKMSafeModule::Enter()
{
	uint8_t* pCorrectCode = reinterpret_cast<uint8_t*>(_alternatePalette ? eddieChingsSafeCode : grsSafeCode);
	_codeCorrect = true;
	for (int i = 0; i < 8; i++)
	{
		if (pCorrectCode[i] != _enteredCode[i])
		{
			_codeCorrect = false;
			break;
		}
	}

	_openSafeSequence = 0;
	_frameDelay = (uint64_t)(60 * TIMER_SCALE);
	_frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	_ready = false;
}

void CUAKMSafeModule::Number(int number)
{
	if (_keyPos < 8)
	{
		_enteredCode[_keyPos++] = (uint8_t)number;
	}
}

void CUAKMSafeModule::Exit()
{
	CModuleController::Pop(this);
}

void CUAKMSafeModule::Press(int key, int sound)
{
	_keyDown[key] = 1;

	PartialRender(19 + key * 2 + 1, keyLocations[key * 2 + 0], keyLocations[key * 2 + 1] - 30, true);

	_frameDelay = (uint64_t)(12 * TIMER_SCALE);
	_frameTimes[key] = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());

	_sound.Play(_safeSoundOffsets[sound % 12]);
}

void CUAKMSafeModule::Resize(int width, int height)
{
}

void CUAKMSafeModule::BeginAction()
{
	if (_ready)
	{
		float x = (_cursorPosX - _left) / _scale;
		float y = (_cursorPosY + _top) / _scale;

		for (int i = 0; i < 15; i++)
		{
			if (x >= keyCoordinates[i * 4 + 2] && x < keyCoordinates[i * 4 + 3] && y >= (keyCoordinates[i * 4 + 0] - 30) && y < (keyCoordinates[i * 4 + 1] - 30))
			{
				if (i == 0)
				{
					Exit();
				}
				else
				{
					int key = i;
					if (key == 11) key = 0;
					else if (key > 11) key--;

					Press(key, i - 1);
				}

				break;
			}
		}
	}
}

void CUAKMSafeModule::Back()
{
	Exit();
}
