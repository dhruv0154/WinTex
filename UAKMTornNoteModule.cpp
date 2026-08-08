#include "UAKMTornNoteModule.h"
#include "Globals.h"
#include "GameController.h"
#include "Utilities.h"
#include "UAKMGame.h"
#include "InventoryModule.h"
#include "ConstantBuffers.h"
#include "Shaders.h"
#include <chrono>
#include <cmath>
#include <algorithm>

int TornNoteDistances[] = { -22, 0, -57, 0, -44, 0, 123, -10, -117, 4, 117, -46, -46, 12, -47, -8, -48, -8, 39, -17, 102, -13, 0, -14, -67, 2, -37, 6, 93, -47, -94, 26, 105, -28, -36, -20, -71, -6, -29, 12, 136, -42, -52, 2, -93, 5 };

CUAKMTornNoteModule* CUAKMTornNoteModule::pUAKMTNM = nullptr;

#define PUZZLE_WIDTH    432.0f
#define PUZZLE_HEIGHT   300.0f

CUAKMTornNoteModule::CUAKMTornNoteModule(int item) : CModuleBase(ModuleType::TornNote)
{
	pUAKMTNM = this;

	_cursorMinX = 0;
	_cursorMaxX = dx.GetWidth() - 1;
	_cursorMinY = 0;
	_screenHeight = static_cast<float>(dx.GetHeight());
	_cursorMaxY = static_cast<int>(_screenHeight - 1.0f);

	_item = item;
	_newItem = -1;

	_numberOfImages = 0;
	_pImageData = nullptr;

	_positionOffset = 0;
	_vertexBuffer = nullptr;

	float width = PUZZLE_WIDTH, height = PUZZLE_HEIGHT;
	float screenWidth = (float)dx.GetWidth();
	float sx = screenWidth / width;
	float sy = _screenHeight / height;
	_scale = std::min(sx, sy);

	_selectedScrap = nullptr;
	_completed = false;

	_timeToExit = 0;
}

CUAKMTornNoteModule::~CUAKMTornNoteModule()
{
	Dispose();
}

void CUAKMTornNoteModule::Dispose()
{
	if (_pImageData != nullptr)
	{
		delete[] _pImageData;
		_pImageData = nullptr;
	}

	if (_vertexBuffer != nullptr)
	{
		_vertexBuffer->Release();
		_vertexBuffer = nullptr;
	}

	CPuzzlePiece::Dispose();
}

void CUAKMTornNoteModule::Render()
{
	if (_vertexBuffer != nullptr)
	{
		dx.DisableZBuffer();

		CConstantBuffers::Setup2D(dx);

		uint32_t stride = sizeof(TEXTURED_VERTEX);
		uint32_t offset = 0;
		dx.SetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
		dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		CShaders::SelectOrthoShader();

		CPuzzlePiece::Render();

		if (_completed)
		{
			_caption.Render(0.0f, dx.GetHeight() - 10.0f - _caption.Height());
		}
		else
		{
			_pBtnResume->Render();
		}

		CModuleController::Cursors[0].SetPosition((float)_cursorPosX, (float)_cursorPosY);
		CModuleController::Cursors[0].Render();

		dx.EnableZBuffer();
	}

	if (_completed && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() >= _timeToExit)
	{
		CInventoryModule::ExamineItemOnResume = _newItem;
		CModuleController::Pop(this);
	}
}

void CUAKMTornNoteModule::Initialize()
{
	CPuzzlePiece::Reset();

	_cursorPosX = dx.GetWidth() / 2.0f;
	_cursorPosY = dx.GetHeight() / 2.0f;

	int palIx = -1;
	if (_item == 33)
	{
		palIx = 21;
		_numberOfImages = 28;
		_positionOffset = UAKM_SAVE_PUZZLE_DATA;
		CGameController::SetHintState(188, 1, 0);
	}
	else if (_item == 57)
	{
		palIx = 24;
		_numberOfImages = 31;
		_positionOffset = UAKM_SAVE_PUZZLE_DATA + 168;
		CGameController::SetHintState(361, 1, 0);
	}
	else if (_item == 139)
	{
		palIx = 26;
		_numberOfImages = 26;
		_positionOffset = UAKM_SAVE_PUZZLE_DATA + 354;
		CGameController::SetHintState(247, 1, 0);
	}

	_caption.SetColours(0, 0xff00c300, 0xff24ff00, 0);
	_caption.SetText("That completes it!  Now that it's assembled, I think I'll glue the pieces together so that they won't move.", CDXText::Alignment::Justify);

	if (palIx >= 0)
	{
		BinaryData bdPal = LoadEntry("GRAPHICS.AP", palIx);
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

		BinaryData bdImages = LoadEntry("GRAPHICS.AP", palIx + 1);
		if (bdImages.Data != nullptr)
		{
			_pImageData = bdImages.Data;
			int count = GetInt(_pImageData, 0, 2) - 1;
			if (_item == 139)
			{
				count = 26;
			}

			float left = (dx.GetWidth() - PUZZLE_WIDTH * _scale) / 2.0f;
			float top = (dx.GetHeight() - PUZZLE_HEIGHT * _scale) / 2.0f;

			for (int i = 0; i < count; i++)
			{
				CPuzzlePiece::Add(i, _pImageData + GetInt(_pImageData, 2 + i * 4, 4), _scale, _palette, _numberOfImages, left, top, _positionOffset);
			}

			CPuzzlePiece::Sort();
		}
	}

	CreateTexturedRectangle(0.5f, -0.5f, -0.5f, 0.5f, &_vertexBuffer, "NoteVertexBuffer");

	const char* pResume = "Resume";
	_pBtnResume = new CDXButton(pResume, TexFont.PixelWidth(pResume), 32.0f * pConfig->FontScale, OnResume);
	_pBtnResume->SetPosition(dx.GetWidth() - _pBtnResume->GetWidth(), dx.GetHeight() - 40 * pConfig->FontScale);
}

bool CUAKMTornNoteModule::CheckCompleted()
{
	if (_item == 33)
	{
		for (int i = 0; i < (_numberOfImages - 5); i++)
		{
			CPuzzlePiece* pScrap1 = CPuzzlePiece::Get(i);
			if (pScrap1->Orientation != 0)
			{
				return false;
			}

			CPuzzlePiece* pScrap2 = CPuzzlePiece::Get(i + 1);

			int dx_val = static_cast<int>(std::abs((pScrap1->X - pScrap2->X) / _scale - TornNoteDistances[i * 2]));
			if (dx_val > 5)
			{
				return false;
			}

			int dy = static_cast<int>(std::abs((pScrap1->Y - pScrap2->Y) / _scale - TornNoteDistances[i * 2 + 1]));
			if (dy > 5)
			{
				return false;
			}
		}
	}
	else if (_item == 57)
	{
		for (int i = 0; i < (_numberOfImages - 1); i++)
		{
			CPuzzlePiece* pScrap1 = CPuzzlePiece::Get(i);
			CPuzzlePiece* pScrap2 = CPuzzlePiece::Get(i + 1);
			int dx_val = static_cast<int>((pScrap2->X - pScrap1->X) / _scale);
			if (dx_val < 8 || dx_val > 12)
			{
				return false;
			}
		}
	}
	else if (_item == 139)
	{
		return false;
	}

	return true;
}

void CUAKMTornNoteModule::Resize(int width, int height)
{
}

void CUAKMTornNoteModule::Cursor(float x, float y, bool relative)
{
	CModuleBase::Cursor(x, y, relative);

	_pBtnResume->SetMouseOver(_pBtnResume->HitTest(x, y) != nullptr);

	if (!_completed && _selectedScrap != nullptr)
	{
		_selectedScrap->X -= _pt.x - _cursorPosX;
		_selectedScrap->Y -= _pt.y - _cursorPosY;
		_pt.x = static_cast<int>(_cursorPosX);
		_pt.y = static_cast<int>(_cursorPosY);
	}
}

void CUAKMTornNoteModule::BeginAction()
{
	if (_pBtnResume->HitTest(_cursorPosX, _cursorPosY))
	{
		_pBtnResume->Click();
	}
	else
	{
		if (!_completed)
		{
			CPuzzlePiece* pScrap = CPuzzlePiece::HitTest(_cursorPosX, _cursorPosY, _item == 33 ? 4 : 0, _scale);
			if (pScrap != nullptr)
			{
				_selectedScrap = pScrap;
				_pt.x = static_cast<int>(_cursorPosX);
				_pt.y = static_cast<int>(_cursorPosY);
			}
		}
	}
}

void CUAKMTornNoteModule::OnResume(void* data)
{
	pUAKMTNM->Back();
}

void CUAKMTornNoteModule::EndAction()
{
	if (_selectedScrap != nullptr && !_completed)
	{
		int x = static_cast<int>(_selectedScrap->X / _scale);
		int y = static_cast<int>(_selectedScrap->Y / _scale);
		CGameController::SetData(_selectedScrap->Offset, x & 0xff);
		CGameController::SetData(_selectedScrap->Offset + 1, (x >> 8) & 0xff);
		CGameController::SetData(_selectedScrap->Offset + 2, y & 0xff);
		CGameController::SetData(_selectedScrap->Offset + 3, (y >> 8) & 0xff);
		CGameController::SetData(_selectedScrap->Offset + 4, _selectedScrap->Z);
		CGameController::SetData(_selectedScrap->Offset + 5, _selectedScrap->Orientation);

		_selectedScrap = nullptr;

		if ((_completed = CheckCompleted()))
		{
			_timeToExit = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() + (uint64_t)(300 * TIMER_SCALE);

			if (_item == 33)
			{
				CGameController::SetHintState(177, 1, 1);
				CGameController::SetHintCategoryState(27, 2);
			}
			else if (_item == 57)
			{
				CGameController::AddScore(12);

				if (CGameController::GetParameter(250) == 5)
				{
					CGameController::SetData(UAKM_SAVE_TRAVEL + 14, 1);
				}

				CGameController::SetHintState(362, 1, 1);
			}

			_newItem = (_item == 33) ? 103 : (_item == 57) ? 109 : -1;
			if (_newItem >= 0)
			{
				int itemCount = CGameController::GetItemCount();
				for (int i = 0; i < itemCount; i++)
				{
					if (CGameController::GetItemId(i) == _item)
					{
						CGameController::SetData(UAKM_SAVE_INVENTORY + i, _newItem);
					}
				}

				CGameController::SetItemState(_item, 2);
				CGameController::SetItemState(_newItem, 1);
			}
		}
	}
}

void CUAKMTornNoteModule::Back()
{
	CModuleController::Pop(this);
}

void CUAKMTornNoteModule::Cycle()
{
	if (!_completed)
	{
		if (_selectedScrap != nullptr)
		{
			_selectedScrap->Orientation = (_selectedScrap->Orientation + 1) & 3;
		}
		else
		{
			CPuzzlePiece* pScrap = CPuzzlePiece::HitTest(_cursorPosX, _cursorPosY, _item == 33 ? 4 : 0, _scale);
			if (pScrap != nullptr)
			{
				pScrap->Orientation = (pScrap->Orientation + 1) & 3;
			}
		}
	}
}

void CUAKMTornNoteModule::Next()
{
	if (_selectedScrap != nullptr && !_completed)
	{
		_selectedScrap->Orientation = (_selectedScrap->Orientation + 1) & 3;
	}
}

void CUAKMTornNoteModule::Prev()
{
	if (_selectedScrap != nullptr && !_completed)
	{
		_selectedScrap->Orientation = (_selectedScrap->Orientation - 1) & 3;
	}
}