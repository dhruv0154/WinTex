#include "DXSound.h"
#include "Configuration.h"
#include "Globals.h"
#include <iostream>
#include <algorithm>

SDL_AudioDeviceID CDXSound::_audioDevice = 0;
std::vector<CDXSound*> CDXSound::_activeSounds;
std::vector<CAudioStream*> CDXSound::_activeStreams; // Global list of active voices
std::mutex CDXSound::_mutex;
float CDXSound::_masterVolume = 1.0f;
int CDXSound::_outputFreq = 44100;

CAudioStream::CAudioStream(const AudioFormat& format) {
    _playing = false;
    _volume = 1.0f;
    _format = format;

    std::lock_guard<std::mutex> lock(CDXSound::_mutex);
    CDXSound::_activeStreams.push_back(this);
}

CAudioStream::~CAudioStream() {
    DestroyStream();
}

void CAudioStream::DestroyStream() {
    std::lock_guard<std::mutex> lock(CDXSound::_mutex);
    auto it = std::find(CDXSound::_activeStreams.begin(), CDXSound::_activeStreams.end(), this);
    if (it != CDXSound::_activeStreams.end()) {
        CDXSound::_activeStreams.erase(it);
    }
}

void CAudioStream::SubmitBuffer(const uint8_t *pData, uint32_t size) {
    if (!pData) return;
    
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    QueuedBuffer qb;
    qb.data = pData;
    qb.size = size;
    qb.position = 0.0;
    _buffers.push_back(qb);
}

void CAudioStream::Start() { _playing = true; }
void CAudioStream::Stop() { _playing = false; }
void CAudioStream::SetVolume(float volume) { _volume = volume; }

void CAudioStream::Mix(int32_t* dst, int numSamples) {
    if (!_playing) return;
    
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_buffers.empty()) return;

    int samplesMixed = 0;
    
    // Calculate bytes per sample based on format
    int bytesPerSample = (_format.bitsPerSample / 8) * _format.channels;
    if (bytesPerSample == 0) bytesPerSample = 2; // Default to 16-bit mono

    float step = 1.0f;
    if (_format.samplesPerSec > 0) {
        step = (float)_format.samplesPerSec / (float)CDXSound::_outputFreq;
    }

    while (samplesMixed < numSamples && !_buffers.empty()) {
        QueuedBuffer& buf = _buffers.front();
        
        while (samplesMixed < numSamples) {
             int pos = (int)buf.position;
             // Align to block
             pos -= (pos % bytesPerSample);
             
             if (pos >= buf.size) {
                 // buffer exhausted
                 break;
             }
             
             int16_t sample = 0;
             if (_format.bitsPerSample == 8) {
                 // 8-bit audio is unsigned 0-255
                 if (_format.channels == 1) {
                     // 8-bit Mono
                     if (pos < buf.size) {
                         uint8_t val = buf.data[pos];
                         sample = (int16_t)((val - 128) * 256);
                     }
                 } else {
                     // 8-bit Stereo
                     if (pos + 1 < buf.size) {
                         uint8_t l = buf.data[pos];
                         uint8_t r = buf.data[pos + 1];
                         sample = (int16_t)((((l - 128) + (r - 128)) / 2) * 256);
                     }
                 }
             } else {
                 // Assume 16-bit
                 if (_format.channels == 1) {
                     // 16-bit Mono
                     if (pos + 1 < buf.size) {
                        sample = ((const int16_t*)(buf.data + pos))[0];
                     }
                 } else {
                     // 16-bit Stereo or more: Average channels to mono
                     if (pos + 3 < buf.size) {
                        int16_t l = ((const int16_t*)(buf.data + pos))[0];
                        int16_t r = ((const int16_t*)(buf.data + pos))[1];
                        sample = (l + r) / 2;
                     }
                 }
             }
             
             dst[samplesMixed++] += (int32_t)(sample * _volume);
             
             buf.position += (step * bytesPerSample);
        }

        if ((int)buf.position >= buf.size) {
            _buffers.erase(_buffers.begin());
        }
    }
}

void CDXSound::AudioCallback(void* userdata, Uint8* stream, int len) {
    std::lock_guard<std::mutex> lock(_mutex);
    SDL_memset(stream, 0, len);

    // Mix into a temporary buffer to handle volume and clipping
    // Assuming 16-bit signed audio (S16SYS)
    // len is in bytes. Samples = len / 2
    int sampleCount = len / 2;
    std::vector<int32_t> mixBuffer(sampleCount, 0);

    // Mix Sounds
    if (!_activeSounds.empty()) {
        float step = 44100.0f / (float)_outputFreq;
        
        for (auto it = _activeSounds.begin(); it != _activeSounds.end(); ) {
            CDXSound* sound = *it;
            if (!sound->_audioData.playing) {
                it = _activeSounds.erase(it);
                continue;
            }
            
            if (sound->_audioData.data == nullptr) {
                 it = _activeSounds.erase(it);
                 continue;
            }

            // Mix
            for (int i = 0; i < sampleCount; ++i) {
                int pos = (int)sound->_audioData.position;
                // Align to 2 bytes (16-bit mono)
                pos -= (pos % 2);
                
                if (pos + 1 >= sound->_audioData.length) {
                    sound->_audioData.playing = false;
                    break;
                }
                
                const int16_t* src = (const int16_t*)(sound->_audioData.data + pos);
                mixBuffer[i] += *src;
                
                sound->_audioData.position += (step * 2.0); // 2 bytes per sample
            }
            
            ++it;
        }
    }
    
    // Mix Voices (BIC)
    if (!_activeStreams.empty()) {
        for (auto voice : _activeStreams) {
            voice->Mix(mixBuffer.data(), sampleCount);
        }
    }

    // Clip and write back
    int16_t* dst = (int16_t*)stream;
    for (int i = 0; i < sampleCount; ++i) {
        int32_t val = (int32_t)(mixBuffer[i] * _masterVolume);
        if (val > 32767) val = 32767;
        if (val < -32768) val = -32768;
        dst[i] = (int16_t)val;
    }
}

CDXSound::CDXSound()
{
    _audioData.playing = false;
    _audioData.data = nullptr;
    _audioData.length = 0;
    _audioData.position = 0.0;
}

CDXSound::~CDXSound()
{
    Stop();
}

void CDXSound::Init()
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Audio Init Failed: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 2048; // Larger buffer to prevent stutter
    want.callback = AudioCallback;
    
    _audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (_audioDevice == 0) {
        std::cerr << "SDL OpenAudioDevice Failed: " << SDL_GetError() << std::endl;
    } else {
        SDL_PauseAudioDevice(_audioDevice, 0);
        _outputFreq = have.freq;
    }
    SetVolume((pConfig->Volume) / 100.0f);
}

void CDXSound::Dispose()
{
    if (_audioDevice != 0) {
        SDL_CloseAudioDevice(_audioDevice);
        _audioDevice = 0;
    }
}

void CDXSound::Play(uint8_t* pData, uint32_t size)
{
	Stop();

    if (_audioDevice == 0) {
        std::cerr << "Play called but audio device not initialized" << std::endl;
        return;
    }
    
    if (pData == nullptr) {
        std::cerr << "Play called with nullptr data" << std::endl;
        return;
    }

    // Validate size (header skip)
    if (size <= 64) {
        std::cerr << "Play called with invalid size: " << size << std::endl;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _audioData.data = pData + 64;
        _audioData.length = size - 64;
        _audioData.position = 0.0;
        _audioData.playing = true;
        
        bool found = false;
        for (auto s : _activeSounds) {
            if (s == this) {
                found = true;
                break;
            }
        }
        if (!found) {
            _activeSounds.push_back(this);
        }
    }
}

void CDXSound::Stop()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _audioData.playing = false;
    // We remove it immediately to be safe
    for (auto it = _activeSounds.begin(); it != _activeSounds.end(); ++it) {
        if (*it == this) {
            _activeSounds.erase(it);
            break;
        }
    }
}

CAudioStream* CDXSound::CreateAudioStream(const AudioFormat& format)
{
    return new CAudioStream(format);
}

void CDXSound::SetVolume(float volume)
{
    _masterVolume = volume;
}
