#include "PDCrosswordModule.h"
#include "PDGame.h"
#include "GameController.h"
#include "PDInventoryModule.h"
#include "ConstantBuffers.h"
#include "Shaders.h"
#include <SDL2/SDL.h>

#define VK_BACK 0x08

const char* CPDCrosswordModule::Solution = "DEATH C TRACTEROS SAP IGORARR ARMOR RUELOTUSSPEEDERSTRAPP Y FRETS   SIR DUO   STEER A SPLITMARTIANCASINOALA NATAL OATCOTS ASP ANNEKNOTS Y ASSES";
const char* CPDCrosswordModule::CheatSequence = "821";

#define PD_CROSSWORD_X_OFFSET                   12
#define PD_CROSSWORD_Y_OFFSET                   16
#define PD_CROSSWORD_CELL_CENTER_X_OFFSET       27
#define PD_CROSSWORD_CELL_CENTER_Y_OFFSET       35
#define PD_CROSSWORD_CELL_SIZE                  31
#define PD_CROSSWORD_CELLS_WIDE                 13
#define PD_CROSSWORD_CELLS_TALL                 11
#define PD_CROSSWORD_CELL_COUNT                 143
#define PD_CROSSWORD_COLOUR_BLACK               0x10
#define PD_CROSSWORD_COLOUR_WHITE               0x3b
#define PD_CROSSWORD_COLOUR_RED                 9

#define PD_CROSSWORD_CELL_CURSOR_Y_OFFSET       7
#define PD_CROSSWORD_SPECIAL_PALETTE_IMAGE      54
#define PD_CROSSWORD_SPECIAL_SOUND              56
#define PD_CROSSWORD_ICONS_FONT                 13

CPDCrosswordModule::CPDCrosswordModule() : CFullScreenModule(ModuleType::Crossword)
{
	_updateTexture = false;

	_currentCellX = 0;
	_currentCellY = 0;
	_advanceX = 0;
	_advanceY = 0;
}

CPDCrosswordModule::~CPDCrosswordModule()
{
	Dispose();
}

void CPDCrosswordModule::Dispose()
{
}

void CPDCrosswordModule::Render()
{
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

		CModuleController::Cursors[(int)CAnimatedCursor::CursorType::Arrow].SetPosition(_cursorPosX, _cursorPosY);
		CModuleController::Cursors[(int)CAnimatedCursor::CursorType::Arrow].Render();

		dx.EnableZBuffer();
	}

	if (CheckCompleted())
	{
		CGameController::TransformItem(PD_INVENTORY_CROSSWORD_PUZZLE, PD_INVENTORY_FINISHED_CROSSWORD_PUZZLE);
		CGameController::SetParameter(205, 1);
		CInventoryModule::SetSelectedItem(PD_INVENTORY_FINISHED_CROSSWORD_PUZZLE);

		CModuleController::Pop(this);
	}
}

void CPDCrosswordModule::Initialize()
{
	CFullScreenModule::Initialize();

	DoubleData dd = LoadDoubleEntry("SPECIAL.AP", PD_CROSSWORD_SPECIAL_PALETTE_IMAGE);
	if (dd.File1.Data != NULL)
	{
		_screen = dd.File2.Data;

		uint8_t* pPal = dd.File1.Data;
		ReadPalette(pPal);

		delete[] pPal;
	}

	// Load extra files
	BinaryData bd = LoadEntry("SPECIAL.AP", PD_CROSSWORD_SPECIAL_SOUND);
	if (bd.Data != NULL)
	{
		_data = bd.Data;
		int count = GetInt(_data, 0, 2) - 1;
		for (int i = 0; i < count; i++)
		{
			_files[i] = _data + GetInt(_data, 2 + i * 4, 4);
		}
	}

	// ICONS.AP.13 contains the crossword puzzle font
	bd = LoadEntry("ICONS.AP", PD_CROSSWORD_ICONS_FONT);
	_font.Init(bd);

	// If this is the first time entering the module, set all entered text to spaces
	if (CGameController::GetData(PD_SAVE_CROSSWORD_PUZZLE) == 0)
	{
		for (int i = 0; i < PD_CROSSWORD_CELL_COUNT; i++)
		{
			CGameController::SetData(PD_SAVE_CROSSWORD_PUZZLE + i, ' ');
		}
	}

	// Update puzzle with cells filled in
	for (int y = 0; y < PD_CROSSWORD_CELLS_TALL; y++)
	{
		for (int x = 0; x < PD_CROSSWORD_CELLS_WIDE; x++)
		{
			uint8_t value = CGameController::GetData(PD_SAVE_CROSSWORD_PUZZLE + y * PD_CROSSWORD_CELLS_WIDE + x);
			if (value != 0 && value != ' ')
			{
				_font.Render(_screen, 640, 480, PD_CROSSWORD_CELL_CENTER_X_OFFSET + x * PD_CROSSWORD_CELL_SIZE, PD_CROSSWORD_CELL_CENTER_Y_OFFSET + y * PD_CROSSWORD_CELL_SIZE, value, PD_CROSSWORD_COLOUR_BLACK - 1, true);
			}
		}
	}

	_updateTexture = true;

	CGameController::SetParameter(205, 0);
}

void CPDCrosswordModule::KeyDown(int key, int lParam)
{
	_currentCellX = (static_cast<int>((_cursorPosX - _left) / _scale) - PD_CROSSWORD_X_OFFSET) / PD_CROSSWORD_CELL_SIZE;
	_currentCellY = (static_cast<int>((_cursorPosY - _top) / _scale) - PD_CROSSWORD_Y_OFFSET) / PD_CROSSWORD_CELL_SIZE;

	int cellIndex = _currentCellY * PD_CROSSWORD_CELLS_WIDE + _currentCellX;
	if (_currentCellX >= 0 && _currentCellX < PD_CROSSWORD_CELLS_WIDE && _currentCellY >= 0 && _currentCellY < PD_CROSSWORD_CELLS_TALL && Solution[cellIndex] != ' ' && ((key >= 'A' && key <= 'Z') || key == ' ' || key == VK_BACK))
	{
		int direction = 1;

		if (key == VK_BACK)
		{
			key = ' ';
			direction = -1;
		}

		int cellCenterX = PD_CROSSWORD_CELL_CENTER_X_OFFSET + _currentCellX * PD_CROSSWORD_CELL_SIZE;
		int cellCenterY = PD_CROSSWORD_CELL_CENTER_Y_OFFSET + _currentCellY * PD_CROSSWORD_CELL_SIZE;

		uint8_t oldValue = CGameController::GetData(PD_SAVE_CROSSWORD_PUZZLE + cellIndex);
		if (oldValue != key)
		{
			if (oldValue >= 'A' && oldValue <= 'Z')
			{
				// Remove old char
				_font.Render(_screen, 640, 480, cellCenterX, cellCenterY, oldValue, PD_CROSSWORD_COLOUR_WHITE - 1, true);
			}

			if (key >= 'A' && key <= 'Z')
			{
				// Render new char
				_font.Render(_screen, 640, 480, cellCenterX, cellCenterY, (uint8_t)key, PD_CROSSWORD_COLOUR_BLACK - 1, true);
			}

			CGameController::SetData(PD_SAVE_CROSSWORD_PUZZLE + cellIndex, (uint8_t)key);

			_updateTexture = true;
		}

		if ((_advanceX != 0 || _advanceY != 0) && !IsBlocked(_currentCellX + _advanceX, _currentCellY + _advanceY))
		{
			_cursorPosX = ((cellCenterX + PD_CROSSWORD_CELL_SIZE * _advanceX) * _scale) + _left;
			_cursorPosY = ((cellCenterY + PD_CROSSWORD_CELL_SIZE * _advanceY + PD_CROSSWORD_CELL_CURSOR_Y_OFFSET) * _scale) + _top;

			_inputEnabled = false;
			
			// Cross-platform cursor mapping replacement
			if (::_hWnd != nullptr) 
			{
				SDL_WarpMouseInWindow((SDL_Window*)::_hWnd, static_cast<int>(_cursorPosX), static_cast<int>(_cursorPosY));
			}
		}

		_sound.Play(_files[0]);
	}
}

bool CPDCrosswordModule::IsBlocked(int x, int y)
{
	return (x < 0 || x >= PD_CROSSWORD_CELLS_WIDE || y < 0 || y >= PD_CROSSWORD_CELLS_TALL || Solution[y * PD_CROSSWORD_CELLS_WIDE + x] == ' ');
}

void CPDCrosswordModule::Cursor(float x, float y, bool relative)
{
	if (_inputEnabled)
	{
		CModuleBase::Cursor(x, y, relative);

		int movedToX = static_cast<int>((_cursorPosX - _left) / _scale);
		int movedToY = static_cast<int>((_cursorPosY - _top) / _scale);
		int movedToCellX = (movedToX - PD_CROSSWORD_X_OFFSET) / PD_CROSSWORD_CELL_SIZE;
		int movedToCellY = (movedToY - PD_CROSSWORD_Y_OFFSET) / PD_CROSSWORD_CELL_SIZE;

		if (movedToCellX >= 0 && movedToCellX < PD_CROSSWORD_CELLS_WIDE && movedToCellY >= 0 && movedToCellY < PD_CROSSWORD_CELLS_TALL)
		{
			if (IsBlocked(movedToCellX, movedToCellY))
			{
				_advanceX = _advanceY = 0;
			}
			else
			{
				bool cellLeftBlocked = IsBlocked(movedToCellX - 1, movedToCellY);
				bool cellRightBlocked = IsBlocked(movedToCellX + 1, movedToCellY);
				bool cellAboveBlocked = IsBlocked(movedToCellX, movedToCellY - 1);
				bool cellBelowBlocked = IsBlocked(movedToCellX, movedToCellY + 1);

				if (cellLeftBlocked && !cellRightBlocked)
				{
					// First in a row, advance horizontal
					_advanceX = 1;
					_advanceY = 0;
				}
				else if (cellAboveBlocked && !cellBelowBlocked)
				{
					// Assuming first in column, advance vertical
					_advanceX = 0;
					_advanceY = 1;
				}
			}
		}
	}

	_inputEnabled = true;
}

void CPDCrosswordModule::BeginAction()
{
	int x = static_cast<int>((_cursorPosX - _left) / _scale);
	int y = static_cast<int>((_cursorPosY - _top) / _scale);

	// Check if exit button is pressed
	if (x >= 581 && x <= 639 && y >= 451 && y <= 479)
	{
		CModuleController::Pop(this);
	}
}

void CPDCrosswordModule::Back()
{
	CModuleController::Pop(this);
}

bool CPDCrosswordModule::CheckCompleted()
{
	for (int i = 0; i < PD_CROSSWORD_CELL_COUNT; i++)
	{
		if (CGameController::GetData(PD_SAVE_CROSSWORD_PUZZLE + i) != Solution[i])
		{
			return false;
		}
	}

	return true;
}