#pragma once

#include "MainMenuModule.h"

class CPDMainMenuModule : public CMainMenuModule
{
public:
	CPDMainMenuModule();
	virtual ~CPDMainMenuModule();

	virtual void Render();

protected:
	virtual void SetupScreen();
	virtual void SetupSave();
	virtual void SetupLoad();

	virtual void SetupConfigFrame();
	virtual void SetupLoadFrame();
	virtual void SetupSaveFrame();

	static void Intro(void* data);
	static void Credits(void* data);
};