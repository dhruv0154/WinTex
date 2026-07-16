#include "Wave.h"
#include "Utilities.h"
#include <SDL2/SDL.h>

CWave::CWave()
{
	_timeOfStart = 0;
}

CWave::~CWave()
{
}

bool CWave::Init(uint8_t* pData, int length)
{
	bool ret = CAnimBase::Init(pData, length);

	// Validate that the file is a WAVE
	if (ret && GetInt(pData, 0, 4) == 0x46464952)
	{
		_remainingAudioLength = GetInt(_pInputBuffer, 0x28, 4);
		_frameTime = 100;	// Default to 10 frames per second
	}

	return ret;
}

bool CWave::DecodeFrame()
{
	if (_remainingAudioLength > 0)
	{
		// This is a new audio buffer
		if (_sourceVoice == nullptr)
		{
			AudioFormat fmt;
            fmt.channels = GetInt(_pInputBuffer, 0x16, 2);
            fmt.samplesPerSec = GetInt(_pInputBuffer, 0x18, 4);
            fmt.bitsPerSample = GetInt(_pInputBuffer, 0x22, 2);
            
            // Route to our new SDL Audio Engine
            _sourceVoice = CDXSound::CreateAudioStream(fmt);
		}

		if (_sourceVoice != nullptr)
		{
			_sourceVoice->Start();

            _sourceVoice->SubmitBuffer((const uint8_t*)(_pInputBuffer + 0x2c), _remainingAudioLength);

            _audioFramesQueued = 1;
            _remainingAudioLength = 0;

            _timeOfStart = SDL_GetTicks64();
		}
	}

	return true;
}
