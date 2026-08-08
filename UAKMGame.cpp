#include "UAKMGame.h"
#include "Utilities.h"
#include "DXText.h"
#include "Globals.h"
#include "DXControls.h"
#include "DXImageButton.h"
#include "DXCheckBox.h"
#include "Configuration.h"
#include "AnimationController.h"
#include "ModuleBase.h"
#include "VideoModule.h"
#include "GameController.h"
#include "UAKMMainMenuModule.h"
#include "Items.h"
#include "LocationModule.h"
#include "InputMapping.h"
#include "UAKMMap.h"
#include "UAKMDMap.h"
#include <cstring>
#include <ctime>
#include <algorithm>

bool CUAKMGame::Init()
{
	if (CModuleController::Init(new CUAKMMap(), new CUAKMDMap()) && LoadIcons() && CItems::Init())
	{
		if (!CFile::Exists("PLAYERS/TEX___00.PLR"))
		{
			CFile file;
			if (file.Open("PLAYERS/TEX___00.PLR", CFile::Mode::Write))
			{
				uint8_t buffer[256];
				memset(buffer, 0, 256);
				buffer[1] = 1;
				buffer[2] = 'T';
				buffer[3] = 'E';
				buffer[4] = 'X';
				memset(buffer + 5, ' ', 21);
				buffer[26] = 0;		
				buffer[27] = 1;		

				file.Write(buffer, 256);
				file.Close();
			}
		}

		CModuleController::Push(new CUAKMMainMenuModule());

		return true;
	}

	return false;
}

CUAKMGame::CUAKMGame()
{
	_gameData = new uint8_t[UAKM_SAVE_SIZE];
	memset(_gameData, 0, UAKM_SAVE_SIZE);

	_lastDialoguePoint = -1;
	_frameTrigger = -1;

	ReadGameXMLInfo(IDR_XML_UAKM);

	SetGamePath("./");
}

CUAKMGame::~CUAKMGame()
{
	CItems::Dispose();
	CAnimationController::Clear();

	for (int i = 0; i < 3; i++)
	{
		DialogueOptions[i].Clear();
	}
}

void CUAKMGame::Render()
{
	CModuleController::Render();
}

void CUAKMGame::MouseDown(Point pt, int btn)
{
	CModuleController::MouseDown(pt, btn);
}

void CUAKMGame::MouseMove(Point pt)
{
	CModuleController::MouseMove(pt);
}

void CUAKMGame::KeyDown(uint32_t key, uint32_t lParam)
{
	CModuleController::KeyDown(key, lParam);
}

void CUAKMGame::KeyUp(uint32_t key, uint32_t lParam)
{
	CModuleController::KeyUp(key, lParam);
}

void CUAKMGame::NewGame()
{
	CMainMenuModule::SetPlayerNameAndEnableButtons();

	memset(_gameData, 0, UAKM_SAVE_SIZE);
	memset(_gameData + 2, ' ', UAKM_SAVE_PADDING1 - 2);

	_gameData[UAKM_SAVE_UNKNOWN1 + 1] = 1;
	SetData(UAKM_SAVE_PLAYER, "TEX");

	_gameData[UAKM_SAVE_GAME_DAY] = 1;

	SetData(UAKM_SAVE_CODED_MESSAGE, "YE UANE CIAFWBHED RIPB AEEIWALHEAL  YWLU CUAXLWLR AL  LUE XPWLE WA LUE  GIODEA GALE UILEO AL LUE PXPAO LWHE.LUE EAXXYIBD LIDARWX XWOWCIA.        ");

	SetAskAboutState(0, 1);							
	SetAskAboutState(1, 1);							
	SetAskAboutState(2, 1);							
	SetAskAboutState(3, 1);							
	SetAskAboutState(4, 1);							
	SetAskAboutState(5, 1);							
	SetAskAboutState(17, 1);						

	SetData(UAKM_SAVE_TRAVEL + 5, 1);				

	_gameData[UAKM_SAVE_CURRENT_ASK] = -1;			
	_gameData[UAKM_SAVE_CURRENT_ITEM] = -1;			
	_gameData[UAKM_SAVE_CHAPTER] = 1;
	_gameData[UAKM_SAVE_PARAMETERS + 99] = -1;
	_gameData[UAKM_SAVE_PARAMETERS + 251] = 2;
	_gameData[UAKM_SAVE_PARAMETERS + 250] = 1;		
	_gameData[UAKM_SAVE_HINT_CATEGORY_STATES + 1] = 1;

	BinaryData bd = LoadEntry("GRAPHICS.AP", 23);
	if (bd.Data != nullptr && bd.Length == 0x1fe)
	{
		memcpy(_gameData + UAKM_SAVE_PUZZLE_DATA, bd.Data, bd.Length);
		delete[] bd.Data;
	}

	LoadFromDMap(0);
}

void CUAKMGame::LoadGame(const char* fileName)
{
	uint8_t data[UAKM_SAVE_SIZE];
	CFile file;
	if (file.Open(fileName))
	{
		int read = file.Read(data, UAKM_SAVE_SIZE);
		file.Close();

		if (read == UAKM_SAVE_SIZE)
		{
			memcpy(_gameData, data, UAKM_SAVE_SIZE);

			int currentItem = _gameData[UAKM_SAVE_CURRENT_ITEM];
			if (currentItem != 0xff)
			{
				int itemCount = _gameData[UAKM_SAVE_ITEM_COUNT];
				bool found = false;
				for (int i = 0; i < itemCount; i++)
				{
					if (_gameData[UAKM_SAVE_INVENTORY + i] == currentItem)
					{
						found = true;
						break;
					}
				}

				if (!found)
				{
					_gameData[UAKM_SAVE_CURRENT_ITEM] = 0xff;
				}
			}

			if (_gameData[UAKM_SAVE_DMAP_FLAG])
			{
				CModuleController::Push(new CVideoModule(VideoType::Scripted, _gameData[UAKM_SAVE_DMAP_ENTRY], GetWord(UAKM_SAVE_SCRIPT_ID)));
			}
			else
			{
				CModuleController::Push(new CLocationModule(_gameData[UAKM_SAVE_MAP_ENTRY], CGameController::GetParameter(249)));
			}
		}
	}
}

void CUAKMGame::SaveGame(const char* fileName)
{
	CFile file;
	if (file.Open(fileName, CFile::Mode::Write))
	{
		std::string sit;
		if (_gameData[UAKM_SAVE_DMAP_FLAG] == 0)
		{
			sit = CGameController::GetSituationDescriptionL(_gameData[UAKM_SAVE_MAP_ENTRY]);
		}
		else
		{
			sit = CGameController::GetSituationDescriptionD(_gameData[UAKM_SAVE_DMAP_ENTRY]);
		}

		memset(_gameData + UAKM_SAVE_LOCATION, ' ', 30);
		for (int i = 0; i < sit.size() && i < 0x1e; i++)
		{
			_gameData[UAKM_SAVE_LOCATION + i] = sit[i] & 0xFF;
		}

		std::time_t t = std::time(nullptr);
		std::tm* time = std::localtime(&t);
		int year = time->tm_year + 1900;
		_gameData[UAKM_SAVE_GAME_DAY] = std::min((uint8_t)7, std::max((uint8_t)1, _gameData[UAKM_SAVE_PARAMETERS + 250]));
		_gameData[UAKM_SAVE_YEAR] = (uint8_t)(year & 0xff);
		_gameData[UAKM_SAVE_YEAR + 1] = (uint8_t)((year >> 8) & 0xff);
		_gameData[UAKM_SAVE_MONTH] = (uint8_t)(time->tm_mon + 1);
		_gameData[UAKM_SAVE_DAY] = (uint8_t)time->tm_mday;
		_gameData[UAKM_SAVE_HOUR] = (uint8_t)time->tm_hour;
		_gameData[UAKM_SAVE_MINUTE] = (uint8_t)time->tm_min;
		_gameData[UAKM_SAVE_SECOND] = (uint8_t)time->tm_sec;

		file.Write(_gameData, UAKM_SAVE_SIZE);
		file.Close();
	}
}

uint8_t CUAKMGame::GetParameter(int index)
{
	return (index >= 0 && index < 256) ? _gameData[UAKM_SAVE_PARAMETERS + index] : 0;
}

void CUAKMGame::SetParameter(int index, uint8_t value)
{
	if (index >= 0 && index < 256)
	{
		_gameData[UAKM_SAVE_PARAMETERS + index] = value;
	}
}

int CUAKMGame::GetWord(int offset, bool signExtend)
{
	int result = (offset >= 0 && offset < (UAKM_SAVE_SIZE - 1)) ? (_gameData[offset + 1] << 8) | _gameData[offset] : 0;
	if (signExtend && result & 0x8000)
	{
		result |= ~0xffff;
	}

	return result;
}

void CUAKMGame::SetWord(int offset, int value)
{
	if (offset >= 0 && offset < (UAKM_SAVE_SIZE - 1))
	{
		_gameData[offset] = value & 0xff;
		_gameData[offset + 1] = (value >> 8) & 0xff;
	}
}

uint8_t CUAKMGame::GetAskAboutState(int index)
{
	return (index >= 0 && index < 45) ? _gameData[UAKM_SAVE_ASK_ABOUT_STATES + index] : 0;
}

void CUAKMGame::SetAskAboutState(int index, uint8_t state)
{
	if (index >= 0 && index <= 45)
	{
		int count = GetAskAboutCount();
		if (state == 0 || state == 2)
		{
			int i = 0;
			for (i = 0; i < count && i < 50; i++)
			{
				if (_gameData[UAKM_SAVE_ASK_ABOUTS + i] == index)
				{
					for (; i < count && i < 49; i++)
					{
						_gameData[UAKM_SAVE_ASK_ABOUTS + i] = _gameData[UAKM_SAVE_ASK_ABOUTS + i + 1];
					}
					_gameData[UAKM_SAVE_ASK_ABOUTS + count] = 0xff;

					_gameData[UAKM_SAVE_ASK_ABOUT_COUNT]--;

					break;
				}
			}
		}
		else if (state == 1)
		{
			for (int i = 0; i < count; i++)
			{
				if (_gameData[UAKM_SAVE_ASK_ABOUTS + i] == index)
				{
					return;
				}
			}

			_gameData[UAKM_SAVE_ASK_ABOUTS + count] = index;
			_gameData[UAKM_SAVE_ASK_ABOUT_COUNT]++;
		}

		_gameData[UAKM_SAVE_ASK_ABOUT_STATES + index] = state;
	}
}

int CUAKMGame::GetAskAboutCount()
{
	return _gameData[UAKM_SAVE_ASK_ABOUT_COUNT];
}

int CUAKMGame::GetAskAboutId(int index)
{
	return _gameData[UAKM_SAVE_ASK_ABOUTS + index];
}

int CUAKMGame::GetScore()
{
	return GetWord(UAKM_SAVE_SCORE, true);
}

void CUAKMGame::AddScore(int value)
{
	SetWord(UAKM_SAVE_SCORE, GetWord(UAKM_SAVE_SCORE) + value);
}

int CUAKMGame::GetItemCount()
{
	return _gameData[UAKM_SAVE_ITEM_COUNT];
}

int CUAKMGame::GetItemId(int index)
{
	return (index >= 0 && index < UAKM_MAX_ITEM_COUNT) ? _gameData[UAKM_SAVE_INVENTORY + index] : 0;
}

int CUAKMGame::GetItemState(int item)
{
	return _gameData[UAKM_SAVE_ITEM_STATES + item];
}

void CUAKMGame::SetItemState(int item, int state)
{
	if (item >= 0 && item < UAKM_MAX_ITEM_COUNT)
	{
		_gameData[UAKM_SAVE_ITEM_STATES + item] = state;

		int count = _gameData[UAKM_SAVE_ITEM_COUNT];

		if (state == 0 || state == 2)
		{
			for (int i = 0; i < count && i < UAKM_MAX_ITEM_COUNT; i++)
			{
				if (_gameData[UAKM_SAVE_INVENTORY + i] == item)
				{
					for (int j = i + 1; j < count && j < UAKM_MAX_ITEM_COUNT; j++)
					{
						_gameData[UAKM_SAVE_INVENTORY + j - 1] = _gameData[UAKM_SAVE_INVENTORY + j];
					}

					_gameData[UAKM_SAVE_ITEM_COUNT]--;

					break;
				}
			}

			if (_gameData[UAKM_SAVE_CURRENT_ITEM] == item)
			{
				_gameData[UAKM_SAVE_CURRENT_ITEM] = 0xff;
			}
		}
		else if (state == 1)
		{
			bool alreadyInInventory = false;
			for (int i = 0; i < count; i++)
			{
				if (_gameData[UAKM_SAVE_INVENTORY + i] == item)
				{
					alreadyInInventory = true;
					break;
				}
			}

			if (!alreadyInInventory)
			{
				_gameData[UAKM_SAVE_INVENTORY + count] = item;
				_gameData[UAKM_SAVE_ITEM_COUNT]++;
				_gameData[UAKM_SAVE_CURRENT_ITEM] = item;
			}
		}
	}
}

int CUAKMGame::GetCurrentItemId()
{
	return (_gameData[UAKM_SAVE_CURRENT_ITEM] == 0xff) ? -1 : _gameData[UAKM_SAVE_CURRENT_ITEM];
}

void CUAKMGame::SetCurrentItemId(int item)
{
	_gameData[UAKM_SAVE_CURRENT_ITEM] = item;
}

int CUAKMGame::SelectNextItem()
{
	int count = _gameData[UAKM_SAVE_ITEM_COUNT];
	int currentItem = _gameData[UAKM_SAVE_CURRENT_ITEM];
	int newIndex = IndexOfItemId(currentItem) + 1;
	if (newIndex >= count)
	{
		newIndex = -1;
	}

	_gameData[UAKM_SAVE_CURRENT_ITEM] = newIndex < 0 ? 0xff : _gameData[UAKM_SAVE_INVENTORY + newIndex];

	return _gameData[UAKM_SAVE_CURRENT_ITEM];
}

int CUAKMGame::SelectPreviousItem()
{
	int count = _gameData[UAKM_SAVE_ITEM_COUNT];
	int currentItem = _gameData[UAKM_SAVE_CURRENT_ITEM];
	int newIndex = IndexOfItemId(currentItem) - 1;
	if (newIndex < -1)
	{
		newIndex = count - 1;
	}

	_gameData[UAKM_SAVE_CURRENT_ITEM] = newIndex < 0 ? 0xff : _gameData[UAKM_SAVE_INVENTORY + newIndex];

	return _gameData[UAKM_SAVE_CURRENT_ITEM];
}

int CUAKMGame::IndexOfItemId(int item)
{
	int count = _gameData[UAKM_SAVE_ITEM_COUNT];
	for (int i = 0; i < count; i++)
	{
		if (_gameData[UAKM_SAVE_INVENTORY + i] == item)
		{
			return i;
		}
	}

	return -1;
}

uint8_t CUAKMGame::GetHintState(int index)
{
	int val = 0;
	if (index >= 0 && index < 1352)
	{
		int byte = index / 4;
		int shift = (index & 3) * 2;
		val = (_gameData[UAKM_SAVE_HINT_STATES + byte] >> shift) & 3;
	}

	return val;
}

int HintStatePairs[] = { 3, 21, 56, 61, 263, 289, 12, 11, 264, 290, 434, 440, 434, 446, 234, 186, 343, 342 };
void CUAKMGame::SetHintState(int index, uint8_t state, int score)
{
	if (index >= 0 && index < 1352)
	{
		int byte = index / 4;
		int shift = (index & 3) * 2;
		int oldState = (_gameData[UAKM_SAVE_HINT_STATES + byte] >> shift) & 3;
		_gameData[UAKM_SAVE_HINT_STATES + byte] |= (state & 3) << shift;
		if (oldState == 0 && score > 0)
		{
			AddScore(score);
		}

		for (int i = 0; i < 9; i++)
		{
			if (HintStatePairs[i * 2] == index)
			{
				int pair = HintStatePairs[i * 2 + 1];
				byte = pair / 4;
				shift = (pair & 3) * 2;
				oldState = (_gameData[UAKM_SAVE_HINT_STATES + byte] >> shift) & 3;
				_gameData[UAKM_SAVE_HINT_STATES + byte] |= (state & 3) << shift;
				if (oldState == 0)
				{
					AddScore(1);
				}
			}
		}
	}
}

uint8_t CUAKMGame::GetHintCategoryState(int index)
{
	return (index >= 0 && index < 86) ? _gameData[UAKM_SAVE_HINT_CATEGORY_STATES + index] : 0;
}

void CUAKMGame::SetHintCategoryState(int index, uint8_t state)
{
	if (index >= 0 && index < 86)
	{
		_gameData[UAKM_SAVE_HINT_CATEGORY_STATES + index] = state;
	}
}

void CUAKMGame::SetTimer(int timer, int duration)
{
	if (timer >= 0 && timer < 32)
	{
		_gameData[UAKM_SAVE_TIMERS + timer] = 1;

		SetWord(UAKM_SAVE_TIMERS_INITIAL + timer * 2, duration);
		SetWord(UAKM_SAVE_TIMERS_CURRENT + timer * 2, duration);
		Timers[timer] = static_cast<int>(duration * TIMER_SCALE);
	}
}

int CUAKMGame::GetTimerState(int timer)
{
	int state = (timer >= 0 && timer < 32) ? _gameData[UAKM_SAVE_TIMERS + timer] : 0;
	return state;
}

void CUAKMGame::ResetTimers()
{
	memset(Timers, 0, sizeof(Timers));
	memset(_gameData + UAKM_SAVE_TIMERS_INITIAL, 0, 32 * 5);
}

void CUAKMGame::Tick(int ticks)
{
	for (int i = 0; i < 32; i++)
	{
		if (_gameData[UAKM_SAVE_TIMERS + i] > 0)
		{
			Timers[i] = std::max(0, Timers[i] - ticks);
			if (Timers[i] == 0)
			{
				_gameData[UAKM_SAVE_TIMERS + i] = 0;
				SetWord(UAKM_SAVE_TIMERS_CURRENT + i * 2, GetWord(UAKM_SAVE_TIMERS_INITIAL + i * 2));
			}
		}
	}
}

uint8_t CUAKMGame::GetData(int offset)
{
	return (offset >= 0 && offset < UAKM_SAVE_SIZE) ? _gameData[offset] : 0;
}

void CUAKMGame::SetData(int offset, uint8_t value)
{
	if (offset >= 0 && offset < UAKM_SAVE_SIZE)
	{
		_gameData[offset] = value;
	}
}

void CUAKMGame::SetData(int offset, const char* text)
{
	if (offset >= 0)
	{
		while (offset < UAKM_SAVE_SIZE && *text)
		{
			_gameData[offset++] = *(text++);
		}
	}
}

void CUAKMGame::SetItemExamined(int itemId, int conditionalScore)
{
	if (itemId >= 0 && itemId < UAKM_MAX_ITEM_COUNT)
	{
		int byte = itemId / 8;
		int shift = itemId & 7;

		int oldState = (_gameData[UAKM_SAVE_ITEMS_EXAMINED_FLAGS + byte] & (1 << shift));
		_gameData[UAKM_SAVE_ITEMS_EXAMINED_FLAGS + byte] |= (1 << shift);

		if (oldState == 0 && conditionalScore > 0)
		{
			AddScore(conditionalScore);
		}
	}
}

bool CUAKMGame::LoadIcons()
{
	bool result = false;
	BinaryData bd = CLZ::Decompress("ICONS.LZ");
	if (bd.Data != nullptr && bd.Length > 0)
	{
		result = CGameBase::LoadIcons(bd);
		delete[] bd.Data;
	}

	return result;
}