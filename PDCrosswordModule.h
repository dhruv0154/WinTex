#pragma once

#include "FullScreenModule.h"
#include "RawFont.h"
#include "BinaryData.h"
#include "Utilities.h"
#include "AmbientAudio.h"

class CPDCrosswordModule : public CFullScreenModule
{
public:
	CPDCrosswordModule();
	~CPDCrosswordModule();

	virtual void Dispose();
	virtual void Render();
	virtual void KeyDown(int key, int lParam);

protected:
	virtual void Initialize();
	CRawFont _font;
	CAmbientAudio _sound;
	bool _updateTexture;
	static const char* Solution;
	static const char* CheatSequence;

	// Input related
	virtual void Cursor(float x, float y, bool relative);
	virtual void BeginAction();
	virtual void Back();

	bool CheckCompleted();

	int _currentCellX;
	int _currentCellY;
	int _advanceX;
	int _advanceY;

	bool IsBlocked(int x, int y);
};