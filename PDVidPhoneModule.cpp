#include "PDVidPhoneModule.h"
#include "Utilities.h"
#include "AnimationController.h"
#include "GameController.h"
#include "PDGame.h"
#include "ConstantBuffers.h"
#include "Shaders.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cmath>

#define VIDPHONE_SCREEN             5
#define VIDPHONE_BUTTONS            7
#define VIDPHONE_SCREEN_RESET       9

#define VIDPHONE_MODE_DEFAULT       0
#define VIDPHONE_MODE_DIALLING      1
#define VIDPHONE_MODE_IN_CALL       2

#define VIDPHONE_IMAGE_DIAL_DOWN    15
#define VIDPHONE_IMAGE_DIAL_UP      19
#define VIDPHONE_IMAGE_EXIT_DOWN    27
#define VIDPHONE_IMAGE_EXIT_UP      31
#define VIDPHONE_IMAGE_MESSAGE_DOWN 39
#define VIDPHONE_IMAGE_MESSAGE_UP   43

#define VIDPHONE_PHONE_NUMBERS      67
#define VIDPHONE_SOUNDS             68

CPDVidPhoneModule::CPDVidPhoneModule() : CFullScreenModule(ModuleType::VidPhone)
{
	_callMap[1] = 0x1b;
	_callMap[6] = 0x25;
	_callMap[8] = 0x14;
	_callMap[12] = 0xc;
	_callMap[14] = 0x15;
	_callMap[16] = 0x28;
	_callMap[17] = 0x11;

	_messages.push_back({ 12, 5 });
	_messages.push_back({ 12, 7 });
	_messages.push_back({ 27, 17 });
	_messages.push_back({ 27, 12 });
	_messages.push_back({ 27, 13 });
	_messages.push_back({ 27, 14 });
	_messages.push_back({ 27, 16 });
	_messages.push_back({ 12, 12 });
	_messages.push_back({ 20, 50 });

	_topCallIndex = 0;
	_mode = VIDPHONE_MODE_DEFAULT;
	_callerIndex = -1;

	_scriptEngine = CGameController::GetScriptEngine();

	_screenResetData = nullptr;

	_diallingIndex = -1;

	_audioStream = nullptr;
	_readyForNextTone = false;
	_nextToneTime = 0;

	_highlighted = nullptr;
}

CPDVidPhoneModule::~CPDVidPhoneModule()
{
	for (auto pb : _phonebook)
	{
		delete pb;
	}

	if (_screenResetData != nullptr)
	{
		delete[] _screenResetData;
		_screenResetData = nullptr;
	}

	if (_audioStream != nullptr)
	{
		_audioStream->Stop();
		delete _audioStream;
		_audioStream = nullptr;
	}

	Dispose();
}

void CPDVidPhoneModule::Initialize()
{
	CFullScreenModule::Initialize();

	CAnimationController::Clear();

	_pdRawFont.Init(IDR_RAWFONT_PD);
	_uakmRawFont.Init(IDR_RAWFONT_UAKM);

	DoubleData dd = LoadDoubleEntry("GRAPHICS.AP", VIDPHONE_SCREEN);
	ReadPalette(dd.File1.Data);

	delete[] dd.File1.Data;

	_screen = dd.File2.Data;
	memset(_screen + 640 * 366, 0, 640 * 114);

	int w = dx.GetWidth();
	int h = dx.GetHeight();

	_cursorPosX = static_cast<float>(w) / 2.0f;
	_cursorPosY = static_cast<float>(h) / 2.0f;

	_cursorMinX = 0;
	_cursorMaxX = w;
	_cursorMinY = 0;
	_cursorMaxY = h;

	BinaryData bd = LoadEntry("GRAPHICS.AP", VIDPHONE_BUTTONS);
	if (bd.Data != nullptr)
	{
		_data = bd.Data;
		int count = GetInt(_data, 0, 2) - 1;
		for (int i = 0; i < count; i++)
		{
			_files[i] = _data + GetInt(_data, 2 + i * 4, 4);
		}
	}

	bd = LoadEntry("GRAPHICS.AP", VIDPHONE_SCREEN_RESET);
	if (bd.Data != nullptr)
	{
		_screenResetData = bd.Data;
	}

	UpdatePhonebook();
	RenderPhonebook();

	RenderEnterPhoneNumber();

	RenderItem(VIDPHONE_IMAGE_DIAL_DOWN, 296, 325);
	RenderItem(VIDPHONE_IMAGE_MESSAGE_DOWN, 124, 325);

	UpdateTexture();

	DialogueOptions[0].SetClick(DialogueOptionA);
	DialogueOptions[1].SetClick(DialogueOptionB);
	DialogueOptions[2].SetClick(DialogueOptionC);
}

void CPDVidPhoneModule::Resize(int width, int height)
{
}

void CPDVidPhoneModule::Render()
{
	if (_mode == VIDPHONE_MODE_IN_CALL)
	{
		if (CAnimationController::Exists() && CAnimationController::IsDone() && _scriptState.WaitingForInput == false && _scriptState.ExecutionPointer == -1)
		{
			_mode = VIDPHONE_MODE_DEFAULT;
			RenderRaw(_screenResetData, 62, 42, 296, 228);

			RenderItem(VIDPHONE_IMAGE_DIAL_DOWN, 296, 325);
			RenderItem(VIDPHONE_IMAGE_EXIT_UP, 38, 325);
			RenderItem(VIDPHONE_IMAGE_MESSAGE_DOWN, 124, 325);

			_callerIndex = -1;
			for (auto pb : _phonebook)
			{
				pb->IsSelected = false;
			}

			RenderPhonebook();
			RenderEnterPhoneNumber();
			UpdateTexture();
		}
	}
	else if (_mode == VIDPHONE_MODE_DIALLING)
	{
		uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

		if (_diallingIndex == -1)
		{
			_mode = VIDPHONE_MODE_IN_CALL;
			LoadVideo(_callerIndex);
		}
		else 
		{
			// Poll the stream to emulate the old OnBufferEnd callback
			if (!_readyForNextTone && _audioStream != nullptr)
			{
				if (_audioStream->GetPendingBufferCount() == 0)
				{
					_nextToneTime = now + 100;
					_readyForNextTone = true;
				}
			}

			if (_readyForNextTone && now > _nextToneTime)
			{
				PlayNextTone();
			}
		}
	}

	if (CAnimationController::Exists())
	{
		CAnimationController::SetOutputBuffer(_screen, 640, 480, -110, -84, _palette, 0x60, 256);
		if (CAnimationController::UpdateAndRender())
		{
			UpdateTexture();
		}

		RenderScreen();

		CAnimationController::RenderCaptions(-1.0f);

		if (_scriptState.WaitingForInput && CAnimationController::IsDone())
		{
			if (DialogueOptionsCount >= 1)
			{
				DialogueOptions[0].Render();
			}
			if (DialogueOptionsCount >= 2)
			{
				DialogueOptions[1].Render();
			}
			if (DialogueOptionsCount >= 3)
			{
				DialogueOptions[2].Render();
			}

			if (_scriptState.AskAbout || _scriptState.Offer || _scriptState.Buy)
			{
				bool recreate = ((_scriptState.AskAbout && CGameController::AskAboutChanged) || (_scriptState.Offer && CGameController::ItemsChanged) || (_scriptState.Buy && CGameController::BuyChanged));

				if (_scriptState.TopItemOffset < 0 || recreate)
				{
					int valToFind = -1;

					std::vector<ListBoxItem> items;
					if (_scriptState.AskAbout && !_scriptState.AskingAboutBuyables)
					{
						int askAboutCount = CGameController::GetAskAboutCount();
						for (int i = 0; i < 256 && i < askAboutCount; i++)
						{
							ListBoxItem lbi;
							lbi.Id = CGameController::GetAskAboutId(i);
							lbi.Text = CGameController::GetAskAboutName(lbi.Id);
							items.push_back(lbi);
						}

						CGameController::AskAboutChanged = false;
						valToFind = 4;
					}
					else if (_scriptState.Offer)
					{
						int itemCount = CGameController::GetItemCount();
						for (int i = 0; i < 256 && i < itemCount; i++)
						{
							ListBoxItem lbi;
							lbi.Id = CGameController::GetItemId(i);
							lbi.Text = CGameController::GetItemName(lbi.Id);
							items.push_back(lbi);
						}

						CGameController::ItemsChanged = false;
						valToFind = 7;
					}
					else if (_scriptState.Buy || (_scriptState.AskAbout && _scriptState.AskingAboutBuyables))
					{
						_askAboutBase = 0x3f;

						int itemCount = CGameController::GetBuyableItemCount();
						for (int i = 0; i < 256 && i < itemCount; i++)
						{
							ListBoxItem lbi;
							lbi.Id = CGameController::GetBuyableItemId(i);
							lbi.Text = CGameController::GetBuyableItemName(lbi.Id);
							items.push_back(lbi);
						}

						CGameController::BuyChanged = false;
						valToFind = _scriptState.Buy ? 6 : 4;
					}

					_scriptState.TopItemOffset = 0;
					int ix = 0;
					if (DialogueOptions[1].GetValue() == valToFind)
					{
						ix = 1;
					}
					else if (DialogueOptions[2].GetValue() == valToFind)
					{
						ix = 2;
					}
					_listBox.Init(items, std::floor(DialogueOptions[ix].GetX() + DialogueOptions[ix].GetWidth() / 2.0f));
				}

				_listBox.Render();
			}

			CModuleController::Cursors[0].SetPosition(_cursorPosX, _cursorPosY);
			CModuleController::Cursors[0].Render();
		}
	}
	else
	{
		RenderScreen();
	}

	if (!CAnimationController::HasAnim() || CAnimationController::IsDone())
	{
		if (_scriptState.WaitingForMediaToFinish)
		{
			_scriptEngine->Resume(&_scriptState, true);
		}
		else if (_scriptState.ExecutionPointer == -1)
		{
			CAnimationController::Clear();
		}
	}
}

void CPDVidPhoneModule::RenderScreen()
{
	if (_vertexBuffer != NULL)
	{
		dx.DisableZBuffer();

		uint32_t stride = sizeof(TEXTURED_VERTEX);
		uint32_t offset = 0;
		dx.SetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
		dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		CShaders::SelectOrthoShader();
		float16 wm = Math::Identity();
		CConstantBuffers::SetWorld(dx, &wm);
		ID3D11ShaderResourceView* pRV = _texture.GetTextureRV();
		dx.SetShaderResources(0, 1, &pRV);
		dx.Draw(4, 0);

		CModuleController::Cursors[0].SetPosition(_cursorPosX, _cursorPosY);
		CModuleController::Cursors[0].Render();

		dx.EnableZBuffer();
	}
}

void CPDVidPhoneModule::BeginAction()
{
	if (_inputEnabled)
	{
		int x = static_cast<int>((_cursorPosX - _left) / _scale);
		int y = static_cast<int>((_cursorPosY - _top) / _scale);

		if (_mode == VIDPHONE_MODE_DEFAULT)
		{
			if (x >= 450 && x < 640 && y >= 1 && y < 142)
			{
				for (auto pb : _phonebook)
				{
					pb->IsSelected = (y >= pb->Box.Top && y < pb->Box.Bottom);
					if (pb->IsSelected)
					{
						_callerIndex = pb->Index;
					}
				}

				RenderPhonebook();

				RenderItem(_callerIndex > -1 ? VIDPHONE_IMAGE_DIAL_UP : VIDPHONE_IMAGE_DIAL_DOWN, 296, 325);

				UpdateTexture();
			}

			if (y >= 328 && y <= 365)
			{
				if (x >= 38 && x <= 120)
				{
					CModuleController::Pop(this);
				}
				else if (x >= 123 && x <= 206)
				{
					// Message
				}
				else if (x >= 209 && x <= 292)
				{
					// Hint
				}
				else if (x >= 296 && x <= 378)
				{
					if (_callerIndex >= 0)
					{
						_mode = VIDPHONE_MODE_DIALLING;

						RenderItem(VIDPHONE_IMAGE_DIAL_DOWN, 296, 325);
						RenderItem(VIDPHONE_IMAGE_EXIT_DOWN, 38, 325);
						RenderItem(VIDPHONE_IMAGE_MESSAGE_DOWN, 124, 325);
						UpdateTexture();

						_diallingIndex = _callerIndex * 20;
						_readyForNextTone = true;
					}
				}
			}
		}
		else if (_mode == VIDPHONE_MODE_IN_CALL)
		{
#ifndef DEBUG
			if (CGameController::CanCancelVideo())
#endif
			{
				CAnimationController::Skip();

				if (_scriptState.WaitingForInput && (_scriptState.AskAbout || _scriptState.Offer || _scriptState.Buy))
				{
					int hitId = _listBox.HitTestLB(x, y);
					if (hitId >= 0)
					{
						int option = _scriptState.AskAbout ? 4 : _scriptState.Offer ? 7 : _scriptState.Buy ? 6 : -1;
						CGameController::SetSelectedItem(hitId + ((option == 4) ? _askAboutBase : 0));
						_scriptState.SelectedOption = option;
						_scriptState.SelectedValue = hitId + ((option == 4) ? _askAboutBase : 0);
						_scriptEngine->Resume(&_scriptState, true);
					}
				}

				for (int i = 0; i < 3; i++)
				{
					if (DialogueOptionsCount > i && DialogueOptions[i].HitTest(x, y) != NULL)
					{
						DialogueOptions[i].Click();
						break;
					}
				}
			}
		}
	}
}

void CPDVidPhoneModule::Back()
{
	if (_mode == VIDPHONE_MODE_DEFAULT)
	{
		CModuleController::Pop(this);
	}
}

void CPDVidPhoneModule::UpdatePhonebook()
{
	int y = 1;
	int phonebookSize = CGameController::GetWord(PD_SAVE_VIDPHONE_ENTRIES_COUNT);
	for (int i = 0; i < phonebookSize; i++)
	{
		int index = CGameController::GetData(PD_SAVE_VIDPHONE_ENTRIES + i * 2);
		Phonebook* pBP = new Phonebook();
		pBP->Index = index;
		_phonebook.push_back(pBP);
	}
}

void CPDVidPhoneModule::RenderPhonebook()
{
	int y = 1;
	std::unordered_map<int, int> colourMap;
	for (auto pb : _phonebook)
	{
		std::string name = CGameController::GetSituationDescriptionD(pb->Index + 1);
		colourMap[2] = pb->IsSelected ? 15 : 13;
		pb->Box = _pdRawFont.Render(_screen, 640, 480, 462, y, name.c_str(), colourMap, -1, -1, true);
		y += _pdRawFont.GetHeight();
	}
}

void CPDVidPhoneModule::Cursor(float x, float y, bool relative)
{
	CModuleBase::Cursor(x, y, relative);

	x = _cursorPosX;
	y = _cursorPosY;

	if (_scriptState.WaitingForInput)
	{
		for (int i = 0; i < 3; i++)
		{
			if (DialogueOptionsCount > i)
			{
				DialogueOptions[i].SetMouseOver(DialogueOptions[i].HitTest(x, y) != NULL);
			}
		}

		if (_scriptState.AskAbout || _scriptState.Offer || _scriptState.Buy)
		{
			_listBox.HitTestLB(x, y);
		}
	}
	else if (_mode == VIDPHONE_MODE_DEFAULT)
	{
		bool update = false;
		if (_highlighted != nullptr)
		{
			DrawRectangle(static_cast<int>(_highlighted->Box.Left - 3), static_cast<int>(_highlighted->Box.Top - 1), static_cast<int>(_highlighted->Box.Right + 3), static_cast<int>(_highlighted->Box.Bottom - 2), 0);
			update = true;
		}

		if (x >= 450 && x < 640 && y >= 1 && y < 142)
		{
			for (auto pb : _phonebook)
			{
				if (y >= pb->Box.Top && y < pb->Box.Bottom)
				{
					DrawRectangle(static_cast<int>(pb->Box.Left - 3), static_cast<int>(pb->Box.Top - 1), static_cast<int>(pb->Box.Right + 3), static_cast<int>(pb->Box.Bottom - 2), 15);
					_highlighted = pb;
					update = true;
				}
			}
		}

		if (update)
		{
			UpdateTexture();
		}
	}
}

void CPDVidPhoneModule::DialogueOptionA(void* data)
{
	SelectOption(DialogueOptions[0].GetValue());
}

void CPDVidPhoneModule::DialogueOptionB(void* data)
{
	SelectOption(DialogueOptions[1].GetValue());
}

void CPDVidPhoneModule::DialogueOptionC(void* data)
{
	SelectOption(DialogueOptions[2].GetValue());
}

void CPDVidPhoneModule::SelectOption(int option)
{
	if (CModuleController::CurrentModule != NULL && CModuleController::CurrentModule->Type == ModuleType::VidPhone)
	{
		((CPDVidPhoneModule*)CModuleController::CurrentModule)->SelectDialogueOption(option);
	}
}

void CPDVidPhoneModule::SelectDialogueOption(int option)
{
	_scriptState.TopItemOffset = -1;
	_scriptEngine->SelectDialogueOption(&_scriptState, option);
}

void CPDVidPhoneModule::LoadVideo(int caller)
{
	int dmapIndex = _callMap[caller];

	_scriptEngine->_mapEntry = CModuleController::pDMap->Get(dmapIndex);
	std::string fileName = CGameController::GetFileName(_scriptEngine->_mapEntry->ScriptFileIndex);
	if (fileName != "")
	{
		BinaryData bd = LoadEntry(fileName.c_str(), _scriptEngine->_mapEntry->ScriptFileEntry);
		_scriptState.Init(bd.Data, bd.Length, fileName.c_str(), _scriptEngine->_mapEntry->ScriptFileEntry);

		_scriptState.ExecutionPointer = _scriptState.GetScript(50); 

		if (_scriptState.ExecutionPointer < 0)
		{
			_scriptState.ExecutionPointer = 0;
		}

		_scriptEngine->Resume(&_scriptState);
	}

	_askAboutBase = 0;
}

void CPDVidPhoneModule::RenderEnterPhoneNumber()
{
	Fill(135, 143, 284, 170, 0);
	DrawRectangle(135, 143, 284, 170, 13);

	std::unordered_map<int, int> colourMap;
	colourMap[2] = 9;
	colourMap[3] = 8;
	_uakmRawFont.Render(_screen, 640, 460, 144, 144, "Enter phone number", colourMap, -1);
	_uakmRawFont.Render(_screen, 640, 460, 174, 157, "now please", colourMap, -1);
}

void CPDVidPhoneModule::PlayNextTone()
{
	if (_diallingIndex >= 0)
	{
		uint8_t* pNumbers = _files[VIDPHONE_PHONE_NUMBERS];
		if (pNumbers != nullptr)
		{
			while (pNumbers[_diallingIndex] != '.')
			{
				char n = pNumbers[_diallingIndex++];
				if (n >= '0' && n <= '9')
				{
					uint8_t* wave = _files[VIDPHONE_SOUNDS + n - '0'];
					if (wave != nullptr)
					{
						if (_audioStream == nullptr)
						{
							AudioFormat formatBuff;
							formatBuff.channels = GetInt(wave, 0x16, 2);
							formatBuff.samplesPerSec = GetInt(wave, 0x18, 4);
							formatBuff.bitsPerSample = GetInt(wave, 0x22, 2);
							
							_audioStream = CDXSound::CreateAudioStream(formatBuff);
						}

						if (_audioStream != nullptr)
						{
							uint32_t audioBytes = GetInt(wave, 0x28, 4);
							const uint8_t* audioData = wave + 0x28;
							
							_audioStream->SubmitBuffer(audioData, audioBytes);
							_audioStream->Start();

							_readyForNextTone = false;
						}
					}
					break;
				}
			}

			if (pNumbers[_diallingIndex] == '.')
			{
				_diallingIndex = -1;
			}
		}
	}
}