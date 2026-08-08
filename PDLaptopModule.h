#pragma once

#include "FullScreenModule.h"
#include "RawFont.h"
#include <cstdint>

class CPDLaptopModule : public CFullScreenModule
{
public:
	CPDLaptopModule(bool cdUsed);
	CPDLaptopModule(uint8_t* screen, int* palette);
	~CPDLaptopModule();

	virtual void Render();

protected:
	virtual void Initialize();

	CRawFont _pdRawFont;

	void RenderScreen();

	// Input related
	virtual void Cursor(float x, float y, bool relative);
	virtual void BeginAction();
	virtual void Back();

	bool _cdUsed;

	int _mode;
	int _stage;
	int _dotX;
	uint64_t _time;

	static int CellSequence[];

	void RenderButton(int x1, int y1, int x2, int y2, const char* text, int boxColour, int textColour);

	void RenderArticleButtons();
};