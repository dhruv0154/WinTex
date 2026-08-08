#pragma once

#include "DXAdapter.h"
#include <vector>
#include <string>
#include <cstdint>

struct MIDIOUTCAPSA {
    uint16_t wMid = 0;
    uint16_t wPid = 0;
    uint32_t vDriverVersion = 0;
    char szPname[32] = "SDL2 / FluidSynth MIDI";
    uint16_t wTechnology = 0;
    uint16_t wVoices = 0;
    uint16_t wNotes = 0;
    uint16_t wChannelMask = 0;
    uint32_t dwSupport = 0;
};

class CConfiguration
{
public:
    CConfiguration();
    CConfiguration(const char* gameName);

    int Width               {640};
    int Height              {480};
    bool FullScreen         {false};
    int ScreenMode          {-1};
    CDXAdapter* pAdapter    {nullptr};

    int MinAcceptedMode     {0};
    bool Captions           {true};
    bool AlternativeMedia   {false};
    bool PlayMIDI           {true};
    int MIDIDeviceId        {-1};

    bool InvertY            {false};
    float MouselookScaling  {1.0f};
    float FontScale         {1.0f};

    bool AnisotropicFilter  {true};
    float Volume            {100.0f};
    float MIDIVolume        {100.0f};

    int NumberOfMIDIOutDevices {0};
    std::vector<MIDIOUTCAPSA> MIDIDevices{};

    void Save();

    std::string GetGameName() { return _gameName; }

protected:
    std::string _gameName{};
};
