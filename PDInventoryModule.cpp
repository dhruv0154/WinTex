#include "PDInventoryModule.h"
#include "GameController.h"
#include "Items.h"
#include "Utilities.h"
#include "AnimationController.h"
#include "PDGame.h"
#include "PDExamStruct.h"
#include "PDCrosswordModule.h"
#include "PDNewspaperModule.h"
#include <algorithm>
#include <string>

short pdExamineScoreTable[] = { 26, 2, 159, 10, 12, 2, 212, 2, 10, 2, 266, 5, 63, 5, 230, 2, 55, 2, 27, 2, 246, 2, 211, 5, 39, 10, 53, 2, 265, 5, 17, 20, 5, 2, 262, 2, 225, 5, 43, 50, -1 };

const char* hex = "0123456789ABCDEF";

CPDInventoryModule::CPDInventoryModule() : CInventoryModule()
{
    _text.SetColours(0xff000000, 0xff000000, 0xff00ff00, 0xff000000);
    CItems::SetTextColours(0, 0, -1, 0);

    // Update cash item description string
    CItems::SetItemName(0, CGameController::GetItemName(0) + " $" + std::to_string(CGameController::GetWord(PD_SAVE_CASH)));

    _closeOnResume = true;
}

void CPDInventoryModule::Examine()
{
    if (_selectedItemId >= 0)
    {
        _closeOnResume = false;
        _anim = NULL;

        PDExamStruct* pExam = (PDExamStruct*)(_examData + 8 + _selectedItemId * sizeof(PDExamStruct));

        _examFileName[4] = hex[(pExam->ExamFileNumber >> 4) & 0xf];
        _examFileName[5] = hex[(pExam->ExamFileNumber) & 0xf];

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

        if (_selectedItemId == PD_INVENTORY_CROSSWORD_PUZZLE)
        {
            CModuleController::Push(new CPDCrosswordModule());
        }
        else if (_selectedItemId == PD_INVENTORY_OLD_NEWSPAPER)
        {
            CModuleController::Push(new CPDNewspaperModule());
        }
        else if ((pExam->Flags & EXAMINE_FLAG_VIDEO) != 0)
        {
            BinaryData bd = LoadEntry(_examFileName, pExam->ExamEntryNumber);
            _anim = CAnimationController::Load(bd, 2);
        }
        else if ((pExam->Flags & EXAMINE_FLAG_IMAGE) != 0)
        {
            DoubleData dd = LoadDoubleEntry(_examFileName, pExam->ExamEntryNumber);
            _anim = CAnimationController::LoadImage(dd, 320, 240, 2);
        }
        else if (pExam->Flags == 0)
        {

        }

        int extraScore = 0;
        int scan = 0;
        while (pdExamineScoreTable[scan * 2] != -1)
        {
            if (pdExamineScoreTable[scan * 2] == _selectedItemId)
            {
                extraScore += pdExamineScoreTable[scan * 2 + 1];
            }
            scan++;
        }

        CGameController::SetItemExamined(_selectedItemId, extraScore);
    }
}

void CPDInventoryModule::Resume()
{
    if (_closeOnResume)
    {
        CModuleController::Pop(CModuleController::CurrentModule);
    }
    else
    {
        _closeOnResume = true;

        if (_anim != NULL)
        {
            delete _anim;
            _anim = NULL;
        }

        PDExamStruct* pExam = (PDExamStruct*)(_examData + 8 + _selectedItemId * sizeof(PDExamStruct));
        if (pExam->Id != _selectedItemId)
        {
            // Transform item (remove old, add new)
            CGameController::SetItemState(_selectedItemId, 2);
            CGameController::SetItemState(pExam->Id, 1);
        }
        if (pExam->AddItemId != -1)
        {
            CGameController::SetItemState(pExam->AddItemId, 1);
        }
        if (pExam->AskAbout1 != 0xff)
        {
            CGameController::SetItemState(PD_SAVE_ASK_ABOUT_BASE, pExam->AskAbout1, 1);
        }
        if (pExam->AskAbout2 != 0xff)
        {
            CGameController::SetItemState(PD_SAVE_ASK_ABOUT_BASE, pExam->AskAbout2, 1);
        }
        if (pExam->ParameterAIndex != -1)
        {
            CGameController::SetParameter(pExam->ParameterAIndex, pExam->ParameterAValue);
        }
        if (pExam->Travel1 != 0xff)
        {
            CGameController::SetData(PD_SAVE_TRAVEL + pExam->Travel1, 1);
        }
        if (pExam->Travel2 != 0xff)
        {
            CGameController::SetData(PD_SAVE_TRAVEL + pExam->Travel2, 1);
        }
        if (pExam->HintState != -1)
        {
            CGameController::SetHintState(pExam->HintState, 1, 1);
        }
    }
}