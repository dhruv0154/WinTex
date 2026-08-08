#include "AmbientAudio.h"
#include "ModuleController.h"
#include "GameController.h"
#include "Utilities.h"
#include <SDL2/SDL.h>
#include <algorithm>

std::unordered_map<int, CAmbientAudio*> CAmbientAudio::Sounds;

std::list<CAmbientAudio*> CAmbientAudio::SoundsToDelete;

CAmbientAudio::CAmbientAudio(BinaryData bd)
{
    _sourceVoice = nullptr;
    _pData = bd.Data;
    _length = bd.Length;
    _finished = true;
    _isLooping = false;
    TimeDisposed = 0;
}

CAmbientAudio::~CAmbientAudio()
{
    if (_sourceVoice != nullptr)
    {
        _sourceVoice->Stop();
        _sourceVoice->DestroyStream();
        delete _sourceVoice;
        _sourceVoice = nullptr;
    }

    if (_pData != nullptr)
    {
        delete[] _pData;
        _pData = nullptr;
    }

    _length = 0;
}

void CAmbientAudio::Clear()
{
    for (auto& it : Sounds)
    {
        it.second->Stop();
        it.second->TimeDisposed = SDL_GetTicks64();
        SoundsToDelete.push_back(it.second);
    }

    Sounds.clear();
}

bool CAmbientAudio::Load(CMapData* mapEntry, int entry)
{
    if (Sounds.find(entry) != Sounds.end())
    {
        return true;
    }

    // Entry does not exist, load the file from the archive
    if (entry < static_cast<int>(mapEntry->EnvironmentAudioMap.size()))
    {
        FileMap fm = mapEntry->EnvironmentAudioMap[entry];
        std::string file = CGameController::GetFileName(fm.File);
        BinaryData bd = LoadEntry(file.c_str(), fm.Entry);
        if (bd.Data != nullptr)
        {
            Sounds[entry] = new CAmbientAudio(bd);
            return true;
        }
    }

    return false;
}

bool CAmbientAudio::LoadPD(CMapData* mapEntry, int entry)
{
    if (Sounds.find(entry) != Sounds.end())
    {
        return true;
    }

    // Entry does not exist, load the file
    if (entry < static_cast<int>(mapEntry->AudioMap.size()))
    {
        FileMap fm = mapEntry->AudioMap[entry];
        std::string file = CGameController::GetFileName(fm.File);
        BinaryData bd = LoadEntry(file.c_str(), fm.Entry);
        if (bd.Data != nullptr)
        {
            Sounds[entry] = new CAmbientAudio(bd);
            return true;
        }
    }

    return false;
}

void CAmbientAudio::Loop(CMapData* mapEntry, int entry1, int entry2)
{
    int entry = entry1 > 0 ? entry1 : entry2;
    entry = entry1 - 1;

    if (Load(mapEntry, entry))
    {
        Sounds[entry]->Loop();
    }
}

void CAmbientAudio::LoadPD(CMapData* mapEntry, int entry1, int entry2)
{
    if (Sounds.find(entry2) != Sounds.end())
    {
        return; // Already loaded
    }

    if (entry1 < static_cast<int>(mapEntry->AudioMap.size()))
    {
        FileMap fm = mapEntry->AudioMap[entry1];
        std::string file = CGameController::GetFileName(fm.File);
        BinaryData bd = LoadEntry(file.c_str(), fm.Entry);
        if (bd.Data != nullptr)
        {
            Sounds[entry2] = new CAmbientAudio(bd);
        }
    }
}

void CAmbientAudio::LoopPD(int entry)
{
    if (Sounds.find(entry) == Sounds.end())
    {
        return; // Not loaded
    }

    if (Sounds[entry]->Finished())
    {
        Sounds[entry]->Loop();
    }
}

void CAmbientAudio::Play(CMapData* mapEntry, int entry, bool playAlways)
{
    CAmbientAudio* pSound = Find(entry);
    if (pSound == nullptr || pSound->Finished() || playAlways)
    {
        if (Load(mapEntry, entry))
        {
            Sounds[entry]->Play();
        }
    }
}

void CAmbientAudio::Stop(int entry)
{
    entry--;
    if (Sounds.size() > 0 && Sounds.find(entry) != Sounds.end() && Sounds.find(entry)->second != nullptr)
    {
        Sounds[entry]->Stop();
    }
}

void CAmbientAudio::StopAll()
{
    for (auto& v : Sounds)
    {
        v.second->Stop();
    }

    Clear();
}

void CAmbientAudio::Loop()
{
    Play(true);
}

void CAmbientAudio::Play()
{
    Play(false);
}

void CAmbientAudio::Stop()
{
    _isLooping = false;
    _finished = true;
    if (_sourceVoice != nullptr)
    {
        _sourceVoice->Stop();
    }
}

bool CAmbientAudio::Finished()
{
    if (_finished) return true;
    if (_sourceVoice != nullptr)
    {
        if (!_isLooping && _sourceVoice->GetPendingBufferCount() == 0)
        {
            _finished = true;
            return true;
        }
    }
    return false;
}

void CAmbientAudio::Play(bool loop)
{
    _finished = false;
    _isLooping = loop;

    if (_sourceVoice != nullptr)
    {
        _sourceVoice->Stop();
        _sourceVoice->DestroyStream();
        delete _sourceVoice;
        _sourceVoice = nullptr;
    }

    AudioFormat fmt;
    fmt.channels = GetInt(_pData, 0x16, 2);
    fmt.samplesPerSec = GetInt(_pData, 0x18, 4);
    fmt.bitsPerSample = GetInt(_pData, 0x22, 2);

    _sourceVoice = CDXSound::CreateAudioStream(fmt);
    if (_sourceVoice != nullptr)
    {
        _sourceVoice->Start();

        uint32_t payloadSize = _length - 0x2c;
        const uint8_t* pPayload = _pData + 0x2c;

        _sourceVoice->SubmitBuffer(pPayload, payloadSize);
        if (_isLooping)
        {
            _sourceVoice->SubmitBuffer(pPayload, payloadSize);
        }
    }
}

void CAmbientAudio::Play(uint8_t* pData)
{
    _finished = false;
    _isLooping = false;

    if (_sourceVoice != nullptr)
    {
        _sourceVoice->Stop();
        _sourceVoice->DestroyStream();
        delete _sourceVoice;
        _sourceVoice = nullptr;
    }

    AudioFormat fmt;
    fmt.channels = GetInt(pData, 0x16, 2);
    fmt.samplesPerSec = GetInt(pData, 0x18, 4);
    fmt.bitsPerSample = GetInt(pData, 0x22, 2);

    _sourceVoice = CDXSound::CreateAudioStream(fmt);
    if (_sourceVoice != nullptr)
    {
        _sourceVoice->Start();

        int length = GetInt(pData, 0x28, 4);
        uint32_t payloadSize = length - 0x2c;
        const uint8_t* pPayload = pData + 0x2c;

        _sourceVoice->SubmitBuffer(pPayload, payloadSize);
    }
}

void CAmbientAudio::SetVolume(int entry, float volume)
{
    CAmbientAudio* pAudio = Find(entry);
    if (pAudio != nullptr)
    {
        pAudio->SetVolume(volume);
    }
}

void CAmbientAudio::SetVolume(float volume)
{
    if (_sourceVoice != nullptr)
    {
        _sourceVoice->SetVolume(volume);
    }
}

CAmbientAudio* CAmbientAudio::Find(int entry)
{
    CAmbientAudio* pRet = nullptr;
    if (Sounds.find(entry) != Sounds.end())
    {
        pRet = Sounds[entry];
    }

    return pRet;
}

void CAmbientAudio::SetPan(int entry, float pan)
{
    CAmbientAudio* pAudio = Find(entry);
    if (pAudio != nullptr)
    {
        pAudio->SetPan(pan);
    }
}

void CAmbientAudio::SetPan(float pan)
{
}

void CAmbientAudio::GC()
{
    uint64_t now = SDL_GetTicks64();

    for (auto& pair : Sounds)
    {
        CAmbientAudio* pSound = pair.second;
        if (pSound && pSound->_isLooping && pSound->_sourceVoice)
        {
            if (pSound->_sourceVoice->GetPendingBufferCount() < 2 && pSound->_pData != nullptr)
            {
                uint32_t payloadSize = pSound->_length - 0x2c;
                pSound->_sourceVoice->SubmitBuffer(pSound->_pData + 0x2c, payloadSize);
            }
        }
    }

    std::list<CAmbientAudio*> deleteThese;
    for (auto* it : SoundsToDelete)
    {
        if ((now - it->TimeDisposed) >= 60000)
        {
            deleteThese.push_back(it);
        }
    }

    for (auto* it : deleteThese)
    {
        SoundsToDelete.remove(it);
        delete it;
    }
}
