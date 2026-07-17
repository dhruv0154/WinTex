#pragma once
#include <cstdint>
#include <vector>
#include <mutex>
#include <SDL2/SDL.h>

struct AudioFormat {
    uint16_t channels;
    uint32_t samplesPerSec;
    uint16_t bitsPerSample;
};

class CAudioStream {
public:
    CAudioStream(const AudioFormat& format);
    ~CAudioStream();

    void SubmitBuffer(const uint8_t* pData, uint32_t size);
    void Stop();
    void Start();
    void SetVolume(float Volume);
    void DestroyStream();

    int GetPendingBufferCount() { return _buffers.size(); }

    // Internal
    void Mix(int32_t* dst, int numSamples);
    bool IsPlaying() const { return _playing; }
    
private:
    bool _playing;
    float _volume;
    AudioFormat _format;
    
    struct QueuedBuffer {
        const uint8_t* data;
        uint32_t size;
        double position;
    };
    std::vector<QueuedBuffer> _buffers;
    std::recursive_mutex _mutex;
};

class CDXSound
{
public:
	CDXSound();
	~CDXSound();

	static void Init();
	static void Dispose();
	static CAudioStream* CreateAudioStream(const AudioFormat& format);

	void Stop();
	void Play(uint8_t* pData, uint32_t size);

	static void SetVolume(float volume);

	struct AudioData {
		const uint8_t* data;
		uint32_t length;
		double position;
		bool playing;
	};
	AudioData _audioData;
    static std::vector<CAudioStream*> _activeStreams;
    static std::mutex _mutex;
    static int _outputFreq;

protected:
	static void AudioCallback(void* userdata, Uint8* stream, int len);
	static SDL_AudioDeviceID _audioDevice;
	static std::vector<CDXSound*> _activeSounds;
	static float _masterVolume;
};
