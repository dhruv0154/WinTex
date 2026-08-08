#pragma once

#include "ScriptState.h"
#include "Utilities.h"
#include <cstdint>
#include <string>

class CPDScriptState : public CScriptState
{
public:
    virtual ~CPDScriptState() = default;

    void Init(uint8_t* script, int length, const std::string& file, int entry) override
    {
        Script = script;
        Length = length;
        ScriptFile = file;
        ScriptEntry = entry;

        _scriptEntries.clear();

        if (Script == nullptr || Length < 16)
        {
            return;
        }

        int count = GetInt(0, 2);

        int requiredBytes = 16 + (count * 2);
        if (Length < requiredBytes)
        {
            count = (Length - 16) / 2;
        }

        for (int i = 0; i < count; i++)
        {
            _scriptEntries[i] = GetInt(16 + (i * 2), 2);
        }
    }
};
