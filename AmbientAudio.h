#pragma once

#include "BinaryData.h"
#include "Map.h"
#include "Mutex.h"
#include "DXSound.h"
#include <cstdint>
#include <string>
#include <list>
#include <unordered_map>

class CAmbientAudio
{
public:
    CAmbientAudio(BinaryData bd);
    CAmbientAudio()
    {
        _pData = nullptr;
        _length = 0;
        _sourceVoice = nullptr;
        _finished = true;
        _isLooping = false;
        TimeDisposed = 0;
    }
    ~CAmbientAudio();

    static void Clear();
    static void Loop(CMapData* mapEntry, int entry1, int entry2);
    static void LoadPD(CMapData* mapEntry, int entry1, int entry2);
    static void LoopPD(int entry);
    static void Play(CMapData* mapEntry, int entry, bool playAlways);
    static void Stop(int entry);
    static void StopAll();
    void Loop();
    void Play();
    void Play(uint8_t* pData);
    void Stop();

    static void SetVolume(int entry, float volume);
    void SetVolume(float volume);

    static void SetPan(int entry, float pan);
    void SetPan(float pan);

    uint64_t TimeDisposed;

protected:
    static bool Load(CMapData* mapEntry, int entry);
    static bool LoadPD(CMapData* mapEntry, int entry);

    static CAmbientAudio* Find(int entry);
    bool Finished();
    bool _finished;
    bool _isLooping;

    void Play(bool loop);

    uint8_t* _pData;
    int _length;
    CAudioStream* _sourceVoice;

    static std::unordered_map<int, CAmbientAudio*> Sounds;
    static std::list<CAmbientAudio*> SoundsToDelete;

    static void GC();
};
