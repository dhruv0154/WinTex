#pragma once

#include "FullScreenModule.h"
#include "AmbientAudio.h"
#include "RawFont.h"
#include <cstdint>

class CPDCabinKeypadModule : public CFullScreenModule
{
public:
	CPDCabinKeypadModule();
	~CPDCabinKeypadModule();

	virtual void Dispose();
	virtual void Render();

protected:
	virtual void Initialize();

	// Input related
	virtual void Cursor(float x, float y, bool relative);
	virtual void BeginAction();
	virtual void Back();

	CRawFont _rawFont;

	void RenderScreen();
	void RenderMessage(const char* message, int colour);

	int _enteredCode;
	CAmbientAudio _sound;

	int _keyStates[16];
	int _keyStateDirections[16];
	uint64_t _keyStateUpdateTime[16];

	static int CabinKeyPositions[];

	int _mode;
	int _flashCount;
	uint64_t _flashtime;
	int _redBackup;
};