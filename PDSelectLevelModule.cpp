#include "PDSelectLevelModule.h"
#include "ModuleController.h"
#include "PDGame.h"
#include "GameController.h"
#include "MainMenuModule.h"
#include <cstring>

#define VK_ESCAPE 0x1B

CPDSelectLevelModule::CPDSelectLevelModule(uint8_t* pGameData) : CModuleBase(ModuleType::NewGame)
{
	_pCBEntertainment = NULL;
	_pCBGamePlayer = NULL;

	_pBtnOK = NULL;
	_pBtnCancel = NULL;

	_gameData = pGameData;

	_bEntertainment = true;
	_bGamePlayer = false;
}

CPDSelectLevelModule::~CPDSelectLevelModule()
{
	Dispose();
}

void CPDSelectLevelModule::Dispose()
{
	if (_pCBEntertainment != NULL)
	{
		delete _pCBEntertainment;
		_pCBEntertainment = NULL;
	}

	if (_pCBGamePlayer != NULL)
	{
		delete _pCBGamePlayer;
		_pCBGamePlayer = NULL;
	}

	if (_pBtnOK != NULL)
	{
		delete _pBtnOK;
		_pBtnOK = NULL;
	}

	if (_pBtnCancel != NULL)
	{
		delete _pBtnCancel;
		_pBtnCancel = NULL;
	}
}

void CPDSelectLevelModule::Initialize()
{
	Rect r1;
	r1.Top = 0;
	r1.Left = 0;
	r1.Bottom = (float)dx.GetHeight();
	r1.Right = (float)dx.GetWidth();

	_line1.SetColours(0xffffffff, 0, 0);
	_line1.SetTextPD("Play level selection", r1);

	Rect r2;
	r2.Top = 0;
	r2.Left = 10;
	r2.Bottom = (float)dx.GetHeight() - 100;
	r2.Right = (float)dx.GetWidth() - 10;
	
	_line2.SetColours(0xff26ff00, 0xff0096ff, 0xffffffff);
	_line2.SetTextPD2("^.Do you wish to play the ^-Entertainment Level^. or the ^-Game Players Level^.?\n\nWe recommend that everyone except experienced game players select the ^-Entertainment Level^. the first time through.  There are hints available on this level as well as an option to bypass the more difficult puzzles (refer to the Hint System.)\n\nThe ^-Game Players Level^. is very challenging and should be selected only by experienced game players, or by players who have already gone through the ^-Entertainment Level^..  There are no hints available, but instead of ^?1500^. possible points, there are ^?4000^..  In addition, there are bonus locations and puzzles.\n\nNote:  Both levels have three narrative paths through the story leading to a total of seven combined endings.", r2);

	Rect r3;
	r3.Top = 0;
	r3.Left = 0;
	r3.Bottom = (float)dx.GetHeight() - 100;
	r3.Right = (float)dx.GetWidth();
	
	_line3.SetColours(0xffffffff, 0, 0);
	_line3.SetTextPD("Play level", r3);

	float chkY = (_line2.Height() + dx.GetHeight()) / 2.0f;

	_pCBEntertainment = new CDXCheckBox("Entertainment", &_bEntertainment, 0.0f);
	_pCBEntertainment->SetColours(0, 0, -1, 0);
	_pCBEntertainment->SetPosition(0, chkY);
	_pCBGamePlayer = new CDXCheckBox("Game player", &_bGamePlayer, 0.0f);
	_pCBGamePlayer->SetColours(0, 0, -1, 0);
	_pCBGamePlayer->SetPosition(dx.GetWidth() - _pCBGamePlayer->GetWidth() - 10, chkY);

	_pBtnOK = new CDXButton("OK", 80, 20);
	_pBtnOK->SetPosition(10, dx.GetHeight() - _pBtnOK->GetHeight() - 10);
	_pBtnCancel = new CDXButton("Cancel", 80, 20);
	_pBtnCancel->SetPosition(dx.GetWidth() - _pBtnCancel->GetWidth() - 10, dx.GetHeight() - _pBtnCancel->GetHeight() - 10);

	_cursorPosX = dx.GetWidth() / 2.0f;
	_cursorPosY = dx.GetHeight() / 2.0f;
}

void CPDSelectLevelModule::Resize(int width, int height)
{
}

void CPDSelectLevelModule::Render()
{
	_line1.Render((dx.GetWidth() - _line1.Width()) / 2.0f, 50);
	_line2.Render(0, 100);
	float chkY = (_line2.Height() + dx.GetHeight()) / 2.0f;
	_line3.Render((dx.GetWidth() - _line3.Width()) / 2.0f, chkY);

	_pCBEntertainment->Render();
	_pCBGamePlayer->Render();

	_pBtnOK->Render();
	_pBtnCancel->Render();

	// Render cursor
	CModuleController::Cursors[0].SetPosition(_cursorPosX, _cursorPosY);
	CModuleController::Cursors[0].Render();
}

void CPDSelectLevelModule::KeyDown(int key, int lParam)
{
	if (key == VK_ESCAPE)
	{
		return Back();
	}
}

void CPDSelectLevelModule::Cursor(float x, float y, bool relative)
{
	CModuleBase::Cursor(x, y, relative);

	_pBtnOK->SetMouseOver(_pBtnOK->HitTest(x, y) != NULL);
	_pBtnCancel->SetMouseOver(_pBtnCancel->HitTest(x, y) != NULL);
	_pCBEntertainment->SetMouseOver(_pCBEntertainment->HitTest(x, y) != NULL);
	_pCBGamePlayer->SetMouseOver(_pCBGamePlayer->HitTest(x, y) != NULL);
}

void CPDSelectLevelModule::BeginAction()
{
	// Check checkboxes and buttons
	if (_pCBEntertainment->HitTest(_cursorPosX, _cursorPosY))
	{
		_pCBEntertainment->SetCheck(true);
		_pCBGamePlayer->SetCheck(false);
	}
	else if (_pCBGamePlayer->HitTest(_cursorPosX, _cursorPosY))
	{
		_pCBGamePlayer->SetCheck(true);
		_pCBEntertainment->SetCheck(false);
	}
	else if (_pBtnOK->HitTest(_cursorPosX, _cursorPosY))
	{
		NewGame();
	}
	else if (_pBtnCancel->HitTest(_cursorPosX, _cursorPosY))
	{
		Back();
	}
}

void CPDSelectLevelModule::Back()
{
	CModuleController::Pop(this);
}

void CPDSelectLevelModule::NewGame()
{
	CMainMenuModule::SetPlayerNameAndEnableButtons();

	memset(_gameData, 0, PD_SAVE_SIZE);

	_gameData[PD_SAVE_HEADER_UNKNOWN1] = 6;
	CGameController::SetData(PD_SAVE_HEADER_PLAYER, "TEX");

	CGameController::SetData(PD_SAVE_HEADER_GAME_DAY, 1);

	CGameController::SetData(PD_SAVE_TRAVEL + 1, 1);                
	CGameController::SetData(PD_SAVE_TRAVEL + 70, 1);               
	CGameController::SetData(PD_SAVE_TRAVEL + 71, 1);               

	CGameController::SetItemState(PD_SAVE_ASK_ABOUT_BASE, 0, 1);    
	CGameController::SetItemState(PD_SAVE_ASK_ABOUT_BASE, 1, 1);    
	CGameController::SetItemState(PD_SAVE_ASK_ABOUT_BASE, 2, 1);    
	CGameController::SetItemState(PD_SAVE_ASK_ABOUT_BASE, 3, 1);    
	CGameController::SetItemState(PD_SAVE_ASK_ABOUT_BASE, 6, 1);    
	CGameController::SetItemState(PD_SAVE_ASK_ABOUT_BASE, 7, 1);    
	CGameController::SetItemState(PD_SAVE_ASK_ABOUT_BASE, 9, 1);    
	CGameController::SetItemState(PD_SAVE_ASK_ABOUT_BASE, 40, 1);   
	CGameController::SetItemState(PD_SAVE_ASK_ABOUT_BASE, 56, 1);   
	CGameController::SetItemState(PD_SAVE_ASK_ABOUT_BASE, 61, 1);   

	_gameData[PD_SAVE_PARAMETERS + 251] = 2;                        
	_gameData[PD_SAVE_PARAMETERS + 250] = 1;                        

	_gameData[PD_SAVE_PARAMETERS_GAME_LEVEL] = _bGamePlayer;

	CGameController::SetWord(PD_SAVE_CASH, 4000);
	CGameController::SetItemState(0, 1);                            
	CGameController::SetItemState(1, 1);                            
	CGameController::SetItemState(2, 1);                            
	CGameController::SetItemState(4, 1);                            

	CGameController::SetHintCategoryState(1, 1);
	CGameController::SetHintCategoryState(2, 1);
	CGameController::SetHintCategoryState(3, 1);
	CGameController::SetHintCategoryState(4, 1);

	// TODO: Clear some tables (hints?)

	CGameController::LoadFromDMap(42);
}
