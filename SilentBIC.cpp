#include "SilentBIC.h"
#include "Utilities.h"

bool CSilentBIC::Init(uint8_t* pData, int length)
{
    _embeddedAudioSize = 0;
    _firstAudioFrame = 0;
    _chunkTest = 0;

    CAnimBase::Init(pData, length);

    _width = GetInt(pData, 6, 2);
    _height = GetInt(pData, 8, 2);
    _rate = 6;
    _frameTime = 100;

    CreateBuffers(_width, _height, _factor);
    _texture.Init(_width, _height);

    // Copy palette
    for (int c = 0; c < 256; c++)
    {
        double r = pData[12 + c * 3 + 0];
        double g = pData[12 + c * 3 + 1];
        double b = pData[12 + c * 3 + 2];
        int ri = static_cast<uint8_t>((r * 255.0) / 63.0);
        int gi = static_cast<uint8_t>((g * 255.0) / 63.0);
        int bi = static_cast<uint8_t>((b * 255.0) / 63.0);
        int col = 0xff000000 | bi | (gi << 8) | (ri << 16);
        _pPalette[c] = col;
    }

    _videoFramePointer = 0x30c;
    _framePointer = _videoFramePointer;

    return true;
}

bool CSilentBIC::DecodeFrame()
{
    bool ret = false;

    if (_pInputBuffer != nullptr && _framePointer >= 0 && _framePointer < _inputBufferLength)
    {
        int chunkSize = *reinterpret_cast<int32_t*>(_pInputBuffer + _framePointer);
        int end = ProcessBICFrame(_framePointer + 4, chunkSize);

        if (end > _framePointer)
        {
            _framePointer += chunkSize + 4;
            ret = true;
        }
    }

    return ret;
}