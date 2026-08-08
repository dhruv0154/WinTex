#include "PDMIDIPlayer.h"
#include "Utilities.h"
#include "Configuration.h"
#include "Globals.h"
#include <cstring>
#include <algorithm>
#include <chrono>
#include <thread>

void CPDMIDIPlayer::Init(BinaryData data)
{
	// Track/Channel list at 0x172
	// Assuming max 16 channels
	// Supposed to support looping and conditional branching
	// Number of tracks @ 0xE4

    Stop();

    if (_midiMutex.Lock())
    {
        if (_data.Data != nullptr)
        {
            delete[] _data.Data;
            _data.Data = nullptr;
        }

        _changed = true;
        _midiEnabled = false;
        while (_changed == true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Reset on device (0xFF = MIDI System Reset)
        SendMidiMessage(0xff);

        _data = data;

        std::memset(_channels, 0, sizeof(uint8_t*) * 16);
        std::memset(_delays, 0, sizeof(int) * 16);
        std::memset(_volumes, 0, sizeof(int) * 16);

        if (data.Data != nullptr)
        {
            uint8_t* pMIDI = data.Data;

            int channelCount = GetInt(pMIDI, 0xe4, 4);
            int check = GetInt(pMIDI, 0x34, 4);
            _division = GetInt(pMIDI, 0xd2, 2);
            _duration = GetInt(pMIDI, 0xd4, 4);

            for (int i = 0; i < channelCount && i < 16; i++)
            {
                _channels[i] = pMIDI + GetInt(pMIDI, 0x172 + i * 4, 4);
                _channels[i] += GetInt(_channels[i], 0x57, 4);
            }
        }

        _midiEnabled = true;
        _midiMutex.Release();
    }
}

uint32_t CPDMIDIPlayer::Player()
{
    while (_running)
    {
        _changed = false;

        if (pConfig && pConfig->PlayMIDI && _midiEnabled && _data.Data != nullptr)
        {
            while (_midiEnabled && _running)
            {
                // Find shortest delay of all tracks, sleep, then execute
                int sleepTime = 1000000000;
                if (_midiMutex.Lock())
                {
                    for (int i = 0; _midiEnabled && i < 16; i++)
                    {
                        if (_channels[i] != nullptr && _delays[i] < sleepTime)
                        {
                            sleepTime = _delays[i];
                        }
                    }

                    _midiMutex.Release();
                }

                if (sleepTime == 1000000000)
                {
                    // Must assume end of data
                    _midiEnabled = false;
                    _data.Data = nullptr;
                    break;
                }

                int remainingSleepTime = sleepTime;
                while (_midiEnabled && _running && remainingSleepTime > 0)
                {
                    int sleepMs = std::min(remainingSleepTime, 200);
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
                    remainingSleepTime -= sleepMs;

                    if (_changed || !_midiEnabled)
                    {
                        break;
                    }
                }

                if (_changed || !_midiEnabled || !_running)
                {
                    break;
                }

                if (_midiMutex.Lock())
                {
                    for (int i = 0; !_changed && _midiEnabled && i < 16; i++)
                    {
                        if (_channels[i] != nullptr)
                        {
                            _delays[i] -= sleepTime;
                            uint8_t* pChannel = _channels[i];
                            while (pChannel != nullptr && _delays[i] <= 0)
                            {
                                int cmd = pChannel[0];
                                cmd |= pChannel[1] << 8;
                                pChannel += 2;
                                if ((cmd & 0xf0) <= 0xb0 || (cmd & 0xf0) >= 0xe0 || cmd == 0xff)
                                {
                                    // One extra byte
                                    cmd |= pChannel[0] << 16;
                                    pChannel++;
                                }

                                if (cmd == 0x002fff)
                                {
                                    pChannel = nullptr;
                                    break;
                                }

                                SendMidiMessage(cmd);

                                if ((cmd & 0xfff0) == 0x07b0)
                                {
                                    _volumes[cmd & 0xf] = (cmd >> 16) & 0xff;
                                }

                                // Calculate new delay
                                int div = 0, v = 0;
                                int shift = 0;
                                do
                                {
                                    v = *pChannel++;
                                    div |= ((v & 0x7f) << shift);
                                    shift += 7;
                                } while ((v & 0x80) == 0);
                                _delays[i] = div * 8;
                            }

                            _channels[i] = pChannel;
                        }
                    }

                    _midiMutex.Release();
                }
            }

            _changed = false;
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    return 0;
}