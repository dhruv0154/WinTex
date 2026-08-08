#pragma once

#include "FullScreenModule.h"
#include "AmbientAudio.h"
#include <cstdint>

class CPDRitzSecurityKeypadModule : public CFullScreenModule
{
public:
	CPDRitzSecurityKeypadModule();
	~CPDRitzSecurityKeypadModule();

	virtual void Dispose();
	virtual void Render();
	virtual void KeyDown(int key, int lParam);

protected:
	virtual void Initialize();

	void ResetCode();
	void Key(int key);
	signed char _enteredCode[5];
	int _keyPos;
	uint64_t _keyTimes[11];
	bool _updateTexture;

	// Input related
	virtual void BeginAction();

	bool _codeCorrect;
	int _blinkFrame;
	uint64_t _blinkFrameTime;

	CAmbientAudio _sound;
	uint64_t _soundStartTime;
};
