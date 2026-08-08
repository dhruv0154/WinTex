#include "UAKMMainMenuModule.h"
#include "GameController.h"
#include "DXImageButton.h"
#include "Utilities.h"
#include "AmbientAudio.h"
#include "UAKMGame.h"
#include <algorithm>
#include <string>
#include <filesystem>
#include <chrono>

CUAKMMainMenuModule::CUAKMMainMenuModule()
{
}

CUAKMMainMenuModule::~CUAKMMainMenuModule()
{
}

void CUAKMMainMenuModule::Intro(void* data)
{
	pMIDI->Stop();
	CAmbientAudio::StopAll();
	CAmbientAudio::Clear();
	CGameController::LoadFromDMap(29);
}

void CUAKMMainMenuModule::Credits(void* data)
{
	pMIDI->Stop();
	CAmbientAudio::StopAll();
	CAmbientAudio::Clear();
	CGameController::LoadFromDMap(41);
}

void CUAKMMainMenuModule::SetupScreen()
{
	uint32_t s = 0;
	uint8_t* pImg = GetResource(IDB_JPG_UAKM_TITLE, "JPG", &s);
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

	float moonW = iw * 0.67f;
	float moonH = ih * 0.64f;

	float moonCenterX = w - imageLeft - moonW / 2.0f;
	float moonCenterY = h / 2.2f;

	float btnRight = moonCenterX + 32.0f * pConfig->FontScale;
	float btnLeft = btnRight - maxw - 96.0f * pConfig->FontScale;
	float btnMiddle = moonCenterY;
	float btnTop = btnMiddle - 64.0f * pConfig->FontScale;
	float btnBottom = btnMiddle + 64.0f * pConfig->FontScale;

	_pScreen->AddButton(pNG, btnLeft, btnTop, maxw, 32.0f * pConfig->FontScale, NewGame);
	CDXButton* pLoadBtn = new CDXButton(pLG, maxw, 32.0f * pConfig->FontScale, Load);
	_pScreen->AddChild(pLoadBtn, btnLeft, btnMiddle);
	
	_btnMainSave = new CDXButton(pSG, maxw, 32.0f * pConfig->FontScale, Save);
	_btnMainSave->SetEnabled(false);
	_pScreen->AddChild(_btnMainSave, btnLeft, btnBottom);

	_pScreen->AddButton(pCf, btnRight, btnTop, maxw, 32.0f * pConfig->FontScale, Config);
	_pScreen->AddButton(pIn, btnRight, btnMiddle, maxw, 32.0f * pConfig->FontScale, Intro);
	_pScreen->AddButton(pCr, btnRight, btnBottom, maxw, 32.0f * pConfig->FontScale, Credits);

	_btnMainResume = _pScreen->AddButton(pRe, moonCenterX - (maxw + 32.0f * pConfig->FontScale) / 2.0f, btnTop - 64.0f * pConfig->FontScale, maxw, 32.0f * pConfig->FontScale, Resume);
	_btnMainResume->SetVisible(false);

	_pScreen->AddButton(pQu, moonCenterX - (maxw + 32.0f * pConfig->FontScale) / 2.0f, btnBottom + 64.0f * pConfig->FontScale, maxw, 32.0f * pConfig->FontScale, Quit);
}

void CUAKMMainMenuModule::SetupSave()
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
	int fileIndex = lastDot != std::string::npos ? std::min(999, std::stoi(CurrentGameInfo.FileName.substr(lastDot + 1))) : 1;
	
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
	if (CGameController::GetData(UAKM_SAVE_DMAP_FLAG) == 0)
	{
		sit = CGameController::GetSituationDescriptionL(CGameController::GetData(UAKM_SAVE_MAP_ENTRY));
	}
	else
	{
		sit = CGameController::GetSituationDescriptionD(CGameController::GetData(UAKM_SAVE_DMAP_ENTRY));
	}
	
	info.Location = sit;
	info.DayInGame = "Day " + std::to_string(std::max(1, std::min(7, static_cast<int>(CGameController::GetData(UAKM_SAVE_GAME_DAY)))));
	
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
}

void CUAKMMainMenuModule::SetupLoad()
{
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
					CFile file;
					std::string fileName = "GAMES\\" + name;
					if (file.Open(fileName.c_str()))
					{
						uint8_t buffer[0xd1];
						if (file.Read(buffer, 0xd0) == 0xd0)
						{
							SaveGameInfo info;
							info.FileName = fileName;
							info.Player = std::string((const char*)(buffer + UAKM_SAVE_PLAYER), UAKM_SAVE_LOCATION - UAKM_SAVE_PLAYER);
							info.Location = std::string((const char*)(buffer + UAKM_SAVE_LOCATION), UAKM_SAVE_GAME_DAY - UAKM_SAVE_LOCATION);
							info.DayInGame = std::string("Day ") + std::to_string(buffer[UAKM_SAVE_GAME_DAY]);
							
							int year = buffer[UAKM_SAVE_YEAR] | (buffer[UAKM_SAVE_YEAR + 1] << 8);
							info.DateTime = IntToString(year, 4) + "-" + 
							                IntToString(buffer[UAKM_SAVE_MONTH], 2) + "-" + 
							                IntToString(buffer[UAKM_SAVE_DAY], 2) + " " + 
							                IntToString(buffer[UAKM_SAVE_HOUR], 2) + ":" + 
							                IntToString(buffer[UAKM_SAVE_MINUTE], 2) + ":" + 
							                IntToString(buffer[UAKM_SAVE_SECOND], 2);
							
							info.Comment = std::string((const char*)(buffer + UAKM_SAVE_COMMENT), UAKM_SAVE_PADDING1 - UAKM_SAVE_COMMENT);

							_savedGames.push_back(info);
						}

						file.Close();
					}
				}
			}
		}
	}

	std::sort(_savedGames.begin(), _savedGames.end());

	LoadSetup();

	_pScreen->ShowModal(_pLoad);
}