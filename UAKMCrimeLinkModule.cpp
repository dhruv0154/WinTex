#include "UAKMCrimeLinkModule.h"
#include "Utilities.h"
#include "LZ.h"
#include "GameBase.h"
#include "GameController.h"
#include "AnimationController.h"
#include "UAKMGame.h"
#include "ConstantBuffers.h"
#include "Shaders.h"
#include <chrono>
#include <cstring>

#define CL_PAGE_1               35
#define CL_PAGE_2               37
#define CL_PAGE_3               39
#define CL_PAGE_MATCH           41
#define CL_PAGE_COMPILING       42
#define CL_PAGE_FLEMM1          44
#define CL_PAGE_FLEMM2          46
#define CL_PAGE_FLEMM3          48

short CUAKMCrimeLinkModule::CorrectSelections[15] = { 1,1,0x10,0x40,2,2,2,8,0x20,0x100,0,2,0,0,0 };
short CUAKMCrimeLinkModule::PlayerSelections[15] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

short CUAKMCrimeLinkModule::Animation[CL_ANIMATION_FRAMES * 4] = { 0xA7, 0xDA, 5, 6,
																0xA7, 0xD1, 6, 6,
																0x73, 0xF1, 7, 6,
																0x73, 0xE3, 8, 6,
																0x73, 0xB7, 8, 6,
																0x73, 0x61, 8, 6,
																0x73, 0x151, 8, 6,
																0x73, 0xFE, 8, 6,
																0x73, 0xA9, 8, 6,
																0x73, 0x69, 8, 6,
																-1, -1, -1, 0x1E,
																0x9D, 0xF0, 9, 6,
																0x9D, 0xC4, 0xA, 6,
																0x9D, 0x89, 0xB, 0x14,
																0xEE, 0x92, 0xC, 0xA,
																0xF7, 0xB0, 0xD, 4,
																0xC3, 0xD3, 0xE, 4,
																0x118, 0xEB, 0xF, 4,
																0x118, 0xEB, 0x10, 4,
																0xDF, 0x11C, 0x11, 4,
																0xCD, 0x130, 0x12, 0x2D,
																0xA0, 0x8C, 0x13, 0x2D,
																0xA0, 0x8C, 0x14, 0x3C,
																0x9D, 0x89, 0x15, 0x3C,
																0x6E, 0x5A, 0x16, 0x1E,
																0x190, 0x8C, 0x17, 6,
																0x185, 0x8C, 0x18, 6,
																0x179, 0x8C, 0x19, 6,
																0x16E, 0x8C, 0x1A, 6,
																0x162, 0x8C, 0x1B, 6,
																0x155, 0x8C, 0x1C, 6,
																0x148, 0x8C, 0x1D, 6,
																0x144, 0x8C, 0x1E, 0x2D,
																0xA0, 0x8C, 0x1F, 0x3C,
																0xE7, 0xBC, 0x20, 0x3C,
																0xDA, 0xAD, 0x21, 0x3C };

CUAKMCrimeLinkModule::CUAKMCrimeLinkModule(int parameter) : CFullScreenModule(ModuleType::CrimeLink)
{
	_animationFrameTime = 0;
	_animationFrameDuration = 0;
	_animationIndex = 0;
	_windowX = 115;
	_windowY = 90;

	_page = -1;

	_lastMouseOver = { -1, -1 };

	_parameter = parameter;

	_flashFrame = 0;

	for (int i = 0; i < 15; i++)
	{
		PlayerSelections[i] = CGameController::GetData(UAKM_SAVE_CRIMELINK_SELECTIONS + i * 2) | CGameController::GetData(UAKM_SAVE_CRIMELINK_SELECTIONS + i * 2 + 1) << 8;
	}

	_pageAnswerOffset = 0;
}

CUAKMCrimeLinkModule::~CUAKMCrimeLinkModule()
{
	Dispose();
}

void CUAKMCrimeLinkModule::Initialize()
{
	CFullScreenModule::Initialize();

	CGameController::SetParameter(_parameter, 0);

	BinaryData bdPal = LoadEntry("SPECIAL.AP", 3);
	if (bdPal.Data != nullptr)
	{
		ReadPalette(bdPal.Data);
		delete[] bdPal.Data;
	}

	BinaryData bdScr = LoadEntry("SPECIAL.AP", 4);
	_screen = bdScr.Data;

	for (int i = 0; i < CL_ANIMATION_FRAMES; i++)
	{
		int entry = Animation[i * 4 + 2];
		if (entry >= 0)
		{
			BinaryData bd = LoadEntry("SPECIAL.AP", entry);
			_files[entry] = bd.Data;
		}
	}

	for (int i = 34; i < 53; i++)
	{
		BinaryData bd = LoadEntry("SPECIAL.AP", i);
		_files[i] = bd.Data;
	}
}

void CUAKMCrimeLinkModule::PartialRender(int entry, int offsetX, int offsetY, bool updateTexture)
{
	uint8_t* data = _files[entry];
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

	if (updateTexture)
	{
		UpdateTexture();
	}
}

void CUAKMCrimeLinkModule::Render()
{
	CAnimationController::UpdateAndRender();

	uint64_t tick = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	if (_page < 0)
	{
		uint64_t diff = tick - _animationFrameTime;
		if (diff >= _animationFrameDuration)
		{
			int offsetX = Animation[_animationIndex + 0];
			int offsetY = Animation[_animationIndex + 1];
			int entry = Animation[_animationIndex + 2];
			int duration = Animation[_animationIndex + 3];

			PartialRender(entry, offsetX, offsetY, true);

			if (_animationIndex >= 8 && _animationIndex <= 36)
			{
				for (int y = 0; y <= 16; y++)
				{
					memset(_screen + (offsetY + y) * 640 + 115, 0, 413);
				}
			}

			_animationIndex += 4;

			_animationFrameDuration = static_cast<uint64_t>(TIMER_SCALE * duration);
			_animationFrameTime = tick;

			if (_animationIndex >= CL_ANIMATION_FRAMES * 4)
			{
				_page = 0;
			}
		}
	}

	if (_page == 0)
	{
		uint64_t diff = tick - _animationFrameTime;
		if (diff >= _animationFrameDuration)
		{
			ShowPage1();
		}
	}

	if (_page == 5)
	{
		uint64_t diff = tick - _animationFrameTime;
		if (diff >= _animationFrameDuration)
		{
			_animationFrameTime = tick;
			_flashFrame--;
			if (_flashFrame < 0)
			{
				ShowPage6();
			}
			else
			{
				if ((_flashFrame & 1) == 0)
				{
					CFullScreenModule::ReplaceColour(240, 208, 397, 228, 0xb0, 0xaf);
				}
				else
				{
					PartialRender(CL_PAGE_COMPILING, _windowX, _windowY, false);
				}

				UpdateTexture();
			}
		}
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

		if (_page > 0 && _page <= 8 && _page != 5)
		{
			CModuleController::Cursors[0].SetPosition(_cursorPosX, _cursorPosY);
			CModuleController::Cursors[0].Render();
		}

		dx.EnableZBuffer();
	}
}

void CUAKMCrimeLinkModule::Pause()
{
	for (int i = 0; i < 15; i++)
	{
		CGameController::SetData(UAKM_SAVE_CRIMELINK_SELECTIONS + i * 2, PlayerSelections[i] & 0xFF);
		CGameController::SetData(UAKM_SAVE_CRIMELINK_SELECTIONS + i * 2 + 1, PlayerSelections[i] >> 8);
	}
	UnsetCursorClipping();
}

void CUAKMCrimeLinkModule::SetCursorArea(int x1, int y1, int x2, int y2)
{
	_lastMouseOver = { -1, -1 };

	_cursorMinX = static_cast<int>(_left + x1 * _scale);
	_cursorMaxX = static_cast<int>(_left + x2 * _scale);
	_cursorMinY = static_cast<int>(_top + y1 * _scale);
	_cursorMaxY = static_cast<int>(_top + y2 * _scale);
	SetCursorClipping(_cursorMinX, _cursorMinY, _cursorMaxX, _cursorMaxY);
}

std::pair<int, int> CUAKMCrimeLinkModule::GetMouseOver()
{
	std::pair<int, int> pt(-1, -1);

	int scaledX = static_cast<int>((_cursorPosX - _left) / _scale);
	int scaledY = static_cast<int>((_cursorPosY - _top) / _scale);

	int c = 0;
	while (_areaData.Table4[c].unk1 != 0xff && (_areaData.Table4[c].unk1 != 0x2f || CheckSelection()))
	{
		if (scaledY >= _areaData.Table4[c].Y1 && scaledY <= _areaData.Table4[c].Y2 && scaledX >= _areaData.Table4[c].X1 && scaledX <= _areaData.Table4[c].X2)
		{
			pt.first = c;

			for (int h = 0; h < 5; h++)
			{
				if (c >= _areaData.CategoryOptionOffsets[h] && c < _areaData.CategoryOptionOffsets[h + 1])
				{
					pt.second = h;
					break;
				}
			}

			break;
		}

		c++;
	}

	return pt;
}

void CUAKMCrimeLinkModule::DrawRectangle(int entry, uint8_t colour, bool isCategory)
{
	int ax1 = 0, ax2 = 0, ay1 = 0, ay2 = 0;
	if (isCategory && _page != 4)
	{
		ax2 = 1;
		ay1 = 5;
		ay2 = 4;
	}

	CFullScreenModule::DrawRectangle(_areaData.Table4[entry].X1 + ax1, _areaData.Table4[entry].Y1 + ay1, _areaData.Table4[entry].X1 + _areaData.Table2[_areaData.Table4[entry].WidthIndex] - ax2, _areaData.Table4[entry].Y2 - ay2, colour);
}

void CUAKMCrimeLinkModule::ReplaceColour(bool isCategory, int entry, uint8_t src, uint8_t dst)
{
	if (isCategory)
	{
		CFullScreenModule::ReplaceColour(_areaData.CategoryAreas[entry].X1, _areaData.CategoryAreas[entry].Y1, _areaData.CategoryAreas[entry].X2, _areaData.CategoryAreas[entry].Y2, src, dst);
	}
	else
	{
		CFullScreenModule::ReplaceColour(_areaData.Table4[entry].X1 + 1, _areaData.Table4[entry].Y1 + 1, _areaData.Table4[entry].X1 + _areaData.Table2[_areaData.Table4[entry].WidthIndex] - 2, _areaData.Table4[entry].Y2 - 2, src, dst);
	}
}

void CUAKMCrimeLinkModule::ShowPage1()
{
	_windowX = 115;
	_windowY = 90;
	PartialRender(CL_PAGE_1, _windowX, _windowY, false);
	SetCursorArea(118, 88, 514, 379);
	_page = 1;
	_areaData.Init(_files[CL_PAGE_1 - 1]);

	_pageAnswerOffset = 0;

	UpdateSelection();
}

void CUAKMCrimeLinkModule::ShowPage2()
{
	PartialRender(CL_PAGE_2, _windowX, _windowY, false);
	SetCursorArea(118, 88, 514, 379);
	_page = 2;
	_areaData.Init(_files[CL_PAGE_2 - 1]);

	_pageAnswerOffset = 5;

	UpdateSelection();
}

void CUAKMCrimeLinkModule::ShowPage3()
{
	PartialRender(CL_PAGE_3, _windowX, _windowY, false);
	SetCursorArea(118, 88, 514, 379);
	_page = 3;
	_areaData.Init(_files[CL_PAGE_3 - 1]);

	_pageAnswerOffset = 10;

	UpdateSelection();
}

bool CUAKMCrimeLinkModule::CheckSelection()
{
	for (int i = 0; i < 15; i++)
	{
		if (PlayerSelections[i] != CorrectSelections[i])
		{
			return false;
		}
	}

	return true;
}

void CUAKMCrimeLinkModule::Click(int entry, int category)
{
	if (category == -1)
	{
		int test = _areaData.Table4[entry].unk1;
		if (test == 0x2f && CheckSelection())
		{
			ShowPage6();
		}
		else if (test == 0x10)
		{
			return CModuleController::Pop(this);
		}
		else if (test == 0x51)
		{
			if (_page == 1) ShowPage2();
			else if (_page == 2) ShowPage3();
			else if (_page == 6) ShowPage7();
			else if (_page == 7) ShowPage8();
		}
		else if (test == 0x49)
		{
			if (_page == 2) ShowPage1();
			else if (_page == 3) ShowPage2();
			else if (_page == 7) ShowPage6();
			else if (_page == 8) ShowPage7();
		}
		else if (test == 0x31)
		{
			ShowPage1();
		}
		else if (test == 0x15)
		{
			ShowPage5();
		}

		Cursor(_cursorPosX, _cursorPosY, false);
	}
	else
	{
		for (int i = 5; i >= 0; i--)
		{
			if (entry >= _areaData.CategoryOptionOffsets[i])
			{
				int oldAnswer = PlayerSelections[_pageAnswerOffset + category];
				int newAnswer = 1 << (entry - _areaData.CategoryOptionOffsets[i]);

				if (oldAnswer == newAnswer)
				{
					PlayerSelections[_pageAnswerOffset + category] = 0;
				}
				else
				{
					PlayerSelections[_pageAnswerOffset + category] = newAnswer;
					ReplaceColour(false, entry, 0xb1, 0xb0);
				}

				if (oldAnswer != 0)
				{
					int answerIndex = GetAnswerIndex(i, oldAnswer);
					if (answerIndex >= 0)
					{
						ReplaceColour(false, _areaData.CategoryOptionOffsets[i] + answerIndex, 0xb0, 0xb1);
					}
				}

				UpdateSelection();

				if (CheckSelection())
				{
					ShowPage4();
				}
				break;
			}
		}
	}
}

void CUAKMCrimeLinkModule::UpdateSelection()
{
	for (int i = 0; i < 5; i++)
	{
		int answer = PlayerSelections[_pageAnswerOffset + i];
		if (answer > 0)
		{
			int index = GetAnswerIndex(i, answer);
			if (index >= 0)
			{
				ReplaceColour(false, _areaData.CategoryOptionOffsets[i] + index, 0xb1, 0xb0);
			}
		}
	}

	int e = 0;
	while (_areaData.Table4[e].unk1 != 0xff)
	{
		if (_areaData.Table4[e].unk1 == 0x2f)
		{
			uint8_t src = 0xb4, dst = 0;
			if (CheckSelection())
			{
				src = 0;
				dst = 0xb4;
			}

			ReplaceColour(false, e, src, dst);
			break;
		}

		e++;
	}

	CFullScreenModule::ReplaceColour(271, 349, 301, 360, 0xb0, 0xaf);
	if (CheckSelection())
	{
		Render1(279, 349);
	}
	else
	{
		Render1(271, 349);
		Render0(277, 349);
		Render0(286, 349);
		RenderPlus(295, 351);
	}

	UpdateTexture();
}

int CUAKMCrimeLinkModule::GetAnswerIndex(int category, int answer)
{
	int mask = 1;
	for (int j = 0; j < 16; j++)
	{
		if (mask == answer)
		{
			return j;
		}
		mask <<= 1;
	}

	return -1;
}

void CUAKMCrimeLinkModule::ShowPage6()
{
	CGameController::SetParameter(_parameter, CGameController::GetParameter(_parameter) | 1);

	_windowX = 115;
	_windowY = 90;
	PartialRender(CL_PAGE_FLEMM1, _windowX, _windowY, false);
	SetCursorArea(242, 374, 397, 382);
	_page = 6;
	_areaData.Init(_files[CL_PAGE_FLEMM1 - 1]);

	UpdateTexture();
}

void CUAKMCrimeLinkModule::ShowPage7()
{
	CGameController::SetParameter(_parameter, CGameController::GetParameter(_parameter) | 2);

	PartialRender(CL_PAGE_FLEMM2, _windowX, _windowY, false);
	SetCursorArea(242, 374, 397, 382);
	_page = 7;
	_areaData.Init(_files[CL_PAGE_FLEMM2 - 1]);

	UpdateTexture();
}

void CUAKMCrimeLinkModule::ShowPage8()
{
	PartialRender(CL_PAGE_FLEMM3, _windowX, _windowY, false);
	SetCursorArea(242, 374, 397, 382);
	_page = 8;
	_areaData.Init(_files[CL_PAGE_FLEMM3 - 1]);

	UpdateTexture();
}

void CUAKMCrimeLinkModule::Render1(int x, int y)
{
	for (int i = 0; i < 9; i++)
	{
		_screen[(y + i) * 640 + x + 0] = 0xaf;
		_screen[(y + i) * 640 + x + 1] = 0xb0;
		_screen[(y + i) * 640 + x + 2] = 0xb0;
	}
	_screen[(y + 1) * 640 + x] = 0xb0;
}

void CUAKMCrimeLinkModule::Render0(int x, int y)
{
	for (int i = 0; i < 9; i++)
	{
		_screen[(y + i) * 640 + x + 0] = (i == 0 || i == 8) ? 0xaf : 0xb0;
		_screen[(y + i) * 640 + x + 1] = 0xb0;
		_screen[(y + i) * 640 + x + 2] = (i == 0 || i == 8) ? 0xb0 : 0xaf;
		_screen[(y + i) * 640 + x + 3] = (i == 0 || i == 8) ? 0xb0 : 0xaf;
		_screen[(y + i) * 640 + x + 4] = 0xb0;
		_screen[(y + i) * 640 + x + 5] = (i == 0 || i == 8) ? 0xaf : 0xb0;
	}
}

void CUAKMCrimeLinkModule::RenderPlus(int x, int y)
{
	for (int i = 0; i < 5; i++)
	{
		_screen[(y + i) * 640 + x + 2] = 0xb0;
		_screen[(y + 2) * 640 + x + i] = 0xb0;
	}
}

void CUAKMCrimeLinkModule::ShowPage4()
{
	for (int y = 100; y <= 394; y++)
	{
		for (int x = 120; x <= 512; x++)
		{
			_screen[y * 640 + x] = 0;
		}
	}

	_windowX = 240;
	_windowY = 208;
	PartialRender(CL_PAGE_MATCH, _windowX, _windowY, false);
	SetCursorArea(273, 262, 350, 281);
	_page = 4;
	_areaData.Init(_files[CL_PAGE_MATCH - 1]);

	UpdateTexture();
}

void CUAKMCrimeLinkModule::ShowPage5()
{
	for (int y = 90; y <= 390; y++)
	{
		for (int x = 115; x <= 529; x++)
		{
			_screen[y * 640 + x] = 0;
		}
	}

	_windowX = 240;
	_windowY = 208;
	PartialRender(CL_PAGE_COMPILING, _windowX, _windowY, false);
	SetCursorArea(242, 367, 397, 382);
	_page = 5;

	_flashFrame = 11;

	_animationFrameTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	_animationFrameDuration = 150;

	UpdateTexture();
}

void CUAKMCrimeLinkModule::Cursor(float x, float y, bool relative)
{
	CModuleBase::Cursor(x, y, relative);

	if (_page > 0 && _flashFrame <= 0)
	{
		bool update = false;
		std::pair<int, int> pt = GetMouseOver();
		if (pt.first != _lastMouseOver.first)
		{
			if (_lastMouseOver.first >= 0)
			{
				DrawRectangle(_lastMouseOver.first, 0, (_lastMouseOver.second < 0));
			}

			if (pt.first >= 0)
			{
				DrawRectangle(pt.first, 0xb0, (pt.second < 0));
			}

			update = true;
		}

		if (_lastMouseOver.second != pt.second)
		{
			if (_lastMouseOver.second >= 0)
			{
				ReplaceColour(true, _lastMouseOver.second, 0xb0, 0xb1);
			}

			if (pt.second >= 0 && pt.second != _lastMouseOver.second)
			{
				ReplaceColour(true, pt.second, 0xb1, 0xb0);
			}

			update = true;
		}

		_lastMouseOver = pt;

		if (update)
		{
			UpdateTexture();
		}
	}
}

void CUAKMCrimeLinkModule::BeginAction()
{
	if (_page < 0)
	{
		ShowPage1();
	}
	else
	{
		if (_page < 0)
		{
			ShowPage1();
		}
		else if (_page >= 1 && _page <= 8 && _page != 5)
		{
			std::pair<int, int> pt = GetMouseOver();
			if (pt.first >= 0)
			{
				Click(pt.first, pt.second);
			}
		}
	}
}