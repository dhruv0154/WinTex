#include "ScriptState.h"
#include "Utilities.h"

CScriptState::CScriptState()
{
    Script = nullptr;
    Length = 0;
    DebugMode = false;
    Clear();
}

void CScriptState::Clear()
{
    ExecutionPointer = 0;
    WaitingForMediaToFinish = false;
    WaitingForInput = false;
    WaitingForExternalModule = false;
    SelectedOption = 0;
    SelectedValue = 0;

    AskAbout = false;
    Offer = false;
    Buy = false;
    AskingAboutBuyables = false;
    TopItemOffset = 0;

    Mode = InteractionMode::None;

    AllowedAction = ActionType::None;
    QueryAction = false;
    CurrentAction = ActionType::None;

    LastDialoguePoint = -1;
    FrameTrigger = -1;

    Parameter = -1;
}

int CScriptState::GetScript(int id)
{
    auto it = _scriptEntries.find(id);
    return (it != _scriptEntries.end() && it->second > 0) ? it->second : -1;
}

int CScriptState::GetInt(int offset, int size)
{
    if (Script == nullptr || offset < 0 || offset + size > Length)
    {
        return 0;
    }
    return ::GetInt(Script, offset, size);
}

int CScriptState::Read8()
{
    if (!CanRead(1)) return 0;
    return Script[ExecutionPointer++];
}

int CScriptState::Read8s()
{
    if (!CanRead(1)) return 0;
    return static_cast<int8_t>(Script[ExecutionPointer++]);
}

int CScriptState::Read16()
{
    if (!CanRead(2)) return 0;
    int ret = GetInt(ExecutionPointer, 2);
    ExecutionPointer += 2;
    return ret;
}

int CScriptState::Read16s()
{
    if (!CanRead(2)) return 0;
    int val = GetInt(ExecutionPointer, 2);
    ExecutionPointer += 2;
    return static_cast<int16_t>(val);
}

int CScriptState::Read32()
{
    if (!CanRead(4)) return 0;
    int ret = GetInt(ExecutionPointer, 4);
    ExecutionPointer += 4;
    return ret;
}

float CScriptState::Read16_16()
{
    if (!CanRead(4)) return 0.0f;
    int i = GetInt(ExecutionPointer, 4);
    ExecutionPointer += 4;
    return static_cast<float>(i) / 65536.0f;
}

float CScriptState::Read12_4()
{
    if (!CanRead(2)) return 0.0f;
    int i = GetInt(ExecutionPointer, 2);
    ExecutionPointer += 2;
    
    int16_t signedVal = static_cast<int16_t>(i);
    return static_cast<float>(signedVal) / 16.0f;
}