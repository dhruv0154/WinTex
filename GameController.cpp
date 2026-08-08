#include "GameController.h"
#include "Utilities.h"

CGameBase* CGameController::Game = nullptr;

std::unordered_map<int, std::string> CGameController::FileMap;

bool CGameController::CanCancelTravel = true;

std::unordered_map<int, std::string> CGameController::AskAboutMap;
std::unordered_map<int, std::string> CGameController::ItemMap;
std::unordered_map<int, std::string> CGameController::BuyableItemMap;

bool CGameController::ItemsChanged = false;
bool CGameController::AskAboutChanged = false;
bool CGameController::BuyChanged = false;

std::unordered_map<int, std::string> CGameController::_lSituations;
std::unordered_map<int, std::string> CGameController::_dSituations;

void CGameController::Init()
{
}

bool CGameController::StartGame(CGameBase* pGame)
{
    if (pGame == nullptr)
    {
        return false;
    }

    Game = pGame;
    Game->Start();

    return true;
}

uint8_t CGameController::GetParameter(int index)
{
    return Game ? Game->GetParameter(index) : 0;
}

void CGameController::SetParameter(int index, uint8_t value)
{
    if (Game) Game->SetParameter(index, value);
}

uint8_t CGameController::GetAskAboutState(int index)
{
    return Game ? Game->GetAskAboutState(index) : 0;
}

void CGameController::SetAskAboutState(int index, uint8_t state)
{
    AskAboutChanged = true;
    if (Game) Game->SetAskAboutState(index, state);
}

uint8_t CGameController::GetHintState(int index)
{
    return Game ? Game->GetHintState(index) : 0;
}

void CGameController::SetHintState(int index, uint8_t state, int score)
{
    if (Game) Game->SetHintState(index, state, score);
}

uint8_t CGameController::GetHintCategoryState(int index)
{
    return Game ? Game->GetHintCategoryState(index) : 0;
}

void CGameController::SetHintCategoryState(int index, uint8_t state)
{
    if (Game) Game->SetHintCategoryState(index, state);
}

int CGameController::GetItemState(int item)
{
    return Game ? Game->GetItemState(item) : 0;
}

void CGameController::SetItemState(int item, int state)
{
    if (Game) Game->SetItemState(item, state);
    ItemsChanged = true;
}

void CGameController::SetItemState(int base, int item, int state)
{
    if (Game) Game->SetItemState(base, item, state);
}

int CGameController::GetItemState(int base, int item)
{
    return Game ? Game->GetItemState(base, item) : 0;
}

void CGameController::SetFileName(int index, const std::string& name)
{
    FileMap[index] = name;
}

std::string CGameController::GetFileName(int index)
{
    auto it = FileMap.find(index);
    return (it != FileMap.end()) ? it->second : "";
}

void CGameController::SetItemName(int index, const std::string& name)
{
    ItemMap[index] = name;
}

std::string CGameController::GetItemName(int index)
{
    auto it = ItemMap.find(index);
    return (it != ItemMap.end()) ? it->second : "";
}

void CGameController::SetBuyableItemName(int index, const std::string& name)
{
    BuyableItemMap[index] = name;
}

std::string CGameController::GetBuyableItemName(int index)
{
    auto it = BuyableItemMap.find(index);
    return (it != BuyableItemMap.end()) ? it->second : "";
}

void CGameController::SetAskAboutName(int index, const std::string& name)
{
    AskAboutMap[index] = name;
}

std::string CGameController::GetAskAboutName(int index)
{
    auto it = AskAboutMap.find(index);
    return (it != AskAboutMap.end()) ? it->second : "";
}

void CGameController::SetTimer(int timer, int duration)
{
    if (Game) Game->SetTimer(timer, duration);
}

int CGameController::GetTimerState(int timer)
{
    return Game ? Game->GetTimerState(timer) : 0;
}

void CGameController::Tick(int ticks)
{
    if (Game) Game->Tick(ticks);
}

void CGameController::ResetTimers()
{
    if (Game) Game->ResetTimers();
}

void CGameController::SetSituationDescriptionL(int ix, const std::string& value)
{
    _lSituations[ix] = value;
}

std::string CGameController::GetSituationDescriptionL(int ix)
{
    auto it = _lSituations.find(ix);
    if (it == _lSituations.end() || it->second.empty())
    {
        it = _lSituations.find(-1);
    }

    return (it != _lSituations.end()) ? it->second : "";
}

void CGameController::SetSituationDescriptionD(int ix, const std::string& value)
{
    _dSituations[ix] = value;
}

std::string CGameController::GetSituationDescriptionD(int ix)
{
    auto it = _dSituations.find(ix);
    if (it == _dSituations.end() || it->second.empty())
    {
        it = _dSituations.find(-1);
    }

    return (it != _dSituations.end()) ? it->second : "";
}

void CGameController::TransformItem(int fromId, int toId)
{
    SetItemState(fromId, 2);
    SetItemState(toId, 1);
}