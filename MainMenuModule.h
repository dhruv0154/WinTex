#pragma once

#include "ModuleBase.h"
#include "DXScreen.h"
#include "DXFrame.h"
#include "SaveGameInfo.h"
#include "SaveGameControl.h"
#include "DXTabControl.h"
#include "DXTabItem.h"
#include "InputMapping.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>
#include "DXControlButton.h"
#include "DXSlider.h"
#include "DXLabel.h"

enum class SaveMode
{
	Load,
	Extension,
	Comment
};

class CMainMenuModule : public CModuleBase
{
public:
	CMainMenuModule();
	virtual ~CMainMenuModule();

	virtual void Resize(int width, int height);

	virtual void Render();

	static CMainMenuModule* MainMenuModule;

	static void GameOver();

	static void EnableSaveAndResume(bool enable);
	static void UpdateSaveGameData();

	static void SetPlayerNameAndEnableButtons();

protected:
	virtual void Initialize();

	virtual void SetupScreen() = 0;
	virtual void SetupConfigFrame();
	virtual void SetupLoadFrame();
	virtual void SetupSaveFrame();

	virtual void SetupSave() = 0;
	virtual void SetupLoad() = 0;

	static void NewGame(void* data);
	static void Resume(void* data);
	static void Load(void* data);
	static void Save(void* data);
	static void Config(void* data);
	static void Quit(void* data);

	static CDXButton* _btnMainResume;
	static CDXButton* _btnMainSave;

	static void ConfigCancel(void* data);
	static void ConfigAccept(void* data);

	static void ConfigPreviousResolution(void* data);
	static void ConfigNextResolution(void* data);

	static void ConfigPreviousMIDIDevice(void* data);
	static void ConfigNextMIDIDevice(void* data);

	static CConfiguration cfg;

	static void UpdateResolutionLabel();
	static void UpdateMIDIDeviceLabel();

	static CDXScreen* _pScreen;
	static CDXFrame* _pConfig;
	static CDXTabControl* _pConfigTab;
	static CDXTabItem* _pConfigVideo;
	static CDXTabItem* _pConfigAudio;
	static CDXTabItem* _pConfigControl;
	static CDXTabItem* _pConfigMisc;
	static CDXTabControl* _pConfigControlTab;
	static CDXTabItem* _pConfigControlKeyMouse;
	static CDXTabItem* _pConfigControlJoystick;
	static CDXFrame* _pLoad;
	static CDXFrame* _pSave;
	static CDXLabel* pResolution;
	static CDXLabel* pMIDIDevice;

	static CDXLabel* pMIDIVolume;
	static CDXLabel* pVolume;
	static CDXLabel* pFontScale;

	static CDXSlider* pMIDIVolumeSlider;
	static CDXSlider* pVolumeSlider;
	static CDXSlider* pFontScaleSlider;

	static CDXButton* _pConfigCancelBtn;
	static CDXButton* _pConfigAcceptBtn;

	static void LoadCancel(void* data);
	static void LoadLoad(void* data);

	static std::vector<SaveGameInfo> _savedGames;
	static std::vector<CSaveGameControl*> _saveGameControls;

	static void LoadGame(SaveGameInfo info);

	static void LoadSetup();
	static int _loadTopIndex;
	static int _loadVisibleSavesCount;
	static void LoadScroll(int top);

	static void SaveCancel(void* data);
	static void SaveSave(void* data);
	static void SaveIncrementSave(void* data);
	static CSaveGameControl* _saveControl;

	static SaveGameInfo CurrentGameInfo;
	static int SaveTypedChars;

	static CDXText _saveCursor;

	static char _commentBuffer[256];
	static int _caretPos;

	void Clear();

	static SaveMode CurrentSaveMode;

	static void ConfigureControl(InputAction data);
	static void ConfigControlsCancel(InputAction data);
	static bool ConfiguringControl;
	static InputAction ControlInputAction;

	static CDXButton* _pCancelConfigControlBtn;
	static std::unordered_map<InputAction, InputMap> _controlMapping;
	static CDXControlButton* _pConfiguredControl;
	static std::unordered_map<InputAction, CDXControlButton*> _mouseKeyControls;
	static std::unordered_map<InputAction, CDXControlButton*> _joystickControls;

	static void MapControl(CControllerData* pControllerData);

	virtual void Scroll(int direction);

	static CDXSlider* _pSliderDragging;

	// Input related
	virtual void Cursor(float x, float y, bool relative);
	virtual void BeginAction();
	virtual void EndAction();
	virtual void Back();
	virtual void Next();
	virtual void Prev();

	bool IsValidForControlConfiguration(InputSource source);
	virtual void MouseMove(Point pt);
	virtual void MouseDown(Point pt, int btn);
	virtual void MouseWheel(int scroll);
	virtual void KeyDown(int key, int lParam);
	virtual void GamepadInput(InputSource source, int offset, int data);
};