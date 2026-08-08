#pragma once

#include "GameBase.h"
#include "ScriptBase.h"
#include <unordered_map>
#include <string>
#include <cstdint>

class CGameController
{
public:
    static void Init();
    static bool StartGame(CGameBase* pGame);

    static int GetScore() { return Game ? Game->GetScore() : 0; }
    static void AddScore(int val) { if (Game) Game->AddScore(val); }
    static uint8_t GetParameter(int index);
    static void SetParameter(int index, uint8_t value);
    static void SetHintState(int index, uint8_t state, int score);
    static uint8_t GetHintState(int index);
    static void SetHintCategoryState(int index, uint8_t state);
    static uint8_t GetHintCategoryState(int index);

    static void SetTimer(int timer, int duration);
    static int GetTimerState(int timer);
    static void ResetTimers();

    static CScriptBase* GetScriptEngine() { return Game ? Game->GetScriptEngine() : nullptr; }
    static CScriptState* GetScriptState() { return Game ? Game->GetScriptState() : nullptr; }

    static void SetFileName(int index, const std::string& name);
    static std::string GetFileName(int index);
    static void SetItemName(int index, const std::string& name);
    static std::string GetItemName(int index);
    static void SetAskAboutName(int index, const std::string& name);
    static std::string GetAskAboutName(int index);

    static void SetSituationDescriptionL(int ix, const std::string& value);
    static std::string GetSituationDescriptionL(int ix);
    static void SetSituationDescriptionD(int ix, const std::string& value);
    static std::string GetSituationDescriptionD(int ix);

    static int GetSaveCommentOffset() { return Game ? Game->GetSaveCommentOffset() : 0; }
    static int GetSaveCommentLength() { return Game ? Game->GetSaveCommentLength() : 0; }

    static uint8_t GetData(int offset) { return Game ? Game->GetData(offset) : 0; }
    static void SetData(int offset, uint8_t value) { if (Game) Game->SetData(offset, value); }
    static void SetData(int offset, const char* text) { if (Game) Game->SetData(offset, const_cast<char*>(text)); }
    static uint8_t* GetDataPointer() { return Game ? Game->GetDataPointer() : nullptr; }
    static uint16_t GetWord(int offset) { return Game ? Game->GetWord(offset) : 0; }
    static void SetWord(int offset, uint16_t value) { if (Game) Game->SetWord(offset, value); }
    static void Copy(uint8_t* source, int destinationOffset, int length) { if (Game) Game->Copy(source, destinationOffset, length); }

    static void TransformItem(int fromId, int toId);
    static void SetItemState(int item, int state);
    static int GetItemState(int item);
    static int GetItemCount() { return Game ? Game->GetItemCount() : 0; }
    static int GetItemId(int index) { return Game ? Game->GetItemId(index) : 0; }
    static int GetCurrentItemId() { return Game ? Game->GetCurrentItemId() : 0; }
    static void SetCurrentItemId(int item) { if (Game) Game->SetCurrentItemId(item); }
    static int SelectNextItem() { return Game ? Game->SelectNextItem() : 0; }
    static int SelectPreviousItem() { return Game ? Game->SelectPreviousItem() : 0; }

    static void SetItemState(int base, int item, int state);
    static int GetItemState(int base, int item);

    static void SetAskAboutState(int index, uint8_t state);
    static uint8_t GetAskAboutState(int index);
    static int GetAskAboutCount() { return Game ? Game->GetAskAboutCount() : 0; }
    static int GetAskAboutId(int index) { return Game ? Game->GetAskAboutId(index) : 0; }

    static void SetItemExamined(int itemId, int conditionalScore = 0) { if (Game) Game->SetItemExamined(itemId, conditionalScore); }

    static void Tick(int ticks);

    static bool ItemsChanged;
    static bool AskAboutChanged;
    static bool BuyChanged;

    static void LoadFromDMap(int entry) { if (Game) Game->LoadFromDMap(entry); }

    static void NewGame() { if (Game) Game->NewGame(); }
    
    static void LoadGame(const char* fileName) { if (Game) Game->LoadGame(const_cast<char*>(fileName)); }
    static void SaveGame(const char* fileName) { if (Game) Game->SaveGame(const_cast<char*>(fileName)); }

    static bool CanCancelTravel;
    static bool CanCancelVideo() { return Game ? Game->CanCancelVideo() : false; }
    static void CanCancelVideo(bool allow) { if (Game) Game->CanCancelVideo(allow); }

    static int GetLocationInitializationScriptId() { return Game ? Game->GetLocationInitializationScriptId() : 0; }
    static int GetLocationEnvironmentScriptId() { return Game ? Game->GetLocationEnvironmentScriptId() : 0; }

    static void AutoSave() { SaveGame("GAMES/SAVEGAME.000"); }

    static int GetHintCategoryCount() { return Game ? Game->GetHintCategoryCount() : 0; }
    static CHintCategory* GetHintCategory(int index) { return Game ? Game->GetHintCategory(index) : nullptr; }

    static CHintModule* GetHintModule() { return Game ? Game->GetHintModule() : nullptr; }

    static void SetSelectedItem(int item) { if (Game) Game->SetSelectedItem(item); }

    static int GetBuyableItemCount() { return Game ? Game->GetBuyableItemCount() : 0; }
    static int GetBuyableItemId(int index) { return Game ? Game->GetBuyableItemId(index) : 0; }
    static void SetBuyableItemState(int index, int state) { if (Game) Game->SetBuyableItemState(index, state); }
    
    static void SetBuyableItemName(int index, const std::string& name);
    static std::string GetBuyableItemName(int index);

protected:
    static CGameBase* Game;

    static std::unordered_map<int, std::string> FileMap;
    static std::unordered_map<int, std::string> AskAboutMap;
    static std::unordered_map<int, std::string> ItemMap;
    static std::unordered_map<int, std::string> BuyableItemMap;

    static std::unordered_map<int, std::string> _lSituations;
    static std::unordered_map<int, std::string> _dSituations;
};