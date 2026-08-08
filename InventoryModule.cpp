#include "InventoryModule.h"
#include "Globals.h"
#include "GameController.h"
#include "Items.h"
#include "UAKMNewsPaperModule.h"
#include "Utilities.h"
#include "AnimationController.h"
#include "UAKMGame.h"
#include "UAKMEncodedMessageModule.h"
#include "UAKMTornNoteModule.h"
#include "LocationModule.h"
#include "ConstantBuffers.h"
#include "Shaders.h"
#include <cmath>
#include <chrono>
#include <algorithm>
#include <cstring>

#define INVENTORY_WIDTH 108.0f
#define INVENTORY_HEIGHT 80.0f

#define ITEM_NOTE_SCRAPS            33
#define ITEM_SHREDDED_NOTE          57
#define ITEM_NEWSPAPER              94
#define ITEM_ENCODED_NOTE           95
#define ITEM_NOTE_SCRAPS_2          139

CInventoryModule* CInventoryModule::Instance = NULL;

int CInventoryModule::_selectedItemId = -1;
int CInventoryModule::_draggingItemId = -1;
int CInventoryModule::_mouseOverItemId = -1;
uint64_t CInventoryModule::_lastItemClick = 0;

CDXButton* CInventoryModule::_pBtnExamine = NULL;
CDXButton* CInventoryModule::_pBtnUse = NULL;
CDXButton* CInventoryModule::_pBtnResume = NULL;

CDXText CInventoryModule::_text;

CDXImageButton* CInventoryModule::_pBtnUp = NULL;
CDXImageButton* CInventoryModule::_pBtnDown = NULL;

ID3D11Buffer* CInventoryModule::_selectionRectangle = NULL;

uint8_t* CInventoryModule::_examData = NULL;
int CInventoryModule::_examStructSize = 0;
CAnimBase* CInventoryModule::_anim = NULL;

char CInventoryModule::_examFileName[16];

int CInventoryModule::ExamineItemOnResume = -1;

int CInventoryModule::_lineAdjustment = 0;
int CInventoryModule::_lineCount = 0;
int CInventoryModule::_visibleLineCount = 0;

Rect CInventoryModule::_limitedRect;

CInventoryModule::CInventoryModule() : CModuleBase(ModuleType::Inventory)
{
	_selectedItemId = -1;
	_draggingItemId = -1;
	_mouseOverItemId = -1;
	_dragging = false;
	_mouseDownX = -1;
	_mouseDownY = -1;
	ExamineItemOnResume = -1;

	_text.SetColours(0xff000000, 0xff00c300, 0xff24ff00, 0xff000000);
	
	_fullRect.Top = 0;
	_fullRect.Left = 0;
	_fullRect.Bottom = 0;
	_fullRect.Right = 0;

	Instance = this;
}

CInventoryModule::~CInventoryModule()
{
	if (_anim != NULL)
	{
		delete _anim;
		_anim = NULL;
	}

	if (_pBtnExamine != NULL)
	{
		delete _pBtnExamine;
		_pBtnExamine = NULL;
	}

	if (_pBtnUse != NULL)
	{
		delete _pBtnUse;
		_pBtnUse = NULL;
	}

	if (_pBtnResume != NULL)
	{
		delete _pBtnResume;
		_pBtnResume = NULL;
	}

	if (_pBtnUp != NULL)
	{
		delete _pBtnUp;
		_pBtnUp = NULL;
	}

	if (_pBtnDown != NULL)
	{
		delete _pBtnDown;
		_pBtnDown = NULL;
	}

	CModuleController::Cursors[(int)CAnimatedCursor::CursorType::Crosshair].SetPosition(dx.GetWidth() / 2.0f, dx.GetHeight() / 2.0f);
}

void CInventoryModule::Initialize()
{
	_cursorPosX = dx.GetWidth() / 2.0f;
	_cursorPosY = dx.GetHeight() / 2.0f;

	CAnimationController::Clear();

	if (_pBtnExamine == NULL)
	{
		const char* pExam = "Examine";
		const char* pRes = "Resume";
		const char* pUse = "Use";
		float maxw = std::max({ TexFont.PixelWidth(pExam), TexFont.PixelWidth(pRes), TexFont.PixelWidth(pUse) });

		_pBtnExamine = new CDXButton(pExam, maxw, 32.0f * pConfig->FontScale, OnExamine);
		_pBtnUse = new CDXButton(pUse, maxw, 32.0f * pConfig->FontScale, OnUse);
		_pBtnResume = new CDXButton(pRes, maxw, 32.0f * pConfig->FontScale, OnResume);

		_pBtnExamine->SetPosition(0, dx.GetHeight() - 40 * pConfig->FontScale);
		_pBtnUse->SetPosition((dx.GetWidth() - _pBtnUse->GetWidth()) / 2, dx.GetHeight() - 40 * pConfig->FontScale);
		_pBtnResume->SetPosition(dx.GetWidth() - _pBtnResume->GetWidth(), dx.GetHeight() - 40 * pConfig->FontScale);

		_fullRect.Top = 0;
		_fullRect.Left = 0;
		_fullRect.Bottom = dx.GetHeight();
		_fullRect.Right = dx.GetWidth();

		_limitedRect.Top = static_cast<int>(3.0f * dx.GetHeight() / 4.0f);
		_limitedRect.Left = 0;
		_limitedRect.Bottom = static_cast<int>(dx.GetHeight() - 10.0f);
		_limitedRect.Right = static_cast<int>(dx.GetWidth() - 40 - _pBtnResume->GetWidth());

		_visibleLineCount = static_cast<int>((_limitedRect.Bottom - _limitedRect.Top) / (TexFont.Height() * pConfig->FontScale));

		_pBtnUp = new CDXImageButton(2, ScrollUp);
		_pBtnUp->SetPosition(static_cast<float>(_limitedRect.Right), static_cast<float>(_limitedRect.Top));
		_pBtnDown = new CDXImageButton(3, ScrollDown);
		_pBtnDown->SetPosition(static_cast<float>(_limitedRect.Right), _limitedRect.Bottom - _pBtnDown->GetHeight() - 10.0f);

		COLOURED_VERTEX_ORTHO* pVB = new COLOURED_VERTEX_ORTHO[5];
		if (pVB != NULL)
		{
			float x1 = 1.0f;
			float x2 = x1 + INVENTORY_WIDTH - 2.0f;
			float y1 = 0.5f;
			float y2 = y1 - INVENTORY_HEIGHT - 2.0f;

			pVB[0].position = float4(x1, y1, 0.0f, 0.0f);
			pVB[0].colour = float4(1.0f, 1.0f, 1.0f, 1.0f);
			pVB[1].position = float4(x2, y1, 0.0f, 0.0f);
			pVB[1].colour = float4(1.0f, 1.0f, 1.0f, 1.0f);
			pVB[2].position = float4(x2, y2 - 0.3f, 0.0f, 0.0f); 
			pVB[2].colour = float4(1.0f, 1.0f, 1.0f, 1.0f);
			pVB[3].position = float4(x1, y2, 0.0f, 0.0f);
			pVB[3].colour = float4(1.0f, 1.0f, 1.0f, 1.0f);
			pVB[4].position = float4(x1, y1, 0.0f, 0.0f);
			pVB[4].colour = float4(1.0f, 1.0f, 1.0f, 1.0f);

			D3D11_BUFFER_DESC vbDesc;
			vbDesc.Usage = D3D11_USAGE_DYNAMIC;
			vbDesc.ByteWidth = sizeof(COLOURED_VERTEX_ORTHO) * 5;
			vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			vbDesc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA vData;
			vData.pSysMem = pVB;
			vData.SysMemPitch = 0;
			vData.SysMemSlicePitch = 0;

			dx.CreateBuffer(&vbDesc, &vData, &_selectionRectangle, "InventorySelection");

			delete[] pVB;
		}

		_examData = CLZ::Decompress("EXAM.LZ").Data;
		_examStructSize = *(int*)_examData;

		strcpy(_examFileName, "EXAM000.AP");
	}

	if (_selectedItemId >= 0)
	{
		_pBtnExamine->Click();
	}
}

void CInventoryModule::Dispose()
{
}

void CInventoryModule::Render()
{
	if (ExamineItemOnResume >= 0)
	{
		_selectedItemId = ExamineItemOnResume;
		ExamineItemOnResume = -1;
		_pBtnExamine->Click();
		return;
	}

	dx.DisableZBuffer();
	CConstantBuffers::Setup2D(dx);

	if (_anim == NULL)
	{
		int itemCount = CGameController::GetItemCount();

		int w = dx.GetWidth();
		int h = dx.GetHeight();

		float inventoryWidth = INVENTORY_WIDTH;
		float inventoryHeight = INVENTORY_HEIGHT;
		int numberOfItemsPerRow = static_cast<int>(w / inventoryWidth);
		float margin = w - numberOfItemsPerRow * inventoryWidth;
		float ox = margin / 2;
		float oy = -inventoryHeight + 10.0f;

		int currentSelection = _mouseOverItemId;
		_mouseOverItemId = -1;

		for (int i = 0; i < itemCount; i++)
		{
			if ((i % numberOfItemsPerRow) == 0)
			{
				ox = margin / 2;
				oy += inventoryHeight;
			}

			int id = CGameController::GetItemId(i);
			bool mouseOver = (_cursorPosX >= (ox - 5) && _cursorPosX < (ox + inventoryWidth - 5) && _cursorPosY >= (oy - 5) && _cursorPosY < (oy + inventoryHeight - 3));
			if (mouseOver)
			{
				_mouseOverItemId = id;
				if (currentSelection != id)
				{
					_lastItemClick = 0;
				}
			}

			if ((oy + inventoryHeight) <= h)
			{
				if (_selectedItemId != id || !_dragging || (_mouseDownX == static_cast<int>(_cursorPosX) && _mouseDownY == static_cast<int>(_cursorPosY)))
				{
					CItems::RenderItemImage(id, ox, oy, mouseOver);
				}

				if (_selectedItemId == id && !_dragging)
				{
					unsigned int stride = sizeof(COLOURED_VERTEX_ORTHO);
					unsigned int offset = 0;
					dx.SetVertexBuffers(0, 1, &_selectionRectangle, &stride, &offset);
					CShaders::SelectColourShader();
					dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

					float16 wm = Math::Translation(ox - 5, 5 - oy, 0.0f);
					CConstantBuffers::SetWorld(dx, &wm);
					dx.Draw(5, 0);
				}
			}

			ox += inventoryWidth;
		}

		_pBtnExamine->Render();
		_pBtnUse->Render();

		if (_mouseOverItemId >= 0 && !_dragging)
		{
			int namePixels = CItems::GetItemNameWidth(_mouseOverItemId);
			float namex = std::min(std::max(0.0f, _cursorPosX - namePixels / 2.0f), w - (float)namePixels);
			CItems::RenderItemName(_mouseOverItemId, namex - 10.0f, _cursorPosY + 16.0f, true);
		}
	}
	else
	{
		CAnimationController::UpdateAndRender(_anim);
	}

	if (_anim != NULL)
	{
		D3D11_RECT d3dRect;
		d3dRect.left = _limitedRect.Left;
		d3dRect.top = _limitedRect.Top;
		d3dRect.right = _limitedRect.Right;
		d3dRect.bottom = _limitedRect.Bottom;
		dx.SetScissorRect(d3dRect);
		
		_text.Render(10.0f, dx.GetHeight() - _text.Height() - 10.0f + _lineAdjustment * TexFont.Height() * pConfig->FontScale);
		
		d3dRect.left = _fullRect.Left;
		d3dRect.top = _fullRect.Top;
		d3dRect.right = _fullRect.Right;
		d3dRect.bottom = _fullRect.Bottom;
		dx.SetScissorRect(d3dRect);

		if (_lineCount > _visibleLineCount)
		{
			_pBtnUp->Render();
			_pBtnDown->Render();
		}
	}

	_pBtnResume->Render();

	if (_dragging && (_mouseDownX != static_cast<int>(_cursorPosX) || _mouseDownY != static_cast<int>(_cursorPosY)))
	{
		CModuleController::Cursors[(int)CAnimatedCursor::CursorType::Crosshair].SetPosition(_cursorPosX, _cursorPosY);
		CModuleController::Cursors[(int)CAnimatedCursor::CursorType::Crosshair].Render();
		CItems::RenderItemImage(_selectedItemId, _cursorPosX, _cursorPosY, false);
	}
	else
	{
		CModuleController::Cursors[(int)CAnimatedCursor::CursorType::Arrow].SetPosition(_cursorPosX, _cursorPosY);
		CModuleController::Cursors[(int)CAnimatedCursor::CursorType::Arrow].Render();
	}

	dx.EnableZBuffer();
}

bool CInventoryModule::CheckButton(CDXButton* btn, float x, float y)
{
	if (btn->HitTest(x, y))
	{
		btn->Click();
		return true;
	}
	return false;
}

short comboTable[] = { 0,5,26,1,4,0,1,5,129,4,129,26,7,37,39,9,12,2,10,77,78,14,15,49,17,35,138,18,112,114,27,28,52,42,43,85,45,46,88,45,48,87,46,87,107,48,88,107,58,59,90,60,90,93,61,70,102,65,66,108,66,73,110,66,96,132,73,131,96,79,80,76,91,103,95,110,131,132,111,114,124 };
short comboScoreTable[] = { 8,85,14,76,6,108,10,93 };
short comboHintStateScoreTable[] = { 0,11,2,75,26,12,39,108,49,135,52,145,76,234,78,227,85,263,88,342,90,373,93,374,95,189,102,418,107,343,108,469,110,453,114,379,124,380,132,454,138,209,255,-1 };

void CInventoryModule::OnExamine(void* data)
{
	Instance->Examine();
}

void CInventoryModule::OnUse(void* data)
{
	CGameController::SetCurrentItemId(_selectedItemId);
	CLocationModule::CurrentAction = ActionType::Use;
	OnResume(NULL);
}

void CInventoryModule::OnResume(void* data)
{
	Instance->Resume();
}

void CInventoryModule::Resume()
{
	if (_anim == NULL)
	{
		CModuleController::Pop(CModuleController::CurrentModule);
	}
	else
	{
		delete _anim;
		_anim = NULL;

		ExminationData* pExam = (ExminationData*)(_examData + 4 + _selectedItemId * _examStructSize);
		if (pExam->ItemId != _selectedItemId)
		{
			CGameController::SetItemState(_selectedItemId, 2);
			CGameController::SetItemState(pExam->ItemId, 1);
		}
		if (pExam->AddItemId != 0xff)
		{
			CGameController::SetItemState(pExam->AddItemId, 1);
		}
		if (pExam->AskAbout1 != 0xff)
		{
			CGameController::SetAskAboutState(pExam->AskAbout1, 1);
		}
		if (pExam->AskAbout2 != 0xff)
		{
			CGameController::SetAskAboutState(pExam->AskAbout2, 1);
		}
		if (pExam->ParameterAIndex != 0xff)
		{
			CGameController::SetParameter(pExam->ParameterAIndex, pExam->ParameterAValue);
		}
		if (pExam->Travel1 != 0xff)
		{
			CGameController::SetData(UAKM_SAVE_TRAVEL + pExam->Travel1, 1);
		}
		if (pExam->Travel2 != 0xff)
		{
			CGameController::SetData(UAKM_SAVE_TRAVEL + pExam->Travel2, 1);
		}
		if (pExam->HintState != 0xffff)
		{
			CGameController::SetHintState(pExam->HintState, 1, 1);
		}
	}
}

void CInventoryModule::Resize(int width, int height)
{
}

void CInventoryModule::Cursor(float x, float y, bool relative)
{
	CModuleBase::Cursor(x, y, relative);

	_pBtnExamine->SetMouseOver(_pBtnExamine->HitTest(x, y) != NULL);
	_pBtnUse->SetMouseOver(_pBtnUse->HitTest(x, y) != NULL);
	_pBtnResume->SetMouseOver(_pBtnResume->HitTest(x, y) != NULL);
	_pBtnUp->SetMouseOver(_pBtnUp->HitTest(x, y) != NULL);
	_pBtnDown->SetMouseOver(_pBtnDown->HitTest(x, y) != NULL);
}

void CInventoryModule::BeginAction()
{
	float x = _cursorPosX;
	float y = _cursorPosY;

	if (_anim == NULL)
	{
		if (CheckButton(_pBtnExamine, x, y) || CheckButton(_pBtnUse, x, y) || CheckButton(_pBtnResume, x, y));
		else
		{
			if (_mouseOverItemId >= 0)
			{
				_selectedItemId = _mouseOverItemId;
				_mouseDownX = static_cast<int>(_cursorPosX);
				_mouseDownY = static_cast<int>(_cursorPosY);
				_dragging = true;

				uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
				if ((now - _lastItemClick) < 500)
				{
					_dragging = false;
					_pBtnExamine->Click();
				}

				_lastItemClick = now;
			}
		}
	}
	else
	{
		if (CheckButton(_pBtnResume, x, y) || CheckButton(_pBtnUp, x, y) || CheckButton(_pBtnDown, x, y)) {
			return;
		};
	}
}

void CInventoryModule::EndAction()
{
	if (_dragging)
	{
		if (_selectedItemId != _mouseOverItemId)
		{
			for (int c = 0; c < 27; c++)
			{
				if ((comboTable[c * 3] == _selectedItemId && comboTable[c * 3 + 1] == _mouseOverItemId) || (comboTable[c * 3] == _mouseOverItemId && comboTable[c * 3 + 1] == _selectedItemId))
				{
					int newItemId = comboTable[c * 3 + 2];
					CGameController::SetItemState(_selectedItemId, 2);
					CGameController::SetItemState(_mouseOverItemId, 2);
					CGameController::SetItemState(newItemId, 1);

					for (int s = 0; s < 4; s++)
					{
						if (comboScoreTable[s * 2 + 1] == newItemId)
						{
							CGameController::AddScore(comboScoreTable[s * 2]);
							break;
						}
					}

					int i = 0;
					while (comboHintStateScoreTable[i * 2] != 0xff)
					{
						if (comboHintStateScoreTable[i * 2] == newItemId)
						{
							CGameController::SetHintState(comboHintStateScoreTable[i * 2 + 1], 1, 1);
							if (newItemId == 95)
							{
								CGameController::SetHintState(398, 1, 0);
							}
							break;
						}
						i++;
					}

					_selectedItemId = newItemId;

					break;
				}
			}
		}

		_dragging = false;
	}
}

void CInventoryModule::Back()
{
	OnResume(NULL);
}

void CInventoryModule::ScrollUp(void* data)
{
	_lineAdjustment = std::max(0, std::min(_lineCount - _visibleLineCount, _lineAdjustment + 1));
	UpdateButtons();
}

void CInventoryModule::ScrollDown(void* data)
{
	_lineAdjustment = std::max(0, _lineAdjustment - 1);
	UpdateButtons();
}

void CInventoryModule::UpdateButtons()
{
	bool tooManyLines = ((_visibleLineCount - _lineCount) < 0);
	_pBtnUp->SetEnabled(tooManyLines && _lineAdjustment != (_lineCount - _visibleLineCount));
	if (!_pBtnUp->GetEnabled())
	{
		_pBtnUp->SetMouseOver(false);
	}

	_pBtnDown->SetEnabled(tooManyLines && _lineAdjustment > 0);
	if (!_pBtnDown->GetEnabled())
	{
		_pBtnDown->SetMouseOver(false);
	}
}

void CInventoryModule::Next()
{
	ScrollDown(NULL);
}

void CInventoryModule::Prev()
{
	ScrollUp(NULL);
}

void CInventoryModule::Examine()
{
	if (_selectedItemId >= 0)
	{
		_anim = NULL;

		ExminationData* pExam = (ExminationData*)(_examData + 4 + _selectedItemId * _examStructSize);

		_examFileName[5] = (char)('0' + (pExam->File >> 4));

		char* pDesc = (char*)(pExam->DescriptionOffset + _examData);
		pAddCaptions->clear();

		std::list<CCaption*>* pOld = pDisplayCaptions;
		pDisplayCaptions = pAddCaptions;
		pAddCaptions = pOld;
		ClearCaptions(pOld);
		Rect rect;
		rect.Top = 0;
		rect.Left = 0;
		rect.Bottom = 1000;
		rect.Right = _limitedRect.Right - _limitedRect.Left;
		
		_text.SetText(pDesc, rect);
		_lineCount = _text.Lines();
		_lineAdjustment = std::max(0, std::min(_lineCount - _visibleLineCount, _lineCount));
		UpdateButtons();

		if ((pExam->Flags & EXAMINE_FLAG_VIDEO) != 0)
		{
			BinaryData bd = LoadEntry(_examFileName, pExam->Entry);
			_anim = CAnimationController::Load(bd, 2);
		}
		else if ((pExam->Flags & EXAMINE_FLAG_IMAGE) != 0)
		{
			DoubleData dd = LoadDoubleEntry(_examFileName, pExam->Entry);
			_anim = CAnimationController::LoadImage(dd, 320, 240, 2);
		}
		else if (pExam->Flags == 0)
		{
			if (_selectedItemId == ITEM_NOTE_SCRAPS || _selectedItemId == ITEM_NOTE_SCRAPS_2 || _selectedItemId == ITEM_SHREDDED_NOTE)
			{
				CModuleController::Push(new CUAKMTornNoteModule(_selectedItemId));
			}
			else if (_selectedItemId == ITEM_NEWSPAPER)
			{
				CModuleController::Push(new CUAKMNewsPaperModule());
			}
			else if (_selectedItemId == ITEM_ENCODED_NOTE)
			{
				CModuleController::Push(new CUAKMEncodedMessageModule());
			}
		}

		int extraScore = 0;
		if (_selectedItemId == 55 || _selectedItemId == 76 || _selectedItemId == 94 || _selectedItemId == 106 || _selectedItemId == 134)
		{
			extraScore = 4;
		}
		else if (_selectedItemId == 85)
		{
			extraScore = 14;
		}

		CGameController::SetItemExamined(_selectedItemId, extraScore);
	}
}