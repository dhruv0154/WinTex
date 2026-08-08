#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include "Enums.h"

class CScriptState
{
public:
    CScriptState();
    virtual ~CScriptState() = default;

    virtual void Init(uint8_t* script, int length, const std::string& file, int entry) = 0;
    void Clear();

    int GetInt(int offset, int size);
    int Read8();
    int Read8s();
    int Read16();
    int Read16s();
    int Read32();
    float Read16_16();
    float Read12_4();

    std::string ScriptFile{""};
    int ScriptEntry{0};

    int GetScript(int id);

    int ExecutionPointer{0};
    bool WaitingForMediaToFinish{false};
    bool WaitingForInput{false};
    bool WaitingForExternalModule{false};
    int SelectedOption{0};
    int SelectedValue{0};

    bool AskAbout{false};
    bool Offer{false};
    bool Buy{false};
    bool AskingAboutBuyables{false};

    int TopItemOffset{0};

    ActionType AllowedAction{ActionType::None};
    bool QueryAction{false};
    ActionType CurrentAction{ActionType::None};

    int LastDialoguePoint{-1};
    int FrameTrigger{-1};

    uint8_t* Script{nullptr};
    int Length{0};

    bool DebugMode{false};

    InteractionMode Mode{InteractionMode::None};

    int Parameter{-1};

protected:
    std::unordered_map<int, int> _scriptEntries;

    inline bool CanRead(int bytes) const
    {
        return (Script != nullptr) && (ExecutionPointer + bytes <= Length);
    }
};