#pragma once

#include "AnimBase.h"

class CVideoPlayer : public CAnimBase
{
public:
	CVideoPlayer();
	~CVideoPlayer();

	void Init(void* hWnd, const char* fileName);
	virtual bool Update();

	virtual bool ShouldClearDXBuffer() { return false; }
	virtual void Skip();
};
