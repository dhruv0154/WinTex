#include "ResumeGameModule.h"
#include "VideoModule.h"
#include "GameController.h"
#include "Utilities.h"
#include "MainMenuModule.h"
#include <algorithm>

int CResumeGameModule::TextColour1 = 0;
int CResumeGameModule::TextColour2 = 0xffc30000;
int CResumeGameModule::TextColour3 = 0xffff1800;
int CResumeGameModule::TextColour4 = 0;

int CResumeGameModule::HeaderColour1 = 0;
int CResumeGameModule::HeaderColour2 = -1;
int CResumeGameModule::HeaderColour3 = -1;
int CResumeGameModule::HeaderColour4 = 0;

CResumeGameModule::CResumeGameModule() : CModuleBase(ModuleType::ResumeGame)
{
	_pFrame = NULL;
	_pLine1 = NULL;
	_pLine2 = NULL;
	_pBtnYes = NULL;
	_pBtnNo = NULL;
}

CResumeGameModule::~CResumeGameModule()
{
}

void CResumeGameModule::Render()
{
	_pFrame->Render();

	CModuleController::Cursors[(int)CAnimatedCursor::CursorType::Arrow].SetPosition(_cursorPosX, _cursorPosY);
	CModuleController::Cursors[(int)CAnimatedCursor::CursorType::Arrow].Render();
}

void CResumeGameModule::Initialize()
{
	_cursorPosX = static_cast<int>(_cursorMaxX) / 2.0f;
	_cursorPosY = static_cast<int>(_cursorMaxY) / 2.0f;

	int w = dx.GetWidth();
	int h = dx.GetHeight();

	const char* pY = "Yes";
	const char* pN = "No";
	const char* pL1 = "Do you wish to continue the";
	const char* pL2 = "current game in progress?";
	const char* pH = "GAME IN PROGRESS";
	
	float maxbtnw = std::max(TexFont.PixelWidth(pY), TexFont.PixelWidth(pN));
	float maxlabelw = std::max(TexFont.PixelWidth(pL1), TexFont.PixelWidth(pL2));
	float hw = std::max(TexFont.PixelWidth(pH), maxlabelw);
	float lineHeight = TexFont.Height() * pConfig->FontScale;
	float fw = hw + 16.0f * pConfig->FontScale;
	float fh = 8.5f * lineHeight;

	_pFrame = new CDXFrame(pH, fw, fh);

	Rect labelRect;
	labelRect.Top = 0;
	labelRect.Left = 0;
	labelRect.Bottom = 0;
	labelRect.Right = static_cast<int>(maxlabelw);

	_pLine1 = new CDXLabel(pL1, labelRect, CDXText::Alignment::JustifyAlways);
	_pLine2 = new CDXLabel(pL2, labelRect, CDXText::Alignment::JustifyAlways);
	_pLine1->SetColours(TextColour1, TextColour2, TextColour3, TextColour4);
	_pLine2->SetColours(TextColour1, TextColour2, TextColour3, TextColour4);

	float fx = (w - fw) / 2.0f;
	float fy = (h - fh) / 2.0f;

	_pFrame->SetColours(HeaderColour1, HeaderColour2, HeaderColour3, HeaderColour4);

	_pFrame->AddChild(_pLine1, fx + 8.0f * pConfig->FontScale, fy + lineHeight * 2);
	_pFrame->AddChild(_pLine2, fx + 8.0f * pConfig->FontScale, fy + lineHeight * 3);
	_pBtnYes = _pFrame->AddButton(pY, fx + 8.0f * pConfig->FontScale, fy + lineHeight * 4.5f, maxbtnw, 20.0f, NULL);
	_pBtnNo = _pFrame->AddButton(pN, fx + fw - maxbtnw - 32.0f * pConfig->FontScale - 8.0f * pConfig->FontScale, fy + lineHeight * 4.5f, maxbtnw, 20.0f, NULL);
	_pFrame->SetPosition(fx, fy);
}

void CResumeGameModule::Cursor(float x, float y, bool relative)
{
	CModuleBase::Cursor(x, y, relative);

	_pBtnYes->SetMouseOver(_pBtnYes->HitTest(x, y) != NULL);
	_pBtnNo->SetMouseOver(_pBtnNo->HitTest(x, y) != NULL);
}

void CResumeGameModule::BeginAction()
{
	if (_pBtnYes != NULL && _pBtnYes->HitTest(_cursorPosX, _cursorPosY) != NULL)
	{
		Yes();
	}
	else if (_pBtnNo != NULL && _pBtnNo->HitTest(_cursorPosX, _cursorPosY) != NULL)
	{
		No();
	}
}

void CResumeGameModule::Resize(int width, int height)
{
}

void CResumeGameModule::Dispose()
{
	// TODO: Delete and dispose of objects
}

void CResumeGameModule::KeyDown(int key, int lParam)
{
	if (key == 'Y')
	{
		Yes();
	}
	else if (key == 'N')
	{
		No();
	}
}

void CResumeGameModule::Yes()
{
	CModuleController::Pop(this);
	CGameController::LoadGame("GAMES\\SAVEGAME.000");
	CMainMenuModule::MainMenuModule->EnableSaveAndResume(true);
	CMainMenuModule::UpdateSaveGameData();
}

void CResumeGameModule::No()
{
	CModuleController::Pop(this);
	CModuleController::Push(new CVideoModule(VideoType::Single, "TITLE.AP", 0));
}

void CResumeGameModule::SetTextColours(int colour1, int colour2, int colour3, int colour4)
{
	TextColour1 = colour1;
	TextColour2 = colour2;
	TextColour3 = colour3;
	TextColour4 = colour4;
}

void CResumeGameModule::SetHeaderColours(int colour1, int colour2, int colour3, int colour4)
{
	HeaderColour1 = colour1;
	HeaderColour2 = colour2;
	HeaderColour3 = colour3;
	HeaderColour4 = colour4;
}