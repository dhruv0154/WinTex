#pragma once

#include "ScriptState.h"
#include <string>
#include <cstdint>

class CUAKMScriptState : public CScriptState
{
public:
    virtual void Init(uint8_t* script, int length, const std::string& file, int entry) override
    {
        Script = script;
        Length = length;
        ScriptFile = file;
        ScriptEntry = entry;

        // Generate list of script entries
        _scriptEntries.clear();
        for (int i = 0; i < (length - 2); i++)
        {
            if (script[i] == 0xe0)  // This is really not a good way to find script entries, what if a value is used that has a byte = 0xe0?
            {
                int id = ((int)script[i + 1]) | (((int)script[i + 2]) << 8);
                if (_scriptEntries.find(id) == _scriptEntries.end())
                {
                    _scriptEntries[id] = i + 3;
                }
                i += 2;
            }
        }
    }
};