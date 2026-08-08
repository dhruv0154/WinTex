#pragma once

#include "FullScreenModule.h"
#include <unordered_map>
#include "Texture.h"
#include "AmbientAudio.h"
#include <cstdint>

class CUAKMCodePanelModule : public CFullScreenModule
{
public:
	CUAKMCodePanelModule(int parameter);
	virtual ~CUAKMCodePanelModule();

	virtual void Render();
	virtual void KeyDown(int key, int lParam);

protected:
	virtual void Initialize();

	int _parameter;

	void RenderDot(int pos);

	void ResetCode();
	void Key(int key);
	signed char _enteredCode[8];
	int _keyPos;

	CAmbientAudio _sound;

	uint64_t _passwordMessageTime;
	int _lastMessageOffset;

	int _wrongFrame;
	uint64_t _wrongFrameTime;

	int _correctFrame;
	uint64_t _correctFrameTime;

	void Render(int entry, int offset_x, int offset_y);

	// Input related
	virtual void BeginAction();
};