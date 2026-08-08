#include "PictureModule.h"
#include "Globals.h"
#include "Utilities.h"
#include "GameController.h"
#include "AnimationController.h"
#include <string>

CPictureModule::CPictureModule(int fileId, int entryIndex, CScriptBase* pScript, CScriptState* pScriptState) : CFullScreenModule(ModuleType::Picture)
{
	_script = pScript;
	_state = pScriptState;
	_image = NULL;
	_rendered = false;

	std::string fn = CGameController::GetFileName(fileId);
	if (fn != "")
	{
		BinaryData bd = LoadEntry(fn.c_str(), entryIndex);
		if (bd.Data != NULL)
		{
			if (bd.Length > 0x304 && bd.Data[0x300] == 'D' && bd.Data[0x301] == 'B' && bd.Data[0x302] == 'E' && bd.Data[0x303] == 1)
			{
				BinaryData imageData = CLZ::Decompress(bd.Data + 0x300, bd.Length - 0x300);

				_image = CAnimationController::LoadImage(bd.Data, imageData, 640, 480);

				delete[] imageData.Data;
			}

			delete[] bd.Data;
		}
	}
}

CPictureModule::~CPictureModule()
{
	Dispose();
}

void CPictureModule::Render()
{
	if (_image != NULL && !_rendered)
	{
		_image->Render();

		_rendered = true;
		_state->WaitingForMediaToFinish = false;
	}
	else if (_state->WaitingForMediaToFinish)
	{
		if (CAnimationController::HasAnim() && !CAnimationController::IsDone())
		{
			CAnimationController::UpdateAndRender();
		}
		else
		{
			_state->WaitingForMediaToFinish = false;
		}
	}
	else if (_script != NULL)
	{
		_script->Resume(_state, true);
	}
}

void CPictureModule::BeginAction()
{
	CAnimationController::Skip();
}