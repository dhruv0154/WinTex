#pragma once

#include "ModuleBase.h"
#include "Image.h"
#include "FullScreenModule.h"
#include "RawFont.h"
#include <unordered_map>
#include <list>
#include <cstdint>
#include "ScriptBase.h"
#include "DXListBox.h"
#include "PDScriptState.h"
#include "Structs.h"
#include "DXSound.h"

class CPDVidPhoneModule : public CFullScreenModule
{
public:
	CPDVidPhoneModule();
	~CPDVidPhoneModule();

	virtual void Resize(int width, int height);
	virtual void Render();

protected:
	virtual void Initialize();

	CRawFont _pdRawFont;
	CRawFont _uakmRawFont;

	// Input related
	virtual void Cursor(float x, float y, bool relative);
	virtual void BeginAction();
	virtual void Back();

	void UpdatePhonebook();
	void RenderPhonebook();
	int _topCallIndex;
	std::unordered_map<int, int> _callMap;
	void RenderEnterPhoneNumber();

	struct PDVidPhoneMessage
	{
		int DMapEntry;
		int DMapScript;
	};

	std::list<PDVidPhoneMessage> _messages;

	class Phonebook
	{
	public:
		Phonebook() { Index = 0; IsSelected = false; Box.Top = 0; Box.Left = 0; Box.Bottom = 0; Box.Right = 0; }
		int Index;
		Rect Box;
		bool IsSelected;
	};

	std::list<Phonebook*> _phonebook;
	int _mode;
	int _callerIndex;
	int _diallingIndex;

	void LoadVideo(int caller);
	void RenderScreen();
	uint8_t* _screenResetData;

	CScriptBase* _scriptEngine;
	CPDScriptState _scriptState;
	CDXListBox _listBox;

	int _askAboutBase;

	static void DialogueOptionA(void* data);
	static void DialogueOptionB(void* data);
	static void DialogueOptionC(void* data);

	static void SelectOption(int option);
	void SelectDialogueOption(int option);

	CAudioStream* _audioStream;
	void PlayNextTone();
	bool _readyForNextTone;
	uint64_t _nextToneTime;

	Phonebook* _highlighted;

	bool _flashMessages;
	bool _flashDial;
};