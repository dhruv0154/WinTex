#include "PDMainMenuModule.h"
#include "GameController.h"
#include "Utilities.h"
#include "ResumeGameModule.h"
#include "PDGame.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <string>

CPDMainMenuModule::CPDMainMenuModule() : CMainMenuModule()
{
	_saveCursor.SetColours(0, 0, 0xffae0000, 0);
}

CPDMainMenuModule::~CPDMainMenuModule()
{
}

void CPDMainMenuModule::Intro(void* data)
{
	pMIDI->Stop();
	CGameController::LoadFromDMap(30);
}

void CPDMainMenuModule::Credits(void* data)
{
	pMIDI->Stop();
	CGameController::LoadFromDMap(57);
}

void CPDMainMenuModule::SetupScreen()
{
	uint32_t s = 0;
	uint8_t* pImg = GetResource(IDB_JPG_PD_TITLE, "JPG", &s);
	CDXBitmap* pBmp = _pScreen->AddBitmap(pImg, s, Alignment::CenterX | Alignment::CenterY | Alignment::Scale | Alignment::Crop);

	const char* pNG = "New game";
	const char* pLG = "Load";
	const char* pSG = "Save";
	const char* pCf = "Config";
	const char* pIn = "Intro";
	const char* pCr = "Credits";
	const char* pRe = "Resume";
	const char* pQu = "Quit";

	float maxw = std::max({
		TexFont.PixelWidth(pNG), TexFont.PixelWidth(pLG), TexFont.PixelWidth(pSG), 
		TexFont.PixelWidth(pCf), TexFont.PixelWidth(pIn), TexFont.PixelWidth(pCr), 
		TexFont.PixelWidth(pRe), TexFont.PixelWidth(pQu)
	});

	float iw = pBmp->GetWidth();
	float ih = pBmp->GetHeight();
	float w = static_cast<float>(dx.GetWidth());
	float h = static_cast<float>(dx.GetHeight());

	float imageTop = (h - ih) / 2.0f;
	float imageLeft = (w - iw) / 2.0f;
	pBmp->SetPosition(imageLeft, imageTop);

	// Buttons should be inside moon (need to find different reference point for pd)
	// Moon covers about 67% of the width and 70% of the height
	float moonW = iw * 0.67f;
	float moonH = ih * 0.64f;

	float moonCenterX = w - imageLeft - moonW / 2.0f;
	float moonCenterY = imageTop + moonH / 2.0f;

	float btnRight = moonCenterX + 32.0f * pConfig->FontScale;
	float btnLeft = btnRight - maxw - 96.0f * pConfig->FontScale;
	float btnMiddle = moonCenterY;
	float btnTop = btnMiddle - 64.0f * pConfig->FontScale;
	float btnBottom = btnMiddle + 64.0f * pConfig->FontScale;

	CDXButton* pNewGameBtn = _pScreen->AddButton(pNG, btnLeft, btnTop, maxw, 32.0f * pConfig->FontScale, NewGame);
	CDXButton* pLoadBtn = new CDXButton(pLG, maxw, 32.0f * pConfig->FontScale, Load);
	_pScreen->AddChild(pLoadBtn, btnLeft, btnMiddle);
	
	_btnMainSave = new CDXButton(pSG, maxw, 32.0f * pConfig->FontScale, Save);
	_btnMainSave->SetEnabled(false);
	_pScreen->AddChild(_btnMainSave, btnLeft, btnBottom);

	CDXButton* pConfigBtn = _pScreen->AddButton(pCf, btnRight, btnTop, maxw, 32.0f * pConfig->FontScale, Config);
	CDXButton* pIntroBtn = _pScreen->AddButton(pIn, btnRight, btnMiddle, maxw, 32.0f * pConfig->FontScale, Intro);
	//pIntroBtn->SetEnabled(FALSE);
	CDXButton* pCreditsBtn = _pScreen->AddButton(pCr, btnRight, btnBottom, maxw, 32.0f * pConfig->FontScale, Credits);

	// Add resume (only visible when game is in progress) and quit (always visible, but ask if game in progress)
	_btnMainResume = _pScreen->AddButton(pRe, moonCenterX - (maxw + 32.0f * pConfig->FontScale) / 2.0f, btnTop - 64.0f * pConfig->FontScale, maxw, 32.0f * pConfig->FontScale, Resume);
	_btnMainResume->SetVisible(false);

	_pScreen->AddButton(pQu, moonCenterX - (maxw + 32.0f * pConfig->FontScale) / 2.0f, btnBottom + 64.0f * pConfig->FontScale, maxw, 32.0f * pConfig->FontScale, Quit);

	_pScreen->SetColours(0, 0, -1, 0);
}

void CPDMainMenuModule::Render()
{
	CDXFont::SelectBlackFont();
	_pScreen->Render();

	uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	if (CurrentSaveMode != SaveMode::Load && (now / 500) % 2)
	{
		if (CurrentSaveMode == SaveMode::Extension)
		{
			// Show cursor at filename extension
			SaveGameInfo info = _saveControl->GetInfo();
			char buffer[32];
			memset(buffer, 0, 32);
			for (int i = 0; i < 32 && i < info.FileName.length(); i++)
			{
				buffer[i] = info.FileName[i];
			}
			float x = _saveControl->GetX() + 21 * pConfig->FontScale + std::ceil(TexFont.PixelWidth(buffer));
			float y = _saveControl->GetY() + 8 * pConfig->FontScale;
			_saveCursor.Render(x, y);
		}
		else if (CurrentSaveMode == SaveMode::Comment)
		{
			// Show cursor at comment
			//SaveGameInfo info = _saveControl->GetInfo();
			float y = _saveControl->GetY() + 68 * pConfig->FontScale;
			_saveCursor.Render(static_cast<float>(_caretPos), y);
		}
	}

	// Render cursor
	CModuleController::Cursors[0].SetPosition(_cursorPosX, _cursorPosY);
	CModuleController::Cursors[0].Render();
}

void CPDMainMenuModule::SetupConfigFrame()
{
	CMainMenuModule::SetupConfigFrame();

	// Update colours on config controls
	_pConfig->SetColours(0, 0, -1, 0);
}

void CPDMainMenuModule::SetupLoadFrame()
{
	CMainMenuModule::SetupLoadFrame();
}

void CPDMainMenuModule::SetupSaveFrame()
{
	CMainMenuModule::SetupSaveFrame();
}

void CPDMainMenuModule::SetupSave()
{
	CurrentSaveMode = SaveMode::Extension;
	SaveTypedChars = 3;

	memset(_commentBuffer, 0, 256);

	SaveGameInfo info;
	info.FileName = "GAMES\\";
	auto nameLength = CurrentGameInfo.Player.length();
	while (nameLength > 0 && CurrentGameInfo.Player.at(nameLength - 1) == ' ')
	{
		nameLength--;
	}

	for (int i = 0; i < 6; i++)
	{
		info.FileName += (char)((i < nameLength) ? CurrentGameInfo.Player.at(i) : '_');
	}
	info.FileName += "00.";
	
	auto lastDot = CurrentGameInfo.FileName.find_last_of('.');
	int fileIndex = lastDot > 0 ? std::min(999, std::stoi(CurrentGameInfo.FileName.c_str() + lastDot + 1)) : 1;
	
	if (fileIndex < 100)
	{
		info.FileName += "0";
	}
	if (fileIndex < 10)
	{
		info.FileName += "0";
	}
	info.FileName += std::to_string(fileIndex);

	info.Player = CurrentGameInfo.Player;
	std::string sit;
	if (CGameController::GetParameter(252) == 0)
	{
		// Dialogue
		sit = CGameController::GetSituationDescriptionD(CGameController::GetData(PD_SAVE_DMAP_ID) + 1);
	}
	else
	{
		// Location
		sit = CGameController::GetSituationDescriptionL(CGameController::GetData(PD_SAVE_MAP_ID) + 1);
	}
	
	info.Location = sit;
	info.DayInGame = "Day " + std::to_string(std::max(1, std::min(10, static_cast<int>(CGameController::GetData(PD_SAVE_PARAMETERS_DAY_IN_GAME)))));
	
	std::time_t t = std::time(nullptr);
	std::tm* tm = std::localtime(&t);
	int year = tm->tm_year + 1900;
	int month = tm->tm_mon + 1;
	int day = tm->tm_mday;
	int hour = tm->tm_hour;
	int minute = tm->tm_min;
	int second = tm->tm_sec;
	
	info.DateTime = IntToString(year, 4) + "-" + IntToString(month, 2) + "-" + IntToString(day, 2) + " " + IntToString(hour, 2) + ":" + IntToString(minute, 2) + ":" + IntToString(second, 2);
	
	_saveControl->SetInfo(info);
	_saveControl->SetPDColours();
}

void CPDMainMenuModule::SetupLoad()
{
	// Get list of current save games
	_savedGames.clear();
	std::string gamesPath = "GAMES";

	if (std::filesystem::exists(gamesPath) && std::filesystem::is_directory(gamesPath))
	{
		for (const auto& entry : std::filesystem::directory_iterator(gamesPath))
		{
			if (entry.is_regular_file())
			{
				std::string name = entry.path().filename().string();
				
				if (name != "SAVEGAME.000")
				{
					// Real file, load header and extract info
					CFile file;
					std::string fileName = "GAMES\\" + name;
					
					if (file.Open(fileName.c_str()))
					{
						uint8_t buffer[0xd8];
						if (file.Read(buffer, 0xd8) == 0xd8)
						{
							SaveGameInfo info;
							info.FileName = fileName;
							info.Player = std::string((const char*)(buffer + PD_SAVE_HEADER_PLAYER), 10);
							
							std::string sit;
							if (buffer[PD_SAVE_PARAMETERS + 252] == 0)
							{
								// Dialogue
								sit = CGameController::GetSituationDescriptionD(buffer[PD_SAVE_DMAP_ID] + 1);
							}
							else
							{
								// Location
								sit = CGameController::GetSituationDescriptionL(buffer[PD_SAVE_MAP_ID] + 1);
							}
							info.Location = sit;

							info.DayInGame = std::string("Day ") + std::to_string(buffer[PD_SAVE_HEADER_GAME_DAY]);
							info.DateTime = IntToString(buffer[PD_SAVE_HEADER_YEAR] | (buffer[PD_SAVE_HEADER_YEAR + 1] << 8), 4) + "-" + IntToString(buffer[PD_SAVE_HEADER_MONTH], 2) + "-" + IntToString(buffer[PD_SAVE_HEADER_DAY], 2) + " " + IntToString(buffer[PD_SAVE_HEADER_HOUR], 2) + ":" + IntToString(buffer[PD_SAVE_HEADER_MINUTE], 2) + ":" + IntToString(buffer[PD_SAVE_HEADER_SECOND], 2);
							info.Comment = std::string((const char*)(buffer + PD_SAVE_HEADER_COMMENT), PD_SAVE_HEADER_PADDING - PD_SAVE_HEADER_COMMENT);

							_savedGames.push_back(info);
						}

						file.Close();
					}
				}
			}
		}
	}

	// Sort list (by last written or by save game date?)
	std::sort(_savedGames.begin(), _savedGames.end());

	LoadSetup();

	int ix = 0;
	while (ix < _saveGameControls.size())
	{
		CSaveGameControl* sgc = _saveGameControls.at(ix);
		sgc->SetPDColours();
		ix++;
	}

	_pScreen->ShowModal(_pLoad);
}
