#include "BIC.h"
#include "Utilities.h"
#include "MediaIdentifiers.h"
#include "DXSound.h"
#include <algorithm>

bool CBIC::Init(uint8_t* pData, int length)
{
    _embeddedAudioSize = 0;
    _firstAudioFrame = 0;
    _chunkTest = 0;

    CAnimBase::Init(pData, length);

    if (GetInt(pData, 0, 4) == BIC)
    {
        _width = GetInt(pData, 10, 2);
        _height = GetInt(pData, 12, 2);
        _rate = 6;
        _frameTime = 100;

        // Fix for some incorrect dimensions
        if (videoMode == VideoMode::Embedded && _width == 640)
        {
            _width = 432;
            _height = 324;
        }

        CreateBuffers(_width, _height, _factor);
        _texture.Init(_width, _height);

        for (int c = 0; c < 256; c++)
        {
            double r = pData[0x40 + c * 3 + 0];
            double g = pData[0x40 + c * 3 + 1];
            double b = pData[0x40 + c * 3 + 2];
            int ri = static_cast<uint8_t>((r * 255.0) / 63.0);
            int gi = static_cast<uint8_t>((g * 255.0) / 63.0);
            int bi = static_cast<uint8_t>((b * 255.0) / 63.0);
            int col = 0xff000000 | bi | (gi << 8) | (ri << 16);
            _pPalette[c] = col;
        }

		// Find video and audio pointers
		_videoFramePointer = 0x340;
		_audioFramePointer = 0x340;	// Initial main header is in first 0x40 bytes

		_chunkTest = 10 - GetInt(pData, 0x12, 2);
		_remainingAudioLength = GetInt(pData, 0x3c, 4);
		_embeddedAudioSize = GetInt(pData, 0x10, 2);
		_firstAudioFrame = GetInt(pData, 0x12, 2);

		if (_chunkTest > 0)
		{
			// We have preloaded wave data
			int preload = _chunkTest * _embeddedAudioSize;
			_videoFramePointer += preload;
		}

        if (_audioFramePointer != 0)
        {
            // This is a new audio buffer
            _remainingAudioLength = GetInt(_pInputBuffer, 0x3c, 4);

            AudioFormat fmt;
            fmt.channels = GetInt(_pInputBuffer, 0x2a, 2);
            fmt.samplesPerSec = GetInt(_pInputBuffer, 0x2c, 4);
            fmt.bitsPerSample = GetInt(_pInputBuffer, 0x36, 2);

            _sourceVoice = CDXSound::CreateAudioStream(fmt);
            if (_sourceVoice != nullptr)
            {
                if (_chunkTest > 0)
                {
                    int audioBytes = _videoFramePointer - 0x340;
                    _remainingAudioLength -= audioBytes;
                    
                    _sourceVoice->SubmitBuffer(_pInputBuffer + 0x340, audioBytes);
                    _audioFramesQueued++;
                }
            }
        }

        _framePointer = _videoFramePointer;
        return true;
    }

    return false;
}

int CBIC::ProcessBICFrame(int inPtr, int chunkSize)
{
    int outPtr = 0;
    int end = inPtr + chunkSize;
    int currentRow = 0;
    bool good = true;

    while (inPtr < end && good)
    {
        int type = *(_pInputBuffer + inPtr++);
        
        if ((type & 1) != 0)
        {
            outPtr = currentRow * _width;

            bool readOffset = false;
            int chunks = *reinterpret_cast<int16_t*>(_pInputBuffer + inPtr);
            inPtr += 2;
            if (chunks < 0)
            {
                chunks = -chunks;
                readOffset = true;
            }

            if (chunks >= 160)
            {
                good = false;
                break;
            }

            while (chunks > 0)
            {
                if (readOffset)
                {
                    outPtr += *(_pInputBuffer + inPtr++) * 4;
                    chunks--;
                }

                int count = *(_pInputBuffer + inPtr++);
                for (int i = 0; i < count; i++)
                {
                    uint8_t b = *(_pInputBuffer + inPtr++);
                    int copyOut = outPtr;
                    for (int y = 0; y < 4; y++)
                    {
                        for (int x = 0; x < 4; x++)
                        {
                            _pVideoOutputBuffer[copyOut + x] = b;
                        }

                        copyOut += _width;
                    }

                    outPtr += 4;
                }

                chunks--;
                readOffset = true;
            }
        }

        if (good && (type & 2) != 0)
        {
            outPtr = currentRow * _width;

            bool readOffset = false;
            int chunks = *reinterpret_cast<int16_t*>(_pInputBuffer + inPtr);
            inPtr += 2;
            if (chunks < 0)
            {
                chunks = -chunks;
                readOffset = true;
            }

            if (chunks >= 160)
            {
                good = false;
                break;
            }

            while (chunks > 0)
            {
                if (readOffset)
                {
                    outPtr += *(_pInputBuffer + inPtr++) * 4;
                    chunks--;
                }

                int count = *(_pInputBuffer + inPtr++);
                for (int i = 0; i < count; i++)
                {
                    int c1 = *(_pInputBuffer + inPtr++);
                    int c2 = *(_pInputBuffer + inPtr++);
                    int pattern = *reinterpret_cast<int16_t*>(_pInputBuffer + inPtr);
                    inPtr += 2;
                    int copyOut = outPtr;
                    for (int y = 0; y < 4; y++)
                    {
                        _pVideoOutputBuffer[copyOut + 0] = static_cast<uint8_t>(((pattern & 1) != 0) ? c2 : c1);
                        _pVideoOutputBuffer[copyOut + 1] = static_cast<uint8_t>(((pattern & 2) != 0) ? c2 : c1);
                        _pVideoOutputBuffer[copyOut + 2] = static_cast<uint8_t>(((pattern & 4) != 0) ? c2 : c1);
                        _pVideoOutputBuffer[copyOut + 3] = static_cast<uint8_t>(((pattern & 8) != 0) ? c2 : c1);

                        copyOut += _width;
                        pattern >>= 4;
                    }

                    outPtr += 4;
                }

                chunks--;
                readOffset = true;
            }
        }

        if (good && (type & 4) != 0)
        {
            outPtr = currentRow * _width;

            bool readOffset = false;
            int chunks = *reinterpret_cast<int16_t*>(_pInputBuffer + inPtr);
            inPtr += 2;
            if (chunks < 0)
            {
                chunks = -chunks;
                readOffset = true;
            }

            if (chunks >= 160)
            {
                good = false;
                break;
            }

            while (chunks > 0)
            {
                if (readOffset)
                {
                    outPtr += *(_pInputBuffer + inPtr++) * 4;
                    chunks--;
                }

                int count = *(_pInputBuffer + inPtr++);
                for (int i = 0; i < count; i++)
                {
                    int copyOut = outPtr;
                    for (int y = 0; y < 4; y++)
                    {
                        for (int x = 0; x < 4; x++)
                        {
                            _pVideoOutputBuffer[copyOut + x] = *(_pInputBuffer + inPtr++);
                        }

                        copyOut += _width;
                    }

                    outPtr += 4;
                }

                chunks--;
                readOffset = true;
            }
        }

        if (good && (type & 8) != 0)
        {
            outPtr = currentRow * _width;

            bool readOffset = false;
            int chunks = *reinterpret_cast<int16_t*>(_pInputBuffer + inPtr);
            inPtr += 2;
            if (chunks < 0)
            {
                chunks = -chunks;
                readOffset = true;
            }

            if (chunks >= 160)
            {
                good = false;
                break;
            }

            while (chunks > 0)
            {
                if (readOffset)
                {
                    outPtr += *(_pInputBuffer + inPtr++) * 4;
                    chunks--;
                }

                int count = *(_pInputBuffer + inPtr++);
                for (int i = 0; i < count; i++)
                {
                    int c = *(_pInputBuffer + inPtr++);
                    int c2 = (c >> 4) & 0xf;
                    int c1 = c & 0xf;
                    int pattern = *reinterpret_cast<int16_t*>(_pInputBuffer + inPtr);
                    inPtr += 2;

                    int copyOut = outPtr;
                    for (int y = 0; y < 4; y++)
                    {
                        _pVideoOutputBuffer[copyOut + 0] = static_cast<uint8_t>(((pattern & 1) != 0) ? c2 : c1);
                        _pVideoOutputBuffer[copyOut + 1] = static_cast<uint8_t>(((pattern & 2) != 0) ? c2 : c1);
                        _pVideoOutputBuffer[copyOut + 2] = static_cast<uint8_t>(((pattern & 4) != 0) ? c2 : c1);
                        _pVideoOutputBuffer[copyOut + 3] = static_cast<uint8_t>(((pattern & 8) != 0) ? c2 : c1);

                        copyOut += _width;
                        pattern >>= 4;
                    }

                    outPtr += 4;
                }

                chunks--;
                readOffset = true;
            }
        }

        currentRow += 4;
    }

    return end;
}

bool CBIC::DecodeFrame()
{
    bool ret = false;

    if (_pInputBuffer != nullptr && _framePointer >= 0 && _framePointer < _inputBufferLength)
    {
        int chunkSize = *reinterpret_cast<int32_t*>(_pInputBuffer + _framePointer);
        int end = ProcessBICFrame(_framePointer + 4, chunkSize);

        if (end > _framePointer)
        {
            _framePointer += chunkSize + 4;

            _chunkTest++;
            if (_chunkTest >= 1 && _remainingAudioLength > 0)
            {
                int remainingBufferLength = _inputBufferLength - _framePointer;
                int waveChunkSize = (remainingBufferLength == _remainingAudioLength) ? _remainingAudioLength : std::min(_embeddedAudioSize, _remainingAudioLength);
                _remainingAudioLength -= waveChunkSize;

                if (_sourceVoice != nullptr)
                {
                    _sourceVoice->SubmitBuffer(_pInputBuffer + _framePointer, waveChunkSize);
                    _audioFramesQueued++;
                }

                _framePointer += waveChunkSize;
            }

            if (_frame == _firstAudioFrame && _sourceVoice != nullptr)
            {
                _sourceVoice->Start();
            }

            ret = true;
        }
    }

    return ret;
}
