#pragma once

#include "ModuleBase.h"
#include "DXMultiColouredText.h"
#include "DXCheckBox.h"
#include "DXButton.h"
#include "DXFrame.h"
#include <cstdint>

class CPDSelectLevelModule : public CModuleBase
{
public:
	CPDSelectLevelModule(uint8_t* pGameData);
	~CPDSelectLevelModule();

	virtual void Resize(int width, int height);
	virtual void Dispose();
	virtual void Render();
	virtual void KeyDown(int key, int lParam);
	virtual void Cursor(float x, float y, bool relative);

protected:
	virtual void Initialize();

	// Input related
	virtual void BeginAction();
	virtual void Back();

	uint8_t* _gameData;
	void NewGame();

	CDXMultiColouredText _line1;
	CDXMultiColouredText _line2;
	CDXMultiColouredText _line3;
	CDXMultiColouredText _line4;

	// Checkbox buttons for Level (Entertainment/Game player)
	CDXCheckBox* _pCBEntertainment;
	CDXCheckBox* _pCBGamePlayer;

	// Buttons for OK & Cancel
	CDXButton* _pBtnOK;
	CDXButton* _pBtnCancel;

	bool _bEntertainment;
	bool _bGamePlayer;
};
