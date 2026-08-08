#pragma once

#include "FullScreenModule.h"
#include <unordered_map>
#include "Texture.h"
#include <cstdint>

class CUAKMGRSComputerModule : public CFullScreenModule
{
public:
	CUAKMGRSComputerModule();
	virtual ~CUAKMGRSComputerModule();

	virtual void Dispose();
	virtual void Render();

	virtual void KeyDown(int key, int lParam);

protected:
	virtual void Initialize();

	uint8_t* _animation;
	int _animationLength;

	void RenderButton(int x, int y, int image);

	uint8_t* _animationPointer;
	int _animationFrames;
	int _animationWidth;
	int _animationHeight;
	bool _animationActive;

	int _previousPage;

	// Input related
	virtual void BeginAction();
	virtual void Back();
};