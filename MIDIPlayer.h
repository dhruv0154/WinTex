#pragma once

#include "BinaryData.h"
#include "Mutex.h"
#include <cstdint>
#include <thread>
#include <atomic>

class CMIDIPlayer
{
public:
    CMIDIPlayer();
    virtual ~CMIDIPlayer();

    void CloseDevice();
    void OpenDevice(uint32_t deviceId);

    virtual void Init(BinaryData data);
    void Stop();
    void Start();
    void Pause();
    void Resume();
    void SetVolume(float volume);

protected:
    static BinaryData _data;

    std::thread _midiThread;
    std::atomic<bool> _running{false};

    static bool _midiEnabled;
    virtual uint32_t Player();
    static void PlayerThread(CMIDIPlayer* pPlayer);

    static void SendMidiMessage(uint32_t cmd);
    static void SendMidiSysEx(const uint8_t* data, size_t len);

    static uint8_t* _channels[16];
    static int _delays[16];
    static int _division;
    static int _duration;
    static int _volumes[16];

    static CMutex _midiMutex;
    static bool _changed;
};