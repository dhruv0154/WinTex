#pragma once

#include "ModuleBase.h"
#include <list>
#include "DXButton.h"
#include "AnimBase.h"
#include "DXImageButton.h"
#include "Structs.h"
#include <cstdint>

#define EXAMINE_FLAG_VIDEO          1
#define EXAMINE_FLAG_IMAGE          2
#define EXAMINE_FLAG_TEXT           4
#define EXAMINE_FLAG_SPECIAL        8

class CInventoryModule : public CModuleBase
{
public:
	CInventoryModule();
	virtual ~CInventoryModule();

	virtual void Resize(int width, int height);

	virtual void Dispose();
	virtual void Render();

	static int ExamineItemOnResume;

	static void SetSelectedItem(int itemId) { _selectedItemId = itemId; }

protected:
	static CInventoryModule* Instance;
	virtual void Initialize();

	bool CheckButton(CDXButton* btn, float x, float y);

	static int _selectedItemId;
	static int _draggingItemId;
	static int _mouseOverItemId;
	
	int _mouseDownX;
	int _mouseDownY;
	bool _dragging;
	static uint64_t _lastItemClick;

	static CDXButton* _pBtnExamine;
	static CDXButton* _pBtnUse;
	static CDXButton* _pBtnResume;

	static void OnExamine(void* data);
	static void OnUse(void* data);
	static void OnResume(void* data);

	virtual void Examine();
	virtual void Resume();

	static ID3D11Buffer* _selectionRectangle;

	static uint8_t* _examData;
	static int _examStructSize;

	enum class ExminationFlag
	{
		Video = 1,
		Image = 2,
		Text = 4,
		Unknown = 8
	};

	#pragma pack(1)
	struct ExminationData
	{
		uint8_t ItemId;
		uint8_t AddItemId;
		uint8_t ParameterAIndex;
		uint8_t ParameterAValue;
		uint8_t AskAbout1;
		uint8_t AskAbout2;
		uint8_t Travel1;
		uint8_t Travel2;
		uint8_t File;
		uint8_t Entry;
		int32_t DescriptionOffset;
		uint8_t Flags;
		uint16_t Rate;
		uint16_t HintState;
	};
	#pragma pack(8)

	static char _examFileName[16];
	static CAnimBase* _anim;

	static CDXText _text;

	static CDXImageButton* _pBtnUp;
	static CDXImageButton* _pBtnDown;

	Rect _fullRect;
	static Rect _limitedRect;

	// Input related
	virtual void Cursor(float x, float y, bool relative);
	virtual void BeginAction();
	virtual void EndAction();
	virtual void Back();
	virtual void Next();
	virtual void Prev();

	static void ScrollUp(void* data);
	static void ScrollDown(void* data);
	static void UpdateButtons();

	static int _lineAdjustment;
	static int _lineCount;
	static int _visibleLineCount;
};