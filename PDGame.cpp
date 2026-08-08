#include "PDGame.h"
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
#include "PDMainMenuModule.h"
#include "Items.h"
#include "LocationModule.h"
#include "PDMap.h"
#include "PDDMap.h"
#include "ResumeGameModule.h"
#include "PDClimbLadderOverlay.h"
#include "PDConvertPointsOverlay.h"
#include "PDElevationModOverlay.h"
#include "PDSelectLevelModule.h"
#include <cstring>
#include <ctime>
#include <algorithm>

bool CPDGame::Init()
{
	if (CModuleController::Init(new CPDMap(), new CPDDMap()) && LoadIcons() && CItems::Init())
	{
		DefaultCaptionColour1 = 0;
		DefaultCaptionColour2 = 0;
		DefaultCaptionColour3 = -1;
		DefaultCaptionColour4 = 0;

		CAnimationController::SetCaptionColours(0xff000000, 0xff000000, 0xff00c300, 0xff000000, 0xff000000, 0xff000000, 0xff0096ff, 0xff000000);
		CDXListBox::SetGreyColours(0, 0, 0xffc3c3c3, 0);
		CDXListBox::SetBlackColours(0, 0, 0xff000000, 0);
		CDXButton::SetButtonColours(0, 0, -1, 0);
		CResumeGameModule::SetTextColours(0, 0, 0xffff0000, 0);
		CResumeGameModule::SetHeaderColours(0, 0, -1, 0);
		CDXDialogueOption::SetGlobalColours(0, 0, 0xff000000, 0);
		CElevation::ElevationModifier = 0.0f;
		CElevation::ElevationCheckModifier = 0.2f;

		if (!CFile::Exists("PLAYERS/TEX___00.PLR"))
		{
			CFile file;
			if (file.Open("PLAYERS/TEX___00.PLR", CFile::Mode::Write))
			{
				uint8_t buffer[256];
				memset(buffer, 0, 256);
				buffer[2] = 'T';
				buffer[3] = 'E';
				buffer[4] = 'X';

				file.Write(buffer, 256);
				file.Close();
			}
		}

		// Create overlay controls
		pClimbLadderOverlay = new CPDClimbLadderOverlay();
		pConvertPointsOverlay = new CPDConvertPointsOverlay();
		pElevationModOverlay = new CPDElevationModOverlay();

		CModuleController::Push(new CPDMainMenuModule());

		return true;
	}

	return false;
}

CPDGame::CPDGame()
{
	_gameData = new uint8_t[PD_SAVE_SIZE];
	memset(_gameData, 0, PD_SAVE_SIZE);

	_lastDialoguePoint = -1;
	_frameTrigger = -1;

	ReadGameXMLInfo(IDR_XML_PD);

	SetGamePath("./");
}

CPDGame::~CPDGame()
{
}

void CPDGame::Render()
{
}

void CPDGame::MouseMove(Point pt)
{
}

void CPDGame::MouseDown(Point pt, int btn)
{
}

void CPDGame::KeyDown(uint32_t key, uint32_t lParam)
{
}

void CPDGame::KeyUp(uint32_t key, uint32_t lParam)
{
}

void CPDGame::LoadGame(const char* fileName)
{
	uint8_t data[PD_SAVE_SIZE];
	CFile file;
	if (file.Open(fileName))
	{
		int read = file.Read(data, PD_SAVE_SIZE);
		file.Close();

		if (read == PD_SAVE_SIZE)
		{
			memcpy(_gameData, data, PD_SAVE_SIZE);

			// Validate inventory, check current item
			int currentItem = _gameData[PD_SAVE_CURRENT_ITEM];
			if (currentItem != 0xff)
			{
				int itemCount = _gameData[PD_SAVE_ITEM_COUNT];
				bool found = false;
				for (int i = 0; i < itemCount; i++)
				{
					if (GetInt(_gameData, PD_SAVE_INVENTORY + i * 2, 2) == currentItem)
					{
						found = true;
						break;
					}
				}

				if (!found)
				{
					SetInt(_gameData, PD_SAVE_CURRENT_ITEM, -1, 2);
				}
			}

			std::string cash = CGameController::GetItemName(0) + " $" + std::to_string(GetInt(_gameData, PD_SAVE_CASH, 2));
			CItems::SetItemName(0, cash);

			// Should now load location or dialogue
			if (_gameData[PD_SAVE_PARAMETERS + 252])
			{
				CModuleController::Push(new CPDLocationModule(_gameData[PD_SAVE_MAP_ID], _gameData[PD_SAVE_STARTUP_POSITION]));
			}
			else
			{
				CModuleController::Push(new CVideoModule(VideoType::Scripted, _gameData[PD_SAVE_DMAP_ID], GetWord(PD_SAVE_SCRIPT_ID)));
			}
		}
	}
}

void CPDGame::SaveGame(const char* fileName)
{
	// TODO: Move to shared code
	CFile file;
	if (file.Open(fileName, CFile::Mode::Write))
	{
		std::time_t t = std::time(nullptr);
		std::tm* time = std::localtime(&t);
		
		int year = time->tm_year + 1900;
		_gameData[PD_SAVE_HEADER_YEAR] = (uint8_t)(year & 0xff);
		_gameData[PD_SAVE_HEADER_YEAR + 1] = (uint8_t)((year >> 8) & 0xff);
		_gameData[PD_SAVE_HEADER_MONTH] = (uint8_t)(time->tm_mon + 1);
		_gameData[PD_SAVE_HEADER_DAY] = (uint8_t)time->tm_mday;
		_gameData[PD_SAVE_HEADER_HOUR] = (uint8_t)time->tm_hour;
		_gameData[PD_SAVE_HEADER_MINUTE] = (uint8_t)time->tm_min;
		_gameData[PD_SAVE_HEADER_SECOND] = (uint8_t)time->tm_sec;

		_gameData[PD_SAVE_HEADER_MAP_ID] = _gameData[PD_SAVE_MAP_ID];
		_gameData[PD_SAVE_HEADER_DMAP_ID] = _gameData[PD_SAVE_DMAP_ID];

		_gameData[PD_SAVE_HEADER_GAME_LEVEL] = _gameData[PD_SAVE_PARAMETERS_GAME_LEVEL];
		_gameData[PD_SAVE_HEADER_SCORE] = _gameData[PD_SAVE_SCORE];
		_gameData[PD_SAVE_HEADER_SCORE + 1] = _gameData[PD_SAVE_SCORE + 1];


		file.Write(_gameData, PD_SAVE_SIZE);
		file.Close();
	}
}

void CPDGame::NewGame()
{
	CPDSelectLevelModule* pSelectLevelModule = new CPDSelectLevelModule(_gameData);
	CModuleController::Push(pSelectLevelModule);
}

uint8_t CPDGame::GetParameter(int index)
{
	return (index >= 0 && index < 1024) ? _gameData[PD_SAVE_PARAMETERS + index] : 0;
}

void CPDGame::SetParameter(int index, uint8_t value)
{
	if (index >= 0 && index < 1024)
	{
		_gameData[PD_SAVE_PARAMETERS + index] = value;
	}
}

uint8_t CPDGame::GetData(int offset)
{
	return (offset >= 0 && offset < PD_SAVE_SIZE) ? _gameData[offset] : 0;
}

void CPDGame::SetData(int offset, uint8_t value)
{
	if (offset >= 0 && offset < PD_SAVE_SIZE)
	{
		_gameData[offset] = value;
	}
}

void CPDGame::SetData(int offset, const char* text)
{
	if (offset >= 0)
	{
		while (offset < PD_SAVE_SIZE && *text)
		{
			_gameData[offset++] = *(text++);
		}
	}
}

uint8_t CPDGame::GetAskAboutState(int index)
{
	return (index >= 0 && index < 200) ? _gameData[PD_SAVE_ASK_ABOUT_STATES + index] : 0;
}

void CPDGame::SetAskAboutState(int index, uint8_t state)
{
	if (index >= 0 && index <= 125)
	{
		// Add or remove from list
		int count = GetAskAboutCount();
		if (state == 0 || state == 2)
		{
			int i = 0;
			for (i = 0; i < count && i < 50; i++)
			{
				if (GetInt(_gameData, PD_SAVE_ASK_ABOUTS + i * 2, 2) == index)
				{
					for (; i < count && i < 49; i++)
					{
						SetInt(_gameData, PD_SAVE_ASK_ABOUTS + i * 2, GetInt(_gameData, PD_SAVE_ASK_ABOUTS + (i + 1) * 2, 2), 2);
					}
					SetInt(_gameData, PD_SAVE_ASK_ABOUTS + count * 2, -1, 2);

					_gameData[PD_SAVE_ASK_ABOUT_COUNT]--;

					break;
				}
			}
		}
		else if (state == 1)
		{
			// Check if state already set
			for (int i = 0; i < count; i++)
			{
				if (GetInt(_gameData, PD_SAVE_ASK_ABOUTS + i * 2, 2) == index)
				{
					return;
				}
			}

			SetInt(_gameData, PD_SAVE_ASK_ABOUTS + count * 2, index, 2);
			_gameData[PD_SAVE_ASK_ABOUT_COUNT]++;
		}

		_gameData[PD_SAVE_ASK_ABOUT_STATES + index] = state;
	}
}

int CPDGame::GetAskAboutCount()
{
	return _gameData[PD_SAVE_ASK_ABOUT_COUNT];
}

int CPDGame::GetAskAboutId(int index)
{
	return (index >= 0 && index < 50) ? GetInt(_gameData, PD_SAVE_ASK_ABOUTS + index * 2, 2) : -1;
}

int CPDGame::GetScore()
{
	return GetWord(PD_SAVE_SCORE, true);
}

void CPDGame::AddScore(int value)
{
	SetWord(PD_SAVE_SCORE, GetWord(PD_SAVE_SCORE) + value);
}

int CPDGame::GetItemCount()
{
	int count = _gameData[PD_SAVE_ITEM_COUNT];
	return count;
}

int CPDGame::GetItemId(int index)
{
	int count = _gameData[PD_SAVE_ITEM_COUNT];
	int id = -1;
	if (index < count)
	{
		id = GetInt(_gameData, PD_SAVE_INVENTORY + index * 2, 2);
	}

	return id;
}

int CPDGame::GetItemState(int item)
{
	if (item >= 0 && item < PD_MAX_ITEM_COUNT)
	{
		return _gameData[PD_SAVE_ITEM_STATES + item];
	}

	return 0;
}

void CPDGame::SetItemState(int item, int state)
{
	if (item >= 0 && item < PD_MAX_ITEM_COUNT)
	{
		_gameData[PD_SAVE_ITEM_STATES + item] = state;

		int count = _gameData[PD_SAVE_ITEM_COUNT];

		// Add or remove from list
		if (state == 0 || state == 2)
		{
			for (int i = 0; i < count && i < PD_MAX_ITEM_COUNT; i++)
			{
				if (GetInt(_gameData, PD_SAVE_INVENTORY + i * 2, 2) == item)
				{
					for (int j = i + 1; j < count && j < PD_MAX_ITEM_COUNT; j++)
					{
						SetInt(_gameData, PD_SAVE_INVENTORY + (j - 1) * 2, GetInt(_gameData, PD_SAVE_INVENTORY + j * 2, 2), 2);
					}

					_gameData[PD_SAVE_ITEM_COUNT]--;

					break;
				}
			}

			if (GetInt(_gameData, PD_SAVE_CURRENT_ITEM, 2) == item)
			{
				SetInt(_gameData, PD_SAVE_CURRENT_ITEM, -1, 2);
			}
		}
		else if (state == 1)
		{
			bool alreadyInInventory = false;
			for (int i = 0; i < count; i++)
			{
				if (GetInt(_gameData, PD_SAVE_INVENTORY + i * 2, 2) == item)
				{
					alreadyInInventory = true;
					break;
				}
			}

			if (!alreadyInInventory)
			{
				// Push all other items back, insert at index 0
				for (int i = count; i > 0; i--)
				{
					SetInt(_gameData, PD_SAVE_INVENTORY + i * 2, GetInt(_gameData, PD_SAVE_INVENTORY + (i - 1) * 2, 2), 2);
				}

				SetInt(_gameData, PD_SAVE_INVENTORY, item, 2);
				_gameData[PD_SAVE_ITEM_COUNT]++;
				SetInt(_gameData, PD_SAVE_CURRENT_ITEM, item, 2);
			}
		}
	}
}

int CPDGame::GetItemState(int base, int item)
{
	// Return the item state based on item id, base is from count, actual list is from base + 10
	return 0;
}

void CPDGame::SetItemState(int base, int item, int state)
{
	int count = GetInt(_gameData, base, 2);

	// Add or remove from list
	if (state == 0 || state == 2)
	{
		for (int i = 0; i < count; i++)
		{
			if (GetInt(_gameData, base + 10 + i * 2, 2) == item)
			{
				for (int j = i + 1; j < count; j++)
				{
					SetInt(_gameData, base + 10 + (j - 1) * 2, GetInt(_gameData, base + 10 + j * 2, 2), 2);
				}

				SetInt(_gameData, base, count - 1, 2);

				if (GetInt(_gameData, base + 4, 2) == item)
				{
					SetInt(_gameData, base + 4, -1, 2);
				}

				break;
			}
		}
	}
	else if (state == 1)
	{
		bool itemExists = false;
		for (int i = 0; i < count; i++)
		{
			if (GetInt(_gameData, base + 10 + i * 2, 2) == item)
			{
				itemExists = true;
				break;
			}
		}

		if (!itemExists)
		{
			// Push all other items back, insert at index 0
			for (int i = count; i > 0; i--)
			{
				SetInt(_gameData, base + 10 + i * 2, GetInt(_gameData, base + 10 + (i - 1) * 2, 2), 2);
			}

			SetInt(_gameData, base + 10, item, 2);
			SetInt(_gameData, base, count + 1, 2);
			SetInt(_gameData, base + 4, item, 2);
		}
	}
}

int CPDGame::GetCurrentItemId()
{
	int itemId = GetInt(_gameData, PD_SAVE_CURRENT_ITEM, 2);
	return (itemId == 0xffff) ? -1 : itemId;
}

void CPDGame::SetCurrentItemId(int item)
{
	_gameData[PD_SAVE_CURRENT_ITEM] = item;
}

int CPDGame::SelectNextItem()
{
	int count = _gameData[PD_SAVE_ITEM_COUNT];
	int currentItem = GetInt(_gameData, PD_SAVE_CURRENT_ITEM, 2);
	int newIndex = IndexOfItemId(currentItem) + 1;
	if (newIndex >= count)
	{
		newIndex = -1;
	}

	SetInt(_gameData, PD_SAVE_CURRENT_ITEM, newIndex < 0 ? -1 : GetInt(_gameData, PD_SAVE_INVENTORY + newIndex * 2, 2), 2);

	return GetInt(_gameData, PD_SAVE_CURRENT_ITEM, 2);
}

int CPDGame::SelectPreviousItem()
{
	int count = _gameData[PD_SAVE_ITEM_COUNT];
	int currentItem = GetInt(_gameData, PD_SAVE_CURRENT_ITEM, 2);
	int newIndex = IndexOfItemId(currentItem) - 1;
	if (newIndex < -1)
	{
		newIndex = count - 1;
	}

	SetInt(_gameData, PD_SAVE_CURRENT_ITEM, newIndex < 0 ? -1 : GetInt(_gameData, PD_SAVE_INVENTORY + newIndex * 2, 2), 2);

	return GetInt(_gameData, PD_SAVE_CURRENT_ITEM, 2);
}

uint8_t CPDGame::GetHintState(int index)
{
	return 0;
}

int PDHintStatePairs[] = { 71, 1, 152, 863, 255, 357, 257, 358, 794, 359, 23, 864, 138, 865, 255, 866, 625, 870, 329, 872, 365, 356, 353, 761, 353, 99, 353, 145, 353, 146, 146, 145, 146, 99, 761, 25, 761, 352, 679, 678, 153, 207, -1 };

void CPDGame::SetHintState(int index, uint8_t state, int score)
{
	if (index >= 0 && index < 861)
	{
		int byte = index / 4;
		int shift = (index & 3) * 2;
		int oldState = (_gameData[PD_SAVE_HINT_STATES + byte] >> shift) & 3;
		_gameData[PD_SAVE_HINT_STATES + byte] |= (state & 3) << shift;
		if (oldState == 0 && score > 0)
		{
			AddScore(score);
			SetHintCategoryStateFromHint(index);
		}

		for (int i = 0; PDHintStatePairs[i * 2] != -1; i++)
		{
			if (PDHintStatePairs[i * 2] == index)
			{
				int pair = PDHintStatePairs[i * 2 + 1];
				byte = pair / 4;
				shift = (pair & 3) * 2;
				oldState = (_gameData[PD_SAVE_HINT_STATES + byte] >> shift) & 3;
				_gameData[PD_SAVE_HINT_STATES + byte] |= (state & 3) << shift;
				if (oldState == 0)
				{
					AddScore(score);
					SetHintCategoryStateFromHint(pair);
				}
			}
		}
	}
}

int PDHintCategoryPairs[] = { 78, 12, -1, 100, 15, -1, 152, 28, -1, 767, 34, -1, 23, 43, -1, 138, 46, -1, 378, 50, -1, 465, 61, -1, 557, 77, -1, 270, 78, -1, 100, 15, -1, 153, 29, -1, 853, 93, -1, -1 };

void CPDGame::SetHintCategoryStateFromHint(int hintIndex)
{
	int ix = 0;
	while (true)
	{
		int test = PDHintCategoryPairs[ix];
		if (test == -1) break;

		if (test == hintIndex)
		{
			while (true)
			{
				ix++;
				int category = PDHintCategoryPairs[ix];
				if (category == -1) break;

				if (_gameData[PD_SAVE_HINT_CATEGORY_STATES + category] == 0)
				{
					_gameData[PD_SAVE_HINT_CATEGORY_STATES + category] = 1;
				}
			}
		}
		else
		{
			while (PDHintCategoryPairs[ix] != -1)
			{
				ix++;
			}
		}

		ix++;
	}
}

uint8_t CPDGame::GetHintCategoryState(int index)
{
	return (index >= 0 && index < 96) ? _gameData[PD_SAVE_HINT_CATEGORY_STATES + index] : 0;
}

void CPDGame::SetHintCategoryState(int index, uint8_t state)
{
	if (index >= 0 && index < 96)
	{
		_gameData[PD_SAVE_HINT_CATEGORY_STATES + index] = state;
	}
}

void CPDGame::SetTimer(int timer, int duration)
{
	if (timer >= 0 && timer < 32)
	{
		//if (_gameData[UAKM_SAVE_TIMERS + timer] < 0)
		//{
		//	_gameData[UAKM_SAVE_TIMERS + timer] = 0;
		//}

		_gameData[PD_SAVE_TIMERS + timer] = 1;

		SetWord(PD_SAVE_TIMERS_INITIAL + timer * 2, duration);
		SetWord(PD_SAVE_TIMERS_CURRENT + timer * 2, duration);
		Timers[timer] = static_cast<int>(duration * TIMER_SCALE);
	}
}

int CPDGame::GetTimerState(int timer)
{
	int state = (timer >= 0 && timer < 32) ? _gameData[PD_SAVE_TIMERS + timer] : 0;

	//Trace(L"State of timer ");
	//Trace(timer);
	//Trace(L" is ");
	//Trace(state);
	//Trace(L", time left ");
	//Trace(Timers[timer]);
	//TraceLine(L" ms");

	return state;
}

void CPDGame::ResetTimers()
{
	memset(Timers, 0, sizeof(Timers));
	memset(_gameData + PD_SAVE_TIMERS_INITIAL, 0, 32 * 5);
}

void CPDGame::Tick(int ticks)
{
	// Check timers
	for (int i = 0; i < 32; i++)
	{
		if (_gameData[PD_SAVE_TIMERS + i] > 0)
		{
			Timers[i] = std::max(0, Timers[i] - ticks);
			if (Timers[i] == 0)
			{
				_gameData[PD_SAVE_TIMERS + i] = 0;
				SetWord(PD_SAVE_TIMERS_CURRENT + i * 2, GetWord(PD_SAVE_TIMERS_INITIAL + i * 2));
			}
		}
	}
}

void CPDGame::SetItemExamined(int itemId, int conditionalScore)
{
	if (itemId >= 0 && itemId < PD_MAX_ITEM_COUNT)
	{
		int byte = itemId / 8;
		int shift = itemId & 7;

		int oldState = (_gameData[PD_SAVE_ITEMS_EXAMINED_FLAGS + byte] & (1 << shift));
		_gameData[PD_SAVE_ITEMS_EXAMINED_FLAGS + byte] |= (1 << shift);

		if (oldState == 0)
		{
			if (conditionalScore > 0)
			{
				AddScore(conditionalScore);
			}

			// TODO: Check if e.g. extra cash should be added
			if (itemId == 274)
			{
				// Disc player with CD
				//ds:word_2A8774= 600
				// Offset 1B8 in save data
				//ds:byte_2A87A5= 1
				// Offset 1E9 in save data
				// Timer enabled and time?
			}
			else if (itemId == 285)
			{
				// Nilo's wallet
				AddCash(100);
			}
			else if (itemId == 288)
			{
				// Orphanage letter
				AddCash(500);
			}
			else if (itemId == 43)
			{
				// Prize letter
				AddCash(100);
			}
			else if (itemId == 225)
			{
				// Money belt
				AddCash(300);
			}
		}
	}
}

int CPDGame::GetWord(int offset, bool signExtend)
{
	int result = (offset >= 0 && offset < (PD_SAVE_SIZE - 1)) ? (_gameData[offset + 1] << 8) | _gameData[offset] : 0;
	if (signExtend && result & 0x8000)
	{
		result |= ~0xffff;
	}

	return result;
}

void CPDGame::SetWord(int offset, int value)
{
	if (offset >= 0 && offset < (PD_SAVE_SIZE - 1))
	{
		_gameData[offset] = value & 0xff;
		_gameData[offset + 1] = (value >> 8) & 0xff;

		if (offset == PD_SAVE_CASH)
		{
			std::string cash = CGameController::GetItemName(0) + " $" + std::to_string(GetInt(_gameData, PD_SAVE_CASH, 2));
			CItems::SetItemName(0, cash);
		}
	}
}

int CPDGame::IndexOfItemId(int item)
{
	int count = _gameData[PD_SAVE_ITEM_COUNT];
	for (int i = 0; i < count; i++)
	{
		if (GetInt(_gameData, PD_SAVE_INVENTORY + i * 2, 2) == item)
		{
			return i;
		}
	}

	return -1;
}

bool CPDGame::LoadIcons()
{
	bool result = false;
	BinaryData bd = LoadEntry("ICONS.AP", 0);
	if (bd.Data != nullptr && bd.Length > 0)
	{
		result = CGameBase::LoadIcons(bd);
		delete[] bd.Data;
	}

	return result;
}

int CPDGame::GetBuyableItemCount()
{
	return GetInt(_gameData, PD_SAVE_BUYABLES_COUNT, 2);
}

int CPDGame::GetBuyableItemId(int index)
{
	if (index >= 0 && index < 25)
	{
		return GetInt(_gameData, PD_SAVE_BUYABLES + index * 2, 2);
	}

	return -1;
}

void CPDGame::SetBuyableItemState(int index, int state)
{
	if (index >= 0 && index < 25)
	{
		// Add or remove from list
		int count = GetBuyableItemCount();
		if (state == 0 || state == 2)
		{
			int i = 0;
			for (i = 0; i < count && i < 25; i++)
			{
				if (_gameData[PD_SAVE_BUYABLES + i * 2] == index)
				{
					for (; i < count && i < 25; i++)
					{
						_gameData[PD_SAVE_BUYABLES + i * 2] = _gameData[PD_SAVE_BUYABLES + (i + 1) * 2];
					}
					SetWord(PD_SAVE_BUYABLES + count * 2, -1);

					_gameData[PD_SAVE_BUYABLES_COUNT]--;

					break;
				}
			}
		}
		else if (state == 1)
		{
			// Check if state already set
			for (int i = 0; i < count; i++)
			{
				if (GetInt(_gameData, PD_SAVE_BUYABLES + i * 2, 2) == index)
				{
					return;
				}
			}

			SetWord(PD_SAVE_BUYABLES + count * 2, index);
			_gameData[PD_SAVE_BUYABLES_COUNT]++;
		}
		else
		{
			int debug = 0;
		}

		_gameData[PD_SAVE_BUYABLES_ASK_ABOUT_STATES + index] = state;
	}
}

void CPDGame::AddCash(int cashToAdd)
{
	int currentCash = GetWord(PD_SAVE_CASH);
	SetWord(PD_SAVE_CASH, currentCash + cashToAdd);
}

void CPDGame::Copy(uint8_t* source, int destinationOffset, int length)
{
	if (source != nullptr && destinationOffset >= 0 && (destinationOffset + length) < PD_SAVE_SIZE)
	{
		memcpy(_gameData + destinationOffset, source, length);
	}
}
