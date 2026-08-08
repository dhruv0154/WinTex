#include "MainMenuModule.h"
#include "DXImageButton.h"
#include "Utilities.h"
#include "GameController.h"
#include "SaveGameControl.h"
#include <algorithm>
#include "AmbientAudio.h"
#include "DXControlButton.h"
#include "Gamepad.h"
#include "Items.h"
#include "DXCheckBox.h"
#include <chrono>
#include <SDL2/SDL.h>

// Temporary virtual key mappings for engine decoupling
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_RETURN 0x0D
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_PRIOR 0x21
#define VK_NEXT 0x22

CConfiguration CMainMenuModule::cfg{};

CDXScreen* CMainMenuModule::_pScreen = NULL;
CDXFrame* CMainMenuModule::_pLoad = NULL;
CDXFrame* CMainMenuModule::_pSave = NULL;
CDXLabel* CMainMenuModule::pResolution = NULL;
CDXLabel* CMainMenuModule::pMIDIDevice = NULL;

CDXLabel* CMainMenuModule::pMIDIVolume = NULL;
CDXLabel* CMainMenuModule::pVolume = NULL;
CDXLabel* CMainMenuModule::pFontScale = NULL;

CDXSlider* CMainMenuModule::pMIDIVolumeSlider = NULL;
CDXSlider* CMainMenuModule::pVolumeSlider = NULL;
CDXSlider* CMainMenuModule::pFontScaleSlider = NULL;

CDXFrame* CMainMenuModule::_pConfig = NULL;
CDXTabControl* CMainMenuModule::_pConfigTab = NULL;
CDXTabItem* CMainMenuModule::_pConfigVideo = NULL;
CDXTabItem* CMainMenuModule::_pConfigAudio = NULL;
CDXTabItem* CMainMenuModule::_pConfigControl = NULL;
CDXTabItem* CMainMenuModule::_pConfigMisc = NULL;
CDXTabControl* CMainMenuModule::_pConfigControlTab = NULL;
CDXTabItem* CMainMenuModule::_pConfigControlKeyMouse = NULL;
CDXTabItem* CMainMenuModule::_pConfigControlJoystick = NULL;

CDXButton* CMainMenuModule::_btnMainResume = NULL;
CDXButton* CMainMenuModule::_btnMainSave = NULL;
CMainMenuModule* CMainMenuModule::MainMenuModule = NULL;

CDXButton* CMainMenuModule::_pConfigCancelBtn = NULL;
CDXButton* CMainMenuModule::_pConfigAcceptBtn = NULL;

std::vector<SaveGameInfo> CMainMenuModule::_savedGames;
std::vector<CSaveGameControl*> CMainMenuModule::_saveGameControls;
int CMainMenuModule::_loadTopIndex = 0;
int CMainMenuModule::_loadVisibleSavesCount = 0;

SaveGameInfo CMainMenuModule::CurrentGameInfo;

CSaveGameControl* CMainMenuModule::_saveControl = NULL;
SaveMode CMainMenuModule::CurrentSaveMode = SaveMode::Load;
int CMainMenuModule::SaveTypedChars = 0;
CDXText CMainMenuModule::_saveCursor;
char CMainMenuModule::_commentBuffer[256];
int CMainMenuModule::_caretPos = 0;

bool CMainMenuModule::ConfiguringControl = false;
InputAction CMainMenuModule::ControlInputAction = InputAction::Cursor;
CDXButton* CMainMenuModule::_pCancelConfigControlBtn = NULL;
std::unordered_map<InputAction, InputMap> CMainMenuModule::_controlMapping;
CDXControlButton* CMainMenuModule::_pConfiguredControl = NULL;

std::unordered_map<InputAction, CDXControlButton*> CMainMenuModule::_mouseKeyControls;
std::unordered_map<InputAction, CDXControlButton*> CMainMenuModule::_joystickControls;

CDXSlider* CMainMenuModule::_pSliderDragging = NULL;

CMainMenuModule::CMainMenuModule() : CModuleBase(ModuleType::MainMenu)
{
	_pScreen = new CDXScreen();
	_pScreen->SetSize(static_cast<float>(dx.GetWidth()), static_cast<float>(dx.GetHeight()));

	MainMenuModule = this;

	_saveCursor.SetText("_");
	_saveCursor.SetColours(0xff616161, 0xff710000, 0xffae0000, 0xffffffff);
}

CMainMenuModule::~CMainMenuModule()
{
	Clear();
}

void CMainMenuModule::Initialize()
{
	_cursorPosX = _cursorMaxX / 2.0f;
	_cursorPosY = _cursorMaxY / 2.0f;

	SetupScreen();
}

void CMainMenuModule::ConfigCancel(void* data)
{
	_pScreen->PopModal();
	// Revert config changes
	cfg = *pConfig;

	pMIDIVolumeSlider->CalculateSliderPosition();
	pMIDIVolumeSlider->UpdateValueText();
	pVolumeSlider->CalculateSliderPosition();
	pVolumeSlider->UpdateValueText();
	pFontScaleSlider->CalculateSliderPosition();
	pFontScaleSlider->UpdateValueText();

	_controlMapping = CInputMapping::ControlsMap;

	for (auto control : _mouseKeyControls)
	{
		control.second->UpdateControlText(&_controlMapping, control.first);
	}
}

void CMainMenuModule::ConfigAccept(void* data)
{
	_pScreen->PopModal();

	bool change_size = (pConfig->Width != cfg.Width) || (pConfig->Height != cfg.Height);
	bool change_fs = (pConfig->FullScreen != cfg.FullScreen);
	bool changeMIDIDevice = (pConfig->PlayMIDI && pConfig->MIDIDeviceId != cfg.MIDIDeviceId);
	bool changeFilter = (pConfig->AnisotropicFilter != cfg.AnisotropicFilter);
	bool changeFontSize = (pConfig->FontScale != cfg.FontScale);

	*pConfig = cfg;
	pConfig->Save();

	if (change_fs)
	{
		dx.SetFullScreen(cfg.FullScreen);
	}

	if (change_size || changeFontSize)
	{
		dx.Resize(cfg.Width, cfg.Height);

		if (_pLoad != NULL)
		{
			_pScreen->RemoveChild(_pLoad);
			delete _pLoad;
			_pLoad = NULL;
		}

		if (_pSave != NULL)
		{
			_pScreen->RemoveChild(_pSave);
			delete _pSave;
			_pSave = NULL;
		}

		CModuleController::Resize(cfg.Width, cfg.Height);

		CConstantBuffers::Setup2D(dx);

		_saveCursor.SetText("_");

		CItems::ResetText();
	}

	if (changeFilter)
	{
		dx.SelectSampler(cfg.AnisotropicFilter);
	}

	if (changeMIDIDevice)
	{
		pMIDI->OpenDevice(cfg.MIDIDeviceId);
	}

	pMIDI->SetVolume(((float)cfg.MIDIVolume) / 100.0f);
	CDXSound::SetVolume(((float)cfg.Volume) / 100.0f);

	CInputMapping::ControlsMap = _controlMapping;
	CInputMapping::SaveControlsMap();
}

void CMainMenuModule::ConfigPreviousResolution(void* data)
{
	if (cfg.ScreenMode > pConfig->MinAcceptedMode && pConfig->pAdapter != NULL)
	{
		cfg.ScreenMode--;
		cfg.Width = pConfig->pAdapter->_displayModeList[cfg.ScreenMode].Width;
		cfg.Height = pConfig->pAdapter->_displayModeList[cfg.ScreenMode].Height;

		UpdateResolutionLabel();
	}
}

void CMainMenuModule::ConfigNextResolution(void* data)
{
	if (pConfig->pAdapter != NULL && cfg.ScreenMode < (static_cast<int>(pConfig->pAdapter->_numModes) - 1))
	{
		cfg.ScreenMode++;
		cfg.Width = pConfig->pAdapter->_displayModeList[cfg.ScreenMode].Width;
		cfg.Height = pConfig->pAdapter->_displayModeList[cfg.ScreenMode].Height;

		UpdateResolutionLabel();
	}
}

void CMainMenuModule::UpdateResolutionLabel()
{
	auto labelText = std::to_string(cfg.Width) + " x " + std::to_string(cfg.Height);
	pResolution->SetText(labelText.c_str());
}

void CMainMenuModule::ConfigPreviousMIDIDevice(void* data)
{
	if (pConfig->MIDIDevices.size() != 0 && cfg.MIDIDeviceId > 0)
	{
		cfg.MIDIDeviceId--;
		UpdateMIDIDeviceLabel();
	}
}

void CMainMenuModule::ConfigNextMIDIDevice(void* data)
{
	if (pConfig->MIDIDevices.size() != 0 && cfg.MIDIDeviceId < (pConfig->NumberOfMIDIOutDevices - 1))
	{
		cfg.MIDIDeviceId++;
		UpdateMIDIDeviceLabel();
	}
}

void CMainMenuModule::UpdateMIDIDeviceLabel()
{
	if (cfg.MIDIDeviceId > -1) {
		MIDIOUTCAPSA midiCaps = pConfig->MIDIDevices.at(cfg.MIDIDeviceId);
		pMIDIDevice->SetText(midiCaps.szPname);
	}
	else {
		pMIDIDevice->SetText("None");
	}
}

void CMainMenuModule::NewGame(void* data)
{
	pMIDI->Stop();
	CAmbientAudio::StopAll();
	CAmbientAudio::Clear();
	CGameController::NewGame();
}

void CMainMenuModule::SetPlayerNameAndEnableButtons()
{
	CurrentGameInfo.Player = "TEX";
	CurrentGameInfo.FileName = "GAMES\\TEX___00.000";
	EnableSaveAndResume(true);
}

void CMainMenuModule::Resume(void* data)
{
	CModuleController::SendToBack(MainMenuModule);
}

bool CompareSaveGames(const SaveGameInfo& first, const SaveGameInfo& second)
{
	unsigned int i = 0;
	while ((i < first.DateTime.length()) && (i < second.DateTime.length()))
	{
		if (tolower(first.DateTime[i]) < tolower(second.DateTime[i])) return false;
		else if (tolower(first.DateTime[i]) > tolower(second.DateTime[i])) return true;
		++i;
	}

	return (first.DateTime.length() > second.DateTime.length());
}

void CMainMenuModule::Load(void* data)
{
	if (_pLoad == NULL)
	{
		(MainMenuModule)->SetupLoadFrame();
	}

	(MainMenuModule)->SetupLoad();
}

void CMainMenuModule::Save(void* data)
{
	if (_pSave == NULL)
	{
		(MainMenuModule)->SetupSaveFrame();
	}

	(MainMenuModule)->SetupSave();

	_pScreen->ShowModal(_pSave);
}

void CMainMenuModule::Config(void* data)
{
	cfg = *pConfig;
	if (_pConfig == NULL)
	{
		(MainMenuModule)->SetupConfigFrame();
	}

	UpdateResolutionLabel();
	UpdateMIDIDeviceLabel();

	_pScreen->ShowModal(_pConfig);
}

void CMainMenuModule::Quit(void* data)
{
	SDL_Event event;
	event.type = SDL_QUIT;
	SDL_PushEvent(&event);
}

void CMainMenuModule::Render()
{
	CDXFont::SelectBlackFont();
	_pScreen->Render();

	uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	if (CurrentSaveMode != SaveMode::Load && (now / 500) % 2)
	{
		if (CurrentSaveMode == SaveMode::Extension)
		{
			SaveGameInfo info = _saveControl->GetInfo();
			char buffer[32];
			memset(buffer, 0, 32);
			for (int i = 0; i < 32 && i < info.FileName.length(); i++)
			{
				buffer[i] = info.FileName.at(i) & 0xFF;
			}
			float x = _saveControl->GetColumn2() + ceil(TexFont.PixelWidth(buffer + 6));
			float y = _saveControl->GetY() + 8 * pConfig->FontScale;
			_saveCursor.Render(x, y);
		}
		else if (CurrentSaveMode == SaveMode::Comment)
		{
			float y = _saveControl->GetY() + 68 * pConfig->FontScale;
			_saveCursor.Render(static_cast<float>(_caretPos), y);
		}
	}

	CModuleController::Cursors[0].SetPosition(_cursorPosX, _cursorPosY);
	CModuleController::Cursors[0].Render();
}

void CMainMenuModule::Resize(int width, int height)
{
	bool saveEnabled = _btnMainSave->GetEnabled();
	bool resumeVisible = _btnMainResume->GetVisible();

	Clear();

	if (!cfg.FullScreen)
	{
		if (::_hWnd != nullptr) 
		{
			SDL_SetWindowSize((SDL_Window*)::_hWnd, width, height);
			SDL_SetWindowPosition((SDL_Window*)::_hWnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		}
	}

	_pScreen = new CDXScreen();
	_pScreen->SetSize(static_cast<float>(width), static_cast<float>(height));
	SetupScreen();

	_cursorMaxX = width - 1;
	_cursorMaxY = height - 1;

	_cursorPosX = width / 2.0f;
	_cursorPosY = height / 2.0f;

	_btnMainSave->SetEnabled(saveEnabled);
	_btnMainResume->SetVisible(resumeVisible);
}

void CMainMenuModule::GameOver()
{
	EnableSaveAndResume(false);
}

void CMainMenuModule::LoadCancel(void* data)
{
	_pScreen->PopModal();
}

void CMainMenuModule::LoadLoad(void* info)
{
	_pScreen->PopModal();
}

void CMainMenuModule::LoadGame(SaveGameInfo info)
{
	CurrentGameInfo = info;

	_pScreen->PopModal();
	
	if (::_hWnd != nullptr) 
	{
		SDL_WarpMouseInWindow((SDL_Window*)::_hWnd, dx.GetWidth() / 2, dx.GetHeight() / 2);
	}

	CAmbientAudio::StopAll();
	CAmbientAudio::Clear();
	videoMode = VideoMode::FullScreen;
	CGameController::LoadGame(info.FileName.c_str());
	EnableSaveAndResume(true);
}

void CMainMenuModule::LoadSetup()
{
	int ix = 0;
	int w = dx.GetWidth();
	int h = dx.GetHeight();
	int y = 32;

	while (ix < _saveGameControls.size() && ix < _savedGames.size())
	{
		CSaveGameControl* sgc = _saveGameControls.at(ix);
		if ((y + sgc->GetHeight()) > (h - (64 * pConfig->FontScale)))
		{
			break;
		}
		SaveGameInfo sgi = _savedGames.at(_loadTopIndex + ix);
		sgc->SetInfo(sgi);
		sgc->SetPosition(16.0f, static_cast<float>(y));
		sgc->SetWidth(w - 80.0f);
		sgc->SetVisible(true);
		y += static_cast<int>(sgc->GetHeight() + 8 * pConfig->FontScale);
		ix++;
	}

	_loadVisibleSavesCount = ix;

	while (ix < _saveGameControls.size())
	{
		CSaveGameControl* sgc = _saveGameControls.at(ix++);
		sgc->SetVisible(false);
	}
}

void CMainMenuModule::LoadScroll(int top)
{
	_loadTopIndex = std::min(static_cast<int>(_savedGames.size()) - _loadVisibleSavesCount, std::max(0, top));
	LoadSetup();
}

void CMainMenuModule::SaveCancel(void* data)
{
	CurrentSaveMode = SaveMode::Load;
	_pScreen->PopModal();
}

void CMainMenuModule::SaveSave(void* data)
{
	if (_saveControl != NULL)
	{
		int commentOffset = CGameController::GetSaveCommentOffset();
		int commentLength = CGameController::GetSaveCommentLength();
		for (int i = 0; i < commentLength; i++)
		{
			CGameController::SetData(commentOffset + i, _commentBuffer[i]);
		}

		SaveGameInfo info = _saveControl->GetInfo();
		CGameController::SaveGame(info.FileName.c_str());
		CurrentGameInfo.FileName = info.FileName;
	}

	CurrentSaveMode = SaveMode::Load;
	_pScreen->PopModal();
}

void CMainMenuModule::SaveIncrementSave(void* data)
{
	if (_saveControl != NULL)
	{
		SaveGameInfo info = _saveControl->GetInfo();
		std::string fn = info.FileName;
		size_t len = fn.length();

		int index = std::stoi(fn.substr(len - 3)) + 1;
		if (index < 1000)
		{
			fn[len - 1] = '0' + index % 10;
			fn[len - 2] = '0' + (index / 10) % 10;
			fn[len - 3] = '0' + (index / 100) % 10;

			CurrentGameInfo.FileName = fn;

			int commentOffset = CGameController::GetSaveCommentOffset();
			int commentLength = CGameController::GetSaveCommentLength();
			for (int i = 0; i < commentLength; i++)
			{
				CGameController::SetData(commentOffset + i, (uint8_t)0);
			}

			CGameController::SaveGame(fn.c_str());

			CurrentSaveMode = SaveMode::Load;
			_pScreen->PopModal();
		}
	}
}

void CMainMenuModule::Clear()
{
	if (_pScreen != NULL)
	{
		delete _pScreen;
		_pScreen = NULL;
	}

	_saveGameControls.clear();

	for (auto sgc : _saveGameControls)
	{
		delete sgc;
	}

	if (_pConfig != NULL)
	{
		_pConfig = NULL;
	}
}

void CMainMenuModule::ConfigureControl(InputAction data)
{
	ConfiguringControl = true;
	ControlInputAction = data;
	_pCancelConfigControlBtn->SetVisible(true);
	_pConfigCancelBtn->SetVisible(false);
	_pConfigAcceptBtn->SetVisible(false);
	_pConfiguredControl->SetIsBeingConfigured(true);
}

void CMainMenuModule::ConfigControlsCancel(InputAction data)
{
	ConfiguringControl = false;
	_pConfiguredControl->SetIsBeingConfigured(false);
	_pCancelConfigControlBtn->SetVisible(false);
	_pConfigCancelBtn->SetVisible(true);
	_pConfigAcceptBtn->SetVisible(true);
}

void CMainMenuModule::MapControl(CControllerData* pControllerData)
{
	bool valid = (IsJoystickSource(pControllerData->Source) == _pConfiguredControl->IsJoystickConfigControl());
	if (valid)
	{
		if (IsJoystickSource(pControllerData->Source))
		{
			int offset = pControllerData->Offset;
			if (offset == 0 || offset == 4)
			{
				offset = 0;
			}
			else if (offset == 8 || offset == 20)
			{
				offset = 20;
			}
			else if (offset == 12 || offset == 16)
			{
				offset = 12;
			}

			if (ControlInputAction == InputAction::MoveForward || ControlInputAction == InputAction::MoveBack || ControlInputAction == InputAction::MoveLeft || ControlInputAction == InputAction::MoveRight)
			{
				_controlMapping[InputAction::MoveForward].JoystickSource = _controlMapping[InputAction::MoveBack].JoystickSource = _controlMapping[InputAction::MoveLeft].JoystickSource = _controlMapping[InputAction::MoveRight].JoystickSource = pControllerData->Source;
				_controlMapping[InputAction::MoveForward].JoystickIdentifier = _controlMapping[InputAction::MoveBack].JoystickIdentifier = _controlMapping[InputAction::MoveLeft].JoystickIdentifier = _controlMapping[InputAction::MoveRight].JoystickIdentifier = offset;

				_joystickControls[InputAction::MoveForward]->UpdateControlText(pControllerData);
			}
			else
			{
				_controlMapping[ControlInputAction].JoystickSource = pControllerData->Source;
				_controlMapping[ControlInputAction].JoystickIdentifier = pControllerData->Offset;
			}
		}
		else
		{
			_controlMapping[ControlInputAction].MouseKeySource = pControllerData->Source;
			_controlMapping[ControlInputAction].MouseKeyIdentifier = pControllerData->Offset;
		}

		_pConfiguredControl->UpdateControlText(pControllerData);
		_pConfiguredControl->SetIsBeingConfigured(false);
		_pCancelConfigControlBtn->SetVisible(false);
		_pConfigCancelBtn->SetVisible(true);
		_pConfigAcceptBtn->SetVisible(true);
		ConfiguringControl = false;
	}
}

void CMainMenuModule::Cursor(float x, float y, bool relative)
{
	CModuleBase::Cursor(x, y, relative);

	if (_pSliderDragging == NULL)
	{
		CDXControl* pPrevMouseOver = _pScreen->GetCurrentMouseOver();
		CDXControl* pHit = _pScreen->HitTest(_cursorPosX, _cursorPosY);
		if (pPrevMouseOver != NULL && (pHit == NULL || pHit != pPrevMouseOver))
		{
			pPrevMouseOver->SetMouseOver(false);
		}

		if (pHit != NULL && pHit->GetEnabled())
		{
			CDXControl::ControlType ct = pHit->GetType();
			if (ct == CDXControl::ControlType::Button || ct == CDXControl::ControlType::ImageButton || ct == CDXControl::ControlType::CheckBox || ct == CDXControl::ControlType::SaveGameControl || ct == CDXControl::ControlType::Control || ct == CDXControl::ControlType::Slider)
			{
				CDXControl* pBtn = (CDXControl*)pHit;
				if (pBtn != NULL && !pBtn->GetMouseOver())
				{
					pBtn->SetMouseOver(true);
				}
			}
		}
	}
	else
	{
		_pSliderDragging->Drag(_cursorPosX, _cursorPosY);
	}
}

void CMainMenuModule::BeginAction()
{
	float x = _cursorPosX;
	float y = _cursorPosY;
	CDXControl* pHit = _pScreen->HitTest(x, y);
	if (pHit != NULL && pHit->GetEnabled())
	{
		CDXControl::ControlType ct = pHit->GetType();
		if (ct == CDXControl::ControlType::Button || ct == CDXControl::ControlType::ImageButton || ct == CDXControl::ControlType::Control)
		{
			CDXButton* pBtn = (CDXButton*)pHit;
			if (pBtn != NULL)
			{
				if (!ConfiguringControl || pBtn == _pCancelConfigControlBtn)
				{
					if (ct == CDXControl::ControlType::Control)
					{
						_pConfiguredControl = (CDXControlButton*)pHit;
					}
					pBtn->Click();
				}
			}
		}
		else if (ct == CDXControl::ControlType::CheckBox)
		{
			CDXCheckBox* pCB = (CDXCheckBox*)pHit;
			if (pCB != NULL)
			{
				pCB->SetCheck(!pCB->GetCheck());
			}
		}
		else if (ct == CDXControl::ControlType::SaveGameControl)
		{
			CSaveGameControl* pSGC = (CSaveGameControl*)pHit;
			if (pSGC != NULL)
			{
				if (CurrentSaveMode == SaveMode::Load)
				{
					pSGC->Click();
				}
				else if (CurrentSaveMode == SaveMode::Extension)
				{
					float cy = _cursorPosY - _saveControl->GetY();
					if (cy >= 68 * pConfig->FontScale && cy <= 83 * pConfig->FontScale && SaveTypedChars == 3)
					{
						CurrentSaveMode = SaveMode::Comment;
						SaveGameInfo info = _saveControl->GetInfo();
						SaveTypedChars = static_cast<int>(info.Comment.length());
						_caretPos = static_cast<int>(_saveControl->GetColumn2() + ceil(TexFont.PixelWidth(_commentBuffer)));
					}
				}
				else if (CurrentSaveMode == SaveMode::Comment)
				{
					float cy = _cursorPosY - _saveControl->GetY();
					if (cy >= 8 * pConfig->FontScale && cy <= 22 * pConfig->FontScale && x < 350 * pConfig->FontScale)
					{
						CurrentSaveMode = SaveMode::Extension;
						SaveTypedChars = 3;
					}
				}
			}
		}
		else if (ct == CDXControl::ControlType::TabItem)
		{
			((CDXTabItem*)pHit)->Select();
		}
		else if (ct == CDXControl::ControlType::Slider)
		{
			_pSliderDragging = (CDXSlider*)pHit;
		}
	}
}

void CMainMenuModule::EndAction()
{
	if (_pSliderDragging != NULL)
	{
		_pSliderDragging = NULL;
	}
}

void CMainMenuModule::Back()
{
	if (_pScreen->IsModal() && _pConfiguredControl == NULL)
	{
		_pScreen->PopModal();
		CurrentSaveMode = SaveMode::Load;
	}
}

void CMainMenuModule::Next()
{
	Scroll(1);
}

void CMainMenuModule::Prev()
{
	Scroll(-1);
}

void CMainMenuModule::Scroll(int direction)
{
	if (_pScreen->GetModal() == _pLoad)
	{
		LoadScroll(_loadTopIndex + direction);
	}
}

bool CMainMenuModule::IsValidForControlConfiguration(InputSource source)
{
	return (ConfiguringControl && (_controlMapping[ControlInputAction].AcceptableSource & source) == source);
}

void CMainMenuModule::MouseMove(Point pt)
{
	if (IsValidForControlConfiguration(InputSource::Mouse) && !_pConfiguredControl->IsJoystickConfigControl())
	{
		CControllerData cdata;
		cdata.Source = InputSource::Mouse;
		cdata.Data = 0;
		MapControl(&cdata);
	}
}

void CMainMenuModule::MouseDown(Point pt, int btn)
{
	if (IsValidForControlConfiguration(InputSource::MouseButton) && !_pConfiguredControl->IsJoystickConfigControl())
	{
		CControllerData cdata;
		cdata.Source = InputSource::MouseButton;
		cdata.Data = btn;
		MapControl(&cdata);
	}
}

void CMainMenuModule::MouseWheel(int scroll)
{
	if (IsValidForControlConfiguration(InputSource::MouseWheel) && !_pConfiguredControl->IsJoystickConfigControl())
	{
		CControllerData cdata;
		cdata.Source = InputSource::MouseWheel;
		cdata.Offset = cdata.Data = scroll;
		MapControl(&cdata);
	}
}

void CMainMenuModule::KeyDown(int key, int lParam)
{
	if (ConfiguringControl)
	{
		if (IsValidForControlConfiguration(InputSource::Key) && !_pConfiguredControl->IsJoystickConfigControl())
		{
			CControllerData cdata;
			cdata.Source = InputSource::Key;
			cdata.Offset = cdata.Data = lParam & 0x00ff0000;
			MapControl(&cdata);
		}
	}
	else if (_pScreen->GetModal() == _pSave)
	{
		if (CurrentSaveMode == SaveMode::Extension)
		{
			if (key == VK_LEFT || key == VK_BACK)
			{
				if (SaveTypedChars > 0)
				{
					SaveGameInfo info = _saveControl->GetInfo();
					std::string fileName = info.FileName.substr(0, info.FileName.length() - 1);
					_saveControl->SetFileName(fileName);
					SaveTypedChars--;
				}
			}
			else if (key >= '0' && key <= '9')
			{
				if (SaveTypedChars < 3)
				{
					SaveGameInfo info = _saveControl->GetInfo();
					info.FileName += (char)key;
					_saveControl->SetFileName(info.FileName);
					SaveTypedChars++;
				}
			}
			else if ((key == VK_TAB || key == VK_RETURN) && SaveTypedChars == 3)
			{
				CurrentSaveMode = SaveMode::Comment;
				SaveGameInfo info = _saveControl->GetInfo();
				SaveTypedChars = static_cast<int>(info.Comment.length());
				_caretPos = static_cast<int>(_saveControl->GetColumn2());
			}
		}
		else if (CurrentSaveMode == SaveMode::Comment)
		{
			if (key == VK_LEFT || key == VK_BACK)
			{
				if (SaveTypedChars > 0)
				{
					_commentBuffer[--SaveTypedChars] = 0;
					_saveControl->SetComment(_commentBuffer);
					_caretPos = static_cast<int>(_saveControl->GetColumn2() + ceil(TexFont.PixelWidth(_commentBuffer)));
				}
			}
			else if (key == VK_RETURN)
			{
				SaveSave(NULL);
			}
			else if (SaveTypedChars < 0x90)
			{
				if (key >= 0x20 && key <= 0x7f)
				{
					float x = _saveControl->GetColumn2() + ceil(TexFont.PixelWidth(_commentBuffer));
					if (x < _saveControl->GetWidth() + 32)
					{
						_commentBuffer[SaveTypedChars++] = (char)(key & 0xFF);
						_saveControl->SetComment(_commentBuffer);
						_caretPos = static_cast<int>(_saveControl->GetColumn2() + ceil(TexFont.PixelWidth(_commentBuffer)));
					}
				}
			}
		}
	}
	else if (_pScreen->GetModal() == _pLoad)
	{
		if (key == VK_UP)
		{
			LoadScroll(_loadTopIndex - 1);
		}
		else if (key == VK_DOWN)
		{
			LoadScroll(_loadTopIndex + 1);
		}
		else if (key == VK_PRIOR)
		{
			LoadScroll(_loadTopIndex - _loadVisibleSavesCount);
		}
		else if (key == VK_NEXT)
		{
			LoadScroll(_loadTopIndex + _loadVisibleSavesCount);
		}
	}
}

void CMainMenuModule::GamepadInput(InputSource source, int offset, int data)
{
	if (IsValidForControlConfiguration(source) && _pConfiguredControl->IsJoystickConfigControl())
	{
		CControllerData cdata;
		cdata.Source = source;
		cdata.Offset = offset;
		cdata.Data = data;
		MapControl(&cdata);
	}
}

void CMainMenuModule::SetupConfigFrame()
{
	int w = dx.GetWidth();
	int h = dx.GetHeight();

	_pConfig = new CDXFrame("Configuration", w - 64.0f, h - 64.0f);
	_pScreen->AddChild(_pConfig, 32.0f, 32.0f);

	_pConfigTab = new CDXTabControl(w - 80.0f, h - 80.0f);
	_pConfig->AddChild(_pConfigTab, 8.0f, 8.0f);

	float fontHeight = TexFont.Height() * pConfig->FontScale;
	float buttonHeight = fontHeight + 24.0f;
	float checkBoxWidth = 250.0f + 72 * pConfig->FontScale;

	float tw = (w - 80.0f) / 3 - 2.0f;
	_pConfigVideo = new CDXTabItem(_pConfigTab, "Video", tw, h - 80.0f);
	_pConfigTab->AddChild(_pConfigVideo, 0.0f, 0.0f);

	float y = 46.0f + fontHeight;

	CDXImageButton* pLBtn = new CDXImageButton(0, ConfigPreviousResolution);
	_pConfigVideo->AddChild(pLBtn, checkBoxWidth - 50.0f, y);

	CDXImageButton* pRBtn = new CDXImageButton(1, ConfigNextResolution);
	_pConfigVideo->AddChild(pRBtn, checkBoxWidth + 30.0f, y);
	y += buttonHeight;

	CDXLabel* pLabel = new CDXLabel("Resolution");
	_pConfigVideo->AddChild(pLabel, 22.0f, y);

	pResolution = new CDXLabel("");
	_pConfigVideo->AddChild(pResolution, checkBoxWidth - 60.0f, y);
	y += buttonHeight;

	_pConfigVideo->AddChild(new CDXCheckBox("Full screen", &cfg.FullScreen, checkBoxWidth), 22.0f, y);
	y += buttonHeight;

	_pConfigVideo->AddChild(new CDXCheckBox("Captions", &cfg.Captions, checkBoxWidth), 22.0f, y);
	y += buttonHeight;

	_pConfigVideo->AddChild(new CDXCheckBox("Alternative media", &cfg.AlternativeMedia, checkBoxWidth), 22.0f, y);
	y += buttonHeight;

	_pConfigVideo->AddChild(new CDXCheckBox("Anisotropic filter", &cfg.AnisotropicFilter, checkBoxWidth), 22.0f, y);
	y += buttonHeight;

	_pConfigVideo->AddChild(pFontScaleSlider = new CDXSlider("Font scale", 1.0f, 3.0f, 0.25f, &cfg.FontScale, 2, checkBoxWidth - 10), 22.0f, y);

	_pConfigAudio = new CDXTabItem(_pConfigTab, "Audio", tw, h - 80.0f);
	_pConfigTab->AddChild(_pConfigAudio, 0.0f, 0.0f);

	y = 46.0f + fontHeight;

	_pConfigAudio->AddChild(pVolumeSlider = new CDXSlider("Volume", 0.0f, 100.0f, 1.0f, &cfg.Volume, 0, checkBoxWidth - 10), 22.0f, y);
	y += buttonHeight * 1.5f;

	_pConfigAudio->AddChild(new CDXCheckBox("MIDI", &cfg.PlayMIDI, checkBoxWidth), 22.0f, y);
	y += buttonHeight;

	CDXImageButton* pMLBtn = new CDXImageButton(0, ConfigPreviousMIDIDevice);
	_pConfigAudio->AddChild(pMLBtn, checkBoxWidth - 50.0f, y);

	CDXImageButton* pMRBtn = new CDXImageButton(1, ConfigNextMIDIDevice);
	_pConfigAudio->AddChild(pMRBtn, checkBoxWidth + 30.0f, y);
	y += buttonHeight;

	CDXLabel* pMIDILabel = new CDXLabel("MIDI Device");
	_pConfigAudio->AddChild(pMIDILabel, 22.0f, y);

	pMIDIDevice = new CDXLabel("");
	_pConfigAudio->AddChild(pMIDIDevice, checkBoxWidth - 60.0f, y);
	y += buttonHeight;

	_pConfigAudio->AddChild(pMIDIVolumeSlider = new CDXSlider("MIDI volume", 0.0f, 100.0f, 1.0f, &cfg.MIDIVolume, 0, checkBoxWidth - 10), 22.0f, y);

	_pConfigControl = new CDXTabItem(_pConfigTab, "Controls", tw, h - 80.0f);
	_pConfigTab->AddChild(_pConfigControl, 0.0f, 0.0f);

	y = 77.0f + fontHeight;

	_pConfigControlTab = new CDXTabControl(w - 80.0f, h - 80.0f);
	_pConfigControl->AddChild(_pConfigControlTab, 0.0f, 7 + fontHeight);

	float ctw = (w - 80.0f) / 2 - 2.0f;
	_pConfigControlKeyMouse = new CDXTabItem(_pConfigControlTab, "Mouse & Keyboard", ctw, h - 80.0f);
	_pConfigControlTab->AddChild(_pConfigControlKeyMouse, 0.0f, 10.0f);
	_pConfigControlJoystick = new CDXTabItem(_pConfigControlTab, "Joystick", ctw, h - 80.0f);
	_pConfigControlTab->AddChild(_pConfigControlJoystick, 0.0f, 10.0f);

	_pConfigControl->AddChild(new CDXCheckBox("Invert Y", &cfg.InvertY, checkBoxWidth), 22.0f, y);

	_controlMapping = CInputMapping::ControlsMap;

	_pCancelConfigControlBtn = new CDXButton("Cancel", 64.0f * pConfig->FontScale, 32.0f * ::pConfig->FontScale, [](void*) {
		ConfigControlsCancel(InputAction::Cursor);
	});
	_pCancelConfigControlBtn->SetVisible(false);
	_pConfigControl->AddChild(_pCancelConfigControlBtn, 24.0f, h - 72.0f - 54.0f * ::pConfig->FontScale);

	float x = 22.0f;

	_mouseKeyControls.clear();
	_mouseKeyControls[InputAction::Cursor] = new CDXControlButton("Cursor", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Cursor);
	_mouseKeyControls[InputAction::Action] = new CDXControlButton("Action", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Action);
	_mouseKeyControls[InputAction::Cycle] = new CDXControlButton("Cycle", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Cycle);
	_mouseKeyControls[InputAction::Back] = new CDXControlButton("Back", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Back);
	_mouseKeyControls[InputAction::Travel] = new CDXControlButton("Travel", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Travel);
	_mouseKeyControls[InputAction::Inventory] = new CDXControlButton("Inventory", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Inventory);
	_mouseKeyControls[InputAction::Run] = new CDXControlButton("Run", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Run);
	_mouseKeyControls[InputAction::Next] = new CDXControlButton("Next", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Next);
	_mouseKeyControls[InputAction::Prev] = new CDXControlButton("Previous", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Prev);
	_mouseKeyControls[InputAction::MoveForward] = new CDXControlButton("Move forward", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::MoveForward);
	_mouseKeyControls[InputAction::MoveBack] = new CDXControlButton("Move back", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::MoveBack);
	_mouseKeyControls[InputAction::MoveLeft] = new CDXControlButton("Move left", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::MoveLeft);
	_mouseKeyControls[InputAction::MoveRight] = new CDXControlButton("Move right", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::MoveRight);
	_mouseKeyControls[InputAction::MoveUp] = new CDXControlButton("Move up", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::MoveUp);
	_mouseKeyControls[InputAction::MoveDown] = new CDXControlButton("Move down", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::MoveDown);
	_mouseKeyControls[InputAction::Hints] = new CDXControlButton("Hints", &_controlMapping, false, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Hints);

	_mouseKeyControls[InputAction::Cursor]->SetEnabled(false);

	_joystickControls.clear();
	_joystickControls[InputAction::Cursor] = new CDXControlButton("Cursor", &_controlMapping, true, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Cursor);
	_joystickControls[InputAction::Action] = new CDXControlButton("Action", &_controlMapping, true, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Action);
	_joystickControls[InputAction::Cycle] = new CDXControlButton("Cycle", &_controlMapping, true, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Cycle);
	_joystickControls[InputAction::Back] = new CDXControlButton("Back", &_controlMapping, true, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Back);
	_joystickControls[InputAction::Travel] = new CDXControlButton("Travel", &_controlMapping, true, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Travel);
	_joystickControls[InputAction::Inventory] = new CDXControlButton("Inventory", &_controlMapping, true, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Inventory);
	_joystickControls[InputAction::Run] = new CDXControlButton("Run", &_controlMapping, true, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Run);
	_joystickControls[InputAction::Next] = new CDXControlButton("Next", &_controlMapping, true, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Next);
	_joystickControls[InputAction::Prev] = new CDXControlButton("Previous", &_controlMapping, true, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Prev);
	_joystickControls[InputAction::MoveForward] = new CDXControlButton("Movement", &_controlMapping, true, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::MoveForward);
	_joystickControls[InputAction::MoveUp] = new CDXControlButton("Move up", &_controlMapping, true, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::MoveUp);
	_joystickControls[InputAction::MoveDown] = new CDXControlButton("Move down", &_controlMapping, true, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::MoveDown);
	_joystickControls[InputAction::Hints] = new CDXControlButton("Hints", &_controlMapping, true, 0.0f, 0.0f, checkBoxWidth - x, ConfigureControl, InputAction::Hints);

	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::Cursor], x, y);
	_pConfigControlJoystick->AddChild(_joystickControls[InputAction::Cursor], x, y);
	y += fontHeight;
	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::Action], x, y);
	_pConfigControlJoystick->AddChild(_joystickControls[InputAction::Action], x, y);
	y += fontHeight;
	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::Back], x, y);
	_pConfigControlJoystick->AddChild(_joystickControls[InputAction::Back], x, y);
	y += fontHeight;
	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::Cycle], x, y);
	_pConfigControlJoystick->AddChild(_joystickControls[InputAction::Cycle], x, y);
	y += fontHeight;
	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::Next], x, y);
	_pConfigControlJoystick->AddChild(_joystickControls[InputAction::Next], x, y);
	y += fontHeight;
	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::Prev], x, y);
	_pConfigControlJoystick->AddChild(_joystickControls[InputAction::Prev], x, y);
	y += fontHeight;
	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::MoveForward], x, y);
	_pConfigControlJoystick->AddChild(_joystickControls[InputAction::MoveForward], x, y);
	y += fontHeight;
	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::MoveBack], x, y);
	_pConfigControlJoystick->AddChild(_joystickControls[InputAction::MoveUp], x, y);
	y += fontHeight;
	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::MoveLeft], x, y);
	_pConfigControlJoystick->AddChild(_joystickControls[InputAction::MoveDown], x, y);
	y += fontHeight;
	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::MoveRight], x, y);
	_pConfigControlJoystick->AddChild(_joystickControls[InputAction::Run], x, y);
	y += fontHeight;
	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::MoveUp], x, y);
	_pConfigControlJoystick->AddChild(_joystickControls[InputAction::Inventory], x, y);
	y += fontHeight;
	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::MoveDown], x, y);
	_pConfigControlJoystick->AddChild(_joystickControls[InputAction::Travel], x, y);
	y += fontHeight;
	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::Run], x, y);
	y += fontHeight;
	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::Inventory], x, y);
	y += fontHeight;
	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::Travel], x, y);
	y += fontHeight;
	_pConfigControlKeyMouse->AddChild(_mouseKeyControls[InputAction::Hints], x, y);
	y += fontHeight * 2.0f;
	_pConfigControlKeyMouse->AddChild(new CDXSlider("Mouse sensitivity", 0.25f, 2.0f, 0.05f, &cfg.MouselookScaling, 2, checkBoxWidth - 10), x, y);
	y += fontHeight * 2.0f;

	_pConfigCancelBtn = new CDXButton("Cancel", 64.0f * pConfig->FontScale, 32.0f * pConfig->FontScale, ConfigCancel);
	_pConfig->AddChild(_pConfigCancelBtn, 32.0f, h - 64.0f - 54.0f * pConfig->FontScale);

	_pConfigAcceptBtn = new CDXButton("Accept", 64.0f * pConfig->FontScale, 32.0f * pConfig->FontScale, ConfigAccept);
	_pConfig->AddChild(_pConfigAcceptBtn, w - 88.0f - 100.0f * pConfig->FontScale, h - 64.0f - 54.0f * pConfig->FontScale);
}

void CMainMenuModule::SetupLoadFrame()
{
	int w = dx.GetWidth();
	int h = dx.GetHeight();

	_pLoad = new CDXFrame("Load Game", w - 8.0f, h - 8.0f);
	_pScreen->AddChild(_pLoad, 4.0f, 4.0f);

	_pLoad->AddChild(new CDXButton("Cancel", 64.0f * pConfig->FontScale, 32.0f * pConfig->FontScale, LoadCancel), 12.0f, h - 64.0f * pConfig->FontScale);

	for (int i = 0; i < 10; i++)
	{
		CSaveGameControl* sgc = new CSaveGameControl(LoadGame);
		_saveGameControls.push_back(sgc);
		_pLoad->AddChild(sgc, 16.0f, 0.0f);
	}
}

void CMainMenuModule::SetupSaveFrame()
{
	int w = dx.GetWidth();
	int h = dx.GetHeight();

	_pSave = new CDXFrame("Save Game", w - 8.0f, h - 8.0f);
	_pScreen->AddChild(_pSave, 4.0f, 4.0f);

	_pSave->AddChild(new CDXButton("Cancel", 64.0f * pConfig->FontScale, 32.0f * pConfig->FontScale, SaveCancel), 12.0f, h - 64.0f * pConfig->FontScale);
	_saveControl = new CSaveGameControl(NULL, true);
	_pSave->AddChild(_saveControl, 12, (h - _saveControl->GetHeight()) / 2);

	CDXButton* pSaveBtn = new CDXButton("Save", 64.0f * pConfig->FontScale, 32.0f * pConfig->FontScale, SaveSave);
	_pSave->AddChild(pSaveBtn, w - 128.0f * pConfig->FontScale, h - 64.0f * pConfig->FontScale);

	CDXButton* pIncrementAndSaveBtn = new CDXButton("Increment", 64.0f * pConfig->FontScale, 32.0f * pConfig->FontScale, SaveIncrementSave);
	_pSave->AddChild(pIncrementAndSaveBtn, w / 2 - 32.0f, h - 64.0f * pConfig->FontScale);
}

void CMainMenuModule::EnableSaveAndResume(bool enable)
{
	_btnMainResume->SetVisible(enable);
	_btnMainSave->SetEnabled(enable);
}

void CMainMenuModule::UpdateSaveGameData()
{
	CurrentGameInfo.Player = "TEX";
	CurrentGameInfo.FileName = "GAMES\\TEX___00.000";
}