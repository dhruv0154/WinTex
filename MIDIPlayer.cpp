#include "MIDIPlayer.h"
#include "Utilities.h"
#include "Configuration.h"
#include "Globals.h"
#include <cstring> 
#include <algorithm>
#include <chrono> 

#ifndef TIMER_SCALE
#define TIMER_SCALE 2
#endif

bool CMIDIPlayer::_midiEnabled = false;
BinaryData CMIDIPlayer::_data;

uint8_t* CMIDIPlayer::_channels[16];
int CMIDIPlayer::_delays[16];
int CMIDIPlayer::_volumes[16];

int CMIDIPlayer::_division = 0;
int CMIDIPlayer::_duration = 0;

CMutex CMIDIPlayer::_midiMutex;
bool CMIDIPlayer::_changed = false;

CMIDIPlayer::CMIDIPlayer()
{
    _data.Data = nullptr;
    _data.Length = 0;

    std::memset(_channels, 0, sizeof(uint8_t*) * 16);
    std::memset(_delays, 0, sizeof(int) * 16);
    std::memset(_volumes, 0, sizeof(int) * 16);

    OpenDevice(pConfig ? pConfig->MIDIDeviceId : 0);

    _running = true;
    _midiThread = std::thread(&CMIDIPlayer::PlayerThread, this);
}

void CMIDIPlayer::CloseDevice()
{
    Stop();
}

void CMIDIPlayer::OpenDevice(uint32_t deviceId)
{
    CloseDevice();
    if (pConfig)
    {
        SetVolume((pConfig->MIDIVolume) / 100.0f);
    }
}

CMIDIPlayer::~CMIDIPlayer()
{
    Stop();

    _running = false;
    if (_midiThread.joinable())
    {
        _midiThread.join();
    }

    if (_data.Data != nullptr)
    {
        delete[] _data.Data;
        _data.Data = nullptr;
    }
}

void CMIDIPlayer::Init(BinaryData data)
{
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

        // Reset synthesizer on device (0xFF = MIDI System Reset)
        SendMidiMessage(0xff);

        _data = data;

        std::memset(_channels, 0, sizeof(uint8_t*) * 16);
        std::memset(_delays, 0, sizeof(int) * 16);
        std::memset(_volumes, 0, sizeof(int) * 16);

        if (data.Data != nullptr)
        {
            uint8_t* pMIDI = data.Data;

            int channelCount = GetInt(pMIDI, 0x30, 4);
            int check = GetInt(pMIDI, 0x34, 4);
            _division = GetInt(pMIDI, 0x38, 4);
            _duration = GetInt(pMIDI, 0x3c, 4);

            uint8_t* pChannel = pMIDI + 0x308;
            for (int i = 0; i < channelCount && i < 16; i++)
            {
                int channelLength = GetInt(pChannel, 4, 4);

                int div = 0, v = 0;
                check = 12;
                int shift = 0;
                do
                {
                    v = pChannel[check++];
                    div |= (v & 0x7f) << shift;
                    shift += 7;
                } while ((v & 0x80) == 0);

                _delays[i] = static_cast<int>(div * TIMER_SCALE / 2);
                _channels[i] = pChannel + check;
                pChannel += channelLength;
            }
        }

        _midiEnabled = true;
        _midiMutex.Release();
    }
}

void CMIDIPlayer::Stop()
{
    if (_midiMutex.Lock())
    {
        _midiEnabled = false;

        for (int i = 0; i < 16; i++)
        {
            SendMidiMessage(0x00007bb0 | i);
        }

        _midiMutex.Release();
    }
}

void CMIDIPlayer::Start()
{
    if (_midiMutex.Lock())
    {
        _midiEnabled = true;
        _midiMutex.Release();
    }
}

void CMIDIPlayer::Pause()
{
    if (_midiMutex.Lock())
    {
        _midiEnabled = false;

        for (int i = 0; i < 16; i++)
        {
            SendMidiMessage(0x000007b0 | i);
        }

        _midiMutex.Release();
    }
}

void CMIDIPlayer::Resume()
{
    if (_midiMutex.Lock())
    {
        // Restore previous channel volumes
        for (int i = 0; i < 16; i++)
        {
            SendMidiMessage(0x000007b0 | i | (_volumes[i] << 16));
        }

        _midiEnabled = true;
        _midiMutex.Release();
    }
}

void CMIDIPlayer::SetVolume(float volume)
{
    int v = static_cast<int>(0x3fff * volume);
    uint8_t data[8];
    data[0] = 0xf0;
    data[1] = 0x7f;
    data[2] = 0x7f;
    data[3] = 0x04;
    data[4] = 0x01;
    data[5] = v & 0x7f;
    data[6] = (v >> 7) & 0x7f;
    data[7] = 0xf7;

    // Send Master Volume Universal System Exclusive (SysEx) message
    SendMidiSysEx(data, 8);
}

void CMIDIPlayer::PlayerThread(CMIDIPlayer* pPlayer)
{
    if (pPlayer != nullptr)
    {
        pPlayer->Player();
    }
}

uint32_t CMIDIPlayer::Player()
{
    while (_running)
    {
        _changed = false;

        if (pConfig && pConfig->PlayMIDI && _midiEnabled && _data.Data != nullptr)
        {
            while (_midiEnabled && _running)
            {
                // Find shortest delay of all 16 tracks, sleep, then execute
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
                    // Must assume end of data sequence
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
                                    // One extra byte required for 3-byte MIDI messages
                                    cmd |= pChannel[0] << 16;
                                    pChannel++;
                                }

                                if (cmd == 0x002fff)
                                {
                                    // End of track marker reached
                                    pChannel = nullptr;
                                    break;
                                }

                                SendMidiMessage(cmd);

                                // If this was a Volume Change Controller message (0x07b0), cache the new volume
                                if ((cmd & 0xfff0) == 0x07b0)
                                {
                                    _volumes[cmd & 0xf] = (cmd >> 16) & 0xff;
                                }

                                // Calculate new delay using Access Software's custom VLQ bit-shifting
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

void CMIDIPlayer::SendMidiMessage(uint32_t cmd)
{
}

void CMIDIPlayer::SendMidiSysEx(const uint8_t* data, size_t len)
{
}