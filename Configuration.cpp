#include "Configuration.h"
#include "Globals.h"
#include "Utilities.h"
#include "GameController.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm> 
#include <cmath>

#define CONFIG_FLAGS_FULLSCREEN         1
#define CONFIG_FLAGS_CAPTIONS           2
#define CONFIG_FLAGS_ALTERNATIVEMEDIA   64
#define CONFIG_FLAGS_PLAYMIDI           128
#define CONFIG_FLAGS_INVERTY            256
#define CONFIG_FLAGS_ANISOTROPICFILTER  512

CConfiguration::CConfiguration() {}

CConfiguration::CConfiguration(const char* gameName) : _gameName{gameName}
{
    std::ifstream configFile("settings.cfg");
    if (configFile.is_open())
    {
        std::string line;
        while (std::getline(configFile, line))
        {
            std::istringstream iss(line);
            std::string key;
            if (std::getline(iss, key, '='))
            {
                std::string valStr;
                if (std::getline(iss, valStr))
                {
                    try {
                        if (key == "Width") Width = std::stoi(valStr);
                        else if (key == "Height") Height = std::stoi(valStr);
                        else if (key == "Flags") {
                            int flags = std::stoi(valStr);
                            FullScreen = ((flags & CONFIG_FLAGS_FULLSCREEN) != 0);
                            Captions = ((flags & CONFIG_FLAGS_CAPTIONS) != 0);
                            AlternativeMedia = ((flags & CONFIG_FLAGS_ALTERNATIVEMEDIA) != 0);
                            PlayMIDI = ((flags & CONFIG_FLAGS_PLAYMIDI) != 0);
                            InvertY = ((flags & CONFIG_FLAGS_INVERTY) != 0);
                            AnisotropicFilter = ((flags & CONFIG_FLAGS_ANISOTROPICFILTER) == 0); // Inverted
                        }
                        else if (key == "MIDIDeviceId") MIDIDeviceId = std::stoi(valStr);
                        else if (key == "MouselookScaling") MouselookScaling = std::stof(valStr);
                        else if (key == "FontScale") FontScale = std::max(1.0f, std::min(std::stof(valStr), 3.0f));
                        else if (key == "Volume") Volume = std::max(0.0f, std::min(std::stof(valStr), 100.0f));
                        else if (key == "MIDIVolume") MIDIVolume = std::max(0.0f, std::min(std::stof(valStr), 100.0f));
                    } catch (...) {
                    }
                }
            }
        }
        configFile.close();
    }

    pAdapter = dx.GetAdapter();
    if (pAdapter != nullptr)
    {
        int currentModeSize = Width * Height;
        int currentMode = -1;
        int currentModeDiff = 1000000000;

        for (int m = 0; m < static_cast<int>(pAdapter->_numModes); m++)
        {
            if (pAdapter->_displayModeList[m].Width >= 640 && pAdapter->_displayModeList[m].Height >= 480)
            {
                int modeSize = pAdapter->_displayModeList[m].Width * pAdapter->_displayModeList[m].Height;
                int modeDiff = std::abs(modeSize - currentModeSize);
                if (modeDiff < currentModeDiff)
                {
                    currentMode = m;
                    currentModeDiff = modeDiff;
                }

                if (Width == static_cast<int>(pAdapter->_displayModeList[m].Width) && 
                    Height == static_cast<int>(pAdapter->_displayModeList[m].Height))
                {
                    ScreenMode = m;
                    break;
                }
            }
            else
            {
                MinAcceptedMode = m + 1;
            }
        }

        if (currentMode >= 0)
        {
            Width = pAdapter->_displayModeList[currentMode].Width;
            Height = pAdapter->_displayModeList[currentMode].Height;
        }
    }

    NumberOfMIDIOutDevices = 1;
    MIDIOUTCAPSA defaultMidiDevice;
    MIDIDevices.push_back(defaultMidiDevice);
}

void CConfiguration::Save()
{
    std::ofstream configFile("settings.cfg", std::ios::trunc);
    if (configFile.is_open())
    {
        int flags = 0;
        if (FullScreen) flags |= CONFIG_FLAGS_FULLSCREEN;
        if (Captions) flags |= CONFIG_FLAGS_CAPTIONS;
        if (AlternativeMedia) flags |= CONFIG_FLAGS_ALTERNATIVEMEDIA;
        if (PlayMIDI) flags |= CONFIG_FLAGS_PLAYMIDI;
        if (InvertY) flags |= CONFIG_FLAGS_INVERTY;
        if (!AnisotropicFilter) flags |= CONFIG_FLAGS_ANISOTROPICFILTER; // Inverted

        configFile << "Width=" << Width << "\n";
        configFile << "Height=" << Height << "\n";
        configFile << "Flags=" << flags << "\n";
        configFile << "MIDIDeviceId=" << MIDIDeviceId << "\n";
        configFile << "MouselookScaling=" << MouselookScaling << "\n";
        configFile << "FontScale=" << FontScale << "\n";
        configFile << "Volume=" << Volume << "\n";
        configFile << "MIDIVolume=" << MIDIVolume << "\n";

        configFile.close();
    }
}
