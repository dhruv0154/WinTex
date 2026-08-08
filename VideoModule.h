#pragma once

#include "ModuleBase.h"
#include "ScriptBase.h"
#include <unordered_map>
#include <string>
#include "DXListBox.h"
#include "ScriptState.h"

enum class VideoType
{
	Single = 1,
	Scripted = 2,
};

class CVideoModule : public CModuleBase
{
public:
	CVideoModule(VideoType type, int dmapIndex, int activeScript = -1);
	CVideoModule(VideoType type, const char* fileName, int itemIndex);
	virtual ~CVideoModule();

	VideoType Type;

	virtual void Resize(int width, int height);

	virtual void KeyDown(int key, int lParam);

	virtual void Dispose();
	virtual void Render();

	virtual void Resume();

protected:
	virtual void Initialize();

	CScriptBase* _scriptEngine;
	CScriptState* _scriptState;
	CDXListBox _listBox;

	static void DialogueOptionA(void* data);
	static void DialogueOptionB(void* data);
	static void DialogueOptionC(void* data);

	static void SelectOption(int option);
	void SelectDialogueOption(int option);

	// Input related
	virtual void Cursor(float x, float y, bool relative);
	virtual void BeginAction();
	virtual void Back();

	int _askAboutBase;
};
