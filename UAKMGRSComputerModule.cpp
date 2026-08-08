#include "UAKMGRSComputerModule.h"
#include "Utilities.h"
#include "GameController.h"
#include "ConstantBuffers.h"
#include "Shaders.h"
#include <chrono>
#include <cstring>

#define VK_SPACE 0x20

int GRS_Page[] = { 2, 3, 4, 5, 7, 9 };

CUAKMGRSComputerModule::CUAKMGRSComputerModule() : CFullScreenModule(ModuleType::GRSComputer)
{
	_animation = nullptr;
	_animationLength = 0;

	_animationPointer = nullptr;
	_animationFrames = 0;
	_animationWidth = 0;
	_animationHeight = 0;
	_animationActive = false;

	_previousPage = 0;

	_inputEnabled = false;
}

CUAKMGRSComputerModule::~CUAKMGRSComputerModule()
{
	Dispose();
}

void CUAKMGRSComputerModule::Dispose()
{
	CFullScreenModule::Dispose();

	if (_animation != nullptr)
	{
		delete[] _animation;
		_animation = nullptr;
	}

	_animationLength = 0;
}

void CUAKMGRSComputerModule::Render()
{
	bool popOnEnd = false;

	uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	auto delta = now - _frameTime;

	if (_currentPage == 0)
	{
		if (_currentFrame == 0 && delta >= 1000)
		{
			_currentFrame++;
			_frameTime = now;
		}
		else if (_currentFrame >= 1 && _currentFrame <= 10 && delta >= 10)
		{
			FadeOut(244, 256, 1, 10);
		}
		else if (_currentFrame == 11)
		{
			RenderItem(2, 123, 226);
			_currentFrame++;
			UpdateTexture();
		}
		else if (_currentFrame >= 12 && _currentFrame <= 27 && delta >= 6 * TIMER_SCALE)
		{
			for (int i = 244; i < 256; i++)
			{
				_palette[i] = (_currentFrame & 1) ? 0xff000000 : _originalPalette[i];
			}

			UpdateTexture();
			_currentFrame++;
			_frameTime = now;
		}
		else if (_currentFrame == 28)
		{
			int len = GetInt(_data, 14, 4) - GetInt(_data, 10, 4);
			BinaryData bd = CLZ::Decompress(_files[3], 0, len);
			RenderRaw(bd.Data, 0x6e, 0x5f, 420, 299);
			delete[] bd.Data;
			UpdateTexture();
			_currentFrame++;
		}
		else if (_currentFrame >= 29 && _currentFrame <= 38 && delta >= 10)
		{
			FadeIn(244, 256, 29, 38);
		}
		else if (_currentFrame == 39 && delta >= 1000)
		{
			_currentPage = 1;
			_currentFrame = 0;
		}
		else if (_currentFrame >= 40 && _currentFrame <= 49 && delta >= 10)
		{
			// Render, fade in, wait 1 sec, fade out (fade out could be on page handler)
		}
	}
	else if (_currentPage >= 1 && _currentPage < 100)
	{
		if (_currentFrame >= 0 && _currentFrame <= 9)
		{
			FadeOut(244, 256, 0, 9);
		}
		else if (_currentFrame == 10)
		{
			if (_currentPage == 99)
			{
				popOnEnd = true;
			}
			else
			{
				int page = 14 + _currentPage;
				int len = GetInt(_data, 6 + page * 4, 4) - GetInt(_data, 2 + page * 4, 4);
				BinaryData bd = CLZ::Decompress(_files[page], 0, len);
				RenderRaw(bd.Data, 0x6e, 0x5f, 420, 299);
				delete[] bd.Data;

				uint8_t* pageData = _files[0] + (_currentPage - 1) * 28;
				_cursorMinY = static_cast<int>(GetInt(pageData, 20, 2) * _scale - _top);
				_cursorMaxY = static_cast<int>(GetInt(pageData, 22, 2) * _scale - _top);
				_cursorMinX = static_cast<int>(GetInt(pageData, 24, 2) * _scale + _left);
				_cursorMaxX = static_cast<int>(GetInt(pageData, 26, 2) * _scale + _left);

				if (_currentPage == 1)
				{
					uint8_t* extraData = pageData + GetInt(pageData, 0, 4);
					_cursorPosX = GetInt(extraData, _previousPage * 8, 2) * _scale + _left;
					_cursorPosY = GetInt(extraData, _previousPage * 8 + 2, 2) * _scale - _top;
				}

				RenderButton(GetInt(pageData, 10, 2), 338, 8);
				RenderButton(GetInt(pageData, 12, 2), 370, 6);
				RenderButton(GetInt(pageData, 14, 2), 370, 10);
				RenderButton(GetInt(pageData, 16, 2), 370, 4);
				RenderButton(GetInt(pageData, 18, 2), 370, 12);

				UpdateTexture();
				_currentFrame++;
			}
		}
		else if (_currentFrame >= 11 && _currentFrame <= 20)
		{
			FadeIn(244, 256, 11, 20);
		}
		else if (_currentFrame == 21)
		{
			_inputEnabled = true;
			_currentFrame++;
		}
	}
	else if (_currentPage == 100 && delta >= 4 * TIMER_SCALE)
	{
		if (_animationPointer != nullptr && _animationActive)
		{
			int chunkSize = GetInt(_animationPointer, 0, 2);
			_animationPointer += 2;
			uint8_t* nextFrame = _animationPointer + chunkSize;

			if (_currentFrame == 0)
			{
				for (int y = 0; y < _animationHeight; y++)
				{
					for (int x = 0; x < _animationWidth; x++)
					{
						_screen[(118 + y) * 640 + 197 + x] = *(_animationPointer++);
					}
				}
			}
			else if (_currentFrame < _animationFrames)
			{
				int x = 0;
				int y = 0;
				while (chunkSize > 0)
				{
					int b = *(_animationPointer++);
					chunkSize--;
					if ((b & 0x80) != 0)
					{
						x += (b & 0x7f);
						while (x >= _animationWidth)
						{
							y++;
							x -= _animationWidth;
						}
					}
					else
					{
						for (int i = 0; i < b; i++)
						{
							_screen[(118 + y) * 640 + 197 + x++] = *(_animationPointer++);
							if (x >= _animationWidth)
							{
								x = 0;
								y++;
							}

							chunkSize--;
						}
					}
				}
			}

			UpdateTexture();

			_animationPointer = nextFrame;
			_currentFrame++;
			if ((_animationPointer - _animation) >= _animationLength)
			{
				_currentPage = 101;
				_currentFrame = 0;
				_animationActive = false;
			}

			_frameTime = now;
		}
	}
	else if (_currentPage == 101 && delta >= 1000)
	{
		_currentPage = 5;

		int page = 14 + _currentPage;
		int len = GetInt(_data, 6 + page * 4, 4) - GetInt(_data, 2 + page * 4, 4);
		BinaryData bd = CLZ::Decompress(_files[page], 0, len);
		RenderRaw(bd.Data, 0x6e, 0x5f, 420, 299);
		delete[] bd.Data;

		uint8_t* pageData = _files[0] + (_currentPage - 1) * 28;
		RenderButton(GetInt(pageData, 10, 2), 338, 8);
		RenderButton(GetInt(pageData, 12, 2), 370, 6);
		RenderButton(GetInt(pageData, 14, 2), 370, 10);
		RenderButton(GetInt(pageData, 16, 2), 370, 4);
		RenderButton(GetInt(pageData, 18, 2), 370, 12);

		UpdateTexture();

		_currentFrame = 22;
		_inputEnabled = true;
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

		if (_inputEnabled)
		{
			dx.SetVertexBuffers(0, 1, &_iconVertexBuffer, &stride, &offset);
			dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			wm = Math::Translation(_cursorPosX, -_cursorPosY, -0.5f);
			CConstantBuffers::SetWorld(dx, &wm);
			pRV = _iconTexture.GetTextureRV();
			dx.SetShaderResources(0, 1, &pRV);
			dx.Draw(4, 0);
		}

		dx.EnableZBuffer();
	}

	if (popOnEnd)
	{
		CModuleController::Pop(this);
	}
}

void CUAKMGRSComputerModule::KeyDown(int key, int lParam)
{
	if (_currentPage == 100 && key == VK_SPACE)
	{
		_animationActive = !_animationActive;
	}
}

void CUAKMGRSComputerModule::Initialize()
{
	CFullScreenModule::Initialize();

	DoubleData dd = LoadDoubleEntry("DUBCOMP.AP", 0);
	if (dd.File1.Data != nullptr)
	{
		_screen = dd.File2.Data;

		uint8_t* pPal = dd.File1.Data;
		ReadPalette(pPal);

		_palette[0x24] = 0xff000000;

		memcpy(_originalPalette, _palette, sizeof(int) * 256);

		delete[] pPal;
	}

	dd = LoadDoubleEntry("DUBCOMP.AP", 2);
	if (dd.File1.Data != nullptr)
	{
		_data = dd.File1.Data;
		int count = GetInt(_data, 0, 2) - 1;
		for (int i = 0; i < count; i++)
		{
			_files[i] = _data + GetInt(_data, 2 + i * 4, 4);
		}

		CreateTexturedRectangle(0.0f, 0.0f, -16.0f, 16.0f, &_iconVertexBuffer, "FullScreenIconVertexBuffer");

		_animation = dd.File2.Data;
		_animationLength = dd.File2.Length;

		_iconTexture.Init(_files[14], 0, 0, &_palette[0], 0, "FullScreenIconTexture");
	}

	RenderItem(1, 139, 226);

	UpdateTexture();

	_currentPage = 0;
	_currentFrame = 0;
	_frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	_inputEnabled = false;
}

void CUAKMGRSComputerModule::RenderButton(int x, int y, int image)
{
	if (x > 0)
	{
		RenderItem(image, x, y);
	}
}

void CUAKMGRSComputerModule::BeginAction()
{
	if (_inputEnabled && _currentPage < 99)
	{
		int x = static_cast<int>((_cursorPosX - _left) / _scale);
		int y = static_cast<int>((_cursorPosY - _top) / _scale);

		uint8_t* data = _files[0];
		uint8_t* pageData = data + (_currentPage - 1) * 28;
		uint8_t* buttonTable = data + GetInt(pageData, 4, 4);
		while (*buttonTable != 0xff)
		{
			if (y >= GetInt(buttonTable, 2, 2) && y < GetInt(buttonTable, 4, 2) && x >= GetInt(buttonTable, 6, 2) && x < GetInt(buttonTable, 8, 2))
			{
				int function = buttonTable[11];
				if (function > 0)
				{
					if (function == 5)
					{
						_currentPage = 99;
						_currentFrame = 0;
					}
					else if (function == 6)
					{
						_previousPage = GetInt(pageData, 9, 1);
						_currentPage = 1;
						_currentFrame = 0;
					}
					else if (function == 7)
					{
						_currentPage++;
						_currentFrame = 0;
					}
					else if (function == 8)
					{
						_currentPage--;
						_currentFrame = 0;
					}
					else if (function == 9)
					{
						_currentPage = 100;
						_currentFrame = 0;
						_animationFrames = GetInt(_animation, 0, 2);
						_animationWidth = GetInt(_animation, 2, 2);
						_animationHeight = GetInt(_animation, 4, 2);
						_animationPointer = _animation + 8;
						_inputEnabled = false;
						_animationActive = true;
					}
				}
				else
				{
					_currentPage = 1 + GRS_Page[buttonTable[10]];
					_currentFrame = 0;
				}

				break;
			}

			buttonTable += 14;
		}
	}
	else if (_currentPage == 100)
	{
		_animationActive = !_animationActive;
	}
}

void CUAKMGRSComputerModule::Back()
{
	if (_inputEnabled && _currentPage < 99)
	{
		_currentPage = 99;
		_currentFrame = 0;
	}
	else if (_currentPage == 100)
	{
		_currentPage = 101;
		_frameTime = 0;
	}
}