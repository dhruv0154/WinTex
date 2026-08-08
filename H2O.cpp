#include "H2O.h"
#include "Utilities.h"
#include "MediaIdentifiers.h"
#include "DXSound.h"
#include <cstring>
#include <algorithm>
#include <iostream>

#define H2O_MAX_AUDIO_BUFFERS   20
#define H2O_AUDIO_BUFFER_SIZE   20000

static void WriteAudioSample(uint8_t* output, int& outputOffset, int value, int size)
{
    output[outputOffset] = static_cast<uint8_t>(value & 0xff);
    if (size == 2)
    {
        output[outputOffset + 1] = static_cast<uint8_t>((value >> 8) & 0xff);
    }
    outputOffset += size;
}

static int ReadBits(uint8_t* data, int bitsToRead, int& bitOffset, bool& commandFlag)
{
    int byteOffset = bitOffset >> 3;
    int bitsUsed = bitOffset & 7;
    int value = GetInt(data, byteOffset, 3) >> bitsUsed;
    int mask = (1 << bitsToRead) - 1;
    int signMask = 1 << (bitsToRead - 1);
    int signExtension = -1 << bitsToRead;

    int maskedValue = (value & mask);
    commandFlag = (maskedValue == signMask);

    int ret = maskedValue;
    if ((value & signMask) != 0)
    {
        ret |= signExtension;
    }

    bitOffset += bitsToRead;
    return ret;
}

CH2O::CH2O(int factor)
{
    _factor = factor;
    _channels = 0;
    _depth = 0;
    _remainingLength = 0;
    _audioCompressed = false;

    if (_lock.Lock())
    {
        _ppAudioOutputBuffers = new uint8_t*[H2O_MAX_AUDIO_BUFFERS];
        for (int i = 0; i < H2O_MAX_AUDIO_BUFFERS; i++)
        {
            _ppAudioOutputBuffers[i] = new uint8_t[H2O_AUDIO_BUFFER_SIZE];
            std::memset(_ppAudioOutputBuffers[i], 0, H2O_AUDIO_BUFFER_SIZE);
        }
        _audioOutputBufferIndex = 0;
        _lock.Release();
    }

    _minimumBitCount = 0;
    _pDecodingTable = new int[65536];
    std::memset(_pDecodingTable, 0, sizeof(int) * 65536);
    
    _pDecodingBuffer = new uint8_t[1048576];
    std::memset(_pDecodingBuffer, 0, 1048576);
    
    _decodedSize = 0;
    _startAudioOnFrame = -1;

    _configuredOutputBuffer = nullptr;
    _renderWidth = 0;
    _renderHeight = 0;
    _offsetX = 0;
    _offsetY = 0;
    _minColAllowChange = 0;
    _maxColAllowChange = 256;

    _h2oWidth = 0;
    _h2oHeight = 0;
}

CH2O::~CH2O()
{
    if (_ppAudioOutputBuffers != nullptr)
    {
        for (int i = 0; i < H2O_MAX_AUDIO_BUFFERS; i++)
        {
            if (_ppAudioOutputBuffers[i] != nullptr)
            {
                delete[] _ppAudioOutputBuffers[i];
            }
        }
        delete[] _ppAudioOutputBuffers;
        _ppAudioOutputBuffers = nullptr;
    }

    if (_pDecodingTable != nullptr)
    {
        delete[] _pDecodingTable;
        _pDecodingTable = nullptr;
    }

    if (_pDecodingBuffer != nullptr)
    {
        delete[] _pDecodingBuffer;
        _pDecodingBuffer = nullptr;
    }
}

bool CH2O::Init(uint8_t* pData, int length)
{
    bool ret = CAnimBase::Init(pData, length);

    // Validate that the file is a H2O
    if (ret && GetInt(pData, 0, 4) == H2O)
    {
        _renderWidth = _width = _h2oWidth = GetInt(pData, 4, 2);
        _renderHeight = _height = _h2oHeight = GetInt(pData, 8, 2);
        _rate = GetInt(pData, 16, 2);
        _frameTime = _rate;

        CreateBuffers(_width, _height, _factor);
        _configuredOutputBuffer = _pVideoOutputBuffer;
        _texture.Init(_width, _height);

        // Find video and audio pointers
        _framePointer = 0x40;
        if (_width > 0 && _height > 0)
        {
            _videoFramePointer = 0x40;
        }

        _audioFramePointer = 0x40;
        _audioCompressed = (GetInt(_pInputBuffer, 0x2c, 4) != 0);
        _configuredPalette = _pPalette;
    }

    return ret;
}

int CH2O::DecodeH2OAudio(uint8_t* source, uint8_t* destination, int chunkLength)
{
    int outputOffsetStart = 0;
    int outputOffset = 0;
    int inputOffset = 0;

    int readBitsCount = inputOffset * 8;
    int endOffset = chunkLength;
    bool commandFlag = false;
    
    int flags = ReadBits(source, 4, readBitsCount, commandFlag);
    int channelCount = ((flags & 2) != 0) ? 2 : 1;
    int bytesPerSample = ((flags & 4) != 0) ? 1 : 2;
    int bitsPerSample = bytesPerSample * 8;

    int audioAmplifier = ReadBits(source, 4, readBitsCount, commandFlag) & 15;

    int channelBitsPerSample[2];
    int channelPreviousSamples[2];

    channelBitsPerSample[0] = 1 + (ReadBits(source, 4, readBitsCount, commandFlag) & 15);
    channelPreviousSamples[0] = ReadBits(source, bitsPerSample, readBitsCount, commandFlag);
    WriteAudioSample(destination, outputOffset, channelPreviousSamples[0] << audioAmplifier, bytesPerSample);

    if (channelCount == 2)
    {
        // Read second channel data
        channelBitsPerSample[1] = 1 + (ReadBits(source, 4, readBitsCount, commandFlag) & 15);
        channelPreviousSamples[1] = ReadBits(source, bitsPerSample, readBitsCount, commandFlag);
        WriteAudioSample(destination, outputOffset, channelPreviousSamples[1] << audioAmplifier, bytesPerSample);
    }

    int channelRepeatCounters[2] = { 0, 0 };
    int channel = 0;

    while ((readBitsCount >> 3) < endOffset)
    {
        bool fullBreak = false;
        int sampleValue = 0;

        if (channelRepeatCounters[channel] == 0)
        {
            while (true)
            {
                // Read new sample
                sampleValue = ReadBits(source, channelBitsPerSample[channel], readBitsCount, commandFlag);
                if (channelBitsPerSample[channel] == bitsPerSample)
                {
                    if (bytesPerSample == 1)
                    {
                        sampleValue &= 0xff;
                    }
                }
                else
                {
                    sampleValue += channelPreviousSamples[channel];
                }

                if (commandFlag)
                {
                    flags = ReadBits(source, 2, readBitsCount, commandFlag) & 3;
                    if (flags == 0)
                    {
                        int innertemp = ReadBits(source, 4, readBitsCount, commandFlag) & 15;
                        if (innertemp == 0)
                        {
                            return outputOffset - outputOffsetStart;
                        }
                        else if (innertemp < 2)
                        {
                            channelRepeatCounters[channel] = ReadBits(source, 8, readBitsCount, commandFlag) & 0xff;
                            fullBreak = true;
                            break;
                        }
                        else
                        {
                            channelBitsPerSample[channel] = innertemp + 1;
                        }
                    }
                    else if (flags == 1)
                    {
                        // New channel bit size
                        channelBitsPerSample[channel] = std::min(16, channelBitsPerSample[channel] + 1);
                    }
                    else if (flags == 2)
                    {
                        // New channel bit size
                        channelBitsPerSample[channel] = std::max(2, channelBitsPerSample[channel] - 1);
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            // Re-use previous sample value
            channelRepeatCounters[channel]--;
            sampleValue = channelPreviousSamples[channel];
        }

        if (!fullBreak)
        {
            channelPreviousSamples[channel] = sampleValue;
            WriteAudioSample(destination, outputOffset, sampleValue << audioAmplifier, bytesPerSample);
            
            channel++;
            if (channel >= channelCount)
            {
                channel = 0;
            }
        }
    }

    return outputOffset - outputOffsetStart;
}

bool CH2O::ProcessFrame(int& offset, bool video)
{
    if (offset > 0 && offset < _inputBufferLength)
    {
        bool initialAudioBuffer = false;

        int frameSize = GetInt(_pInputBuffer, offset, 3);
        int frameFlags = _pInputBuffer[offset + 3];
        int chunkOffset = offset + 4;

        if ((frameFlags & 0x80) != 0)
        {
            int chunkSize = GetInt(_pInputBuffer, chunkOffset, 4);
            if (video)
            {
                int newWidth = GetInt(_pInputBuffer, chunkOffset + 4, 4);
                int newHeight = GetInt(_pInputBuffer, chunkOffset + 8, 4);

                if (newWidth != _h2oWidth || newHeight != _h2oHeight)
                {
                    if (newWidth <= _renderWidth && newHeight <= _renderHeight)
                    {
                        if (newWidth < _h2oWidth)
                        {
                            for (int y = 0; y < _renderHeight; y++)
                            {
                                for (int x = 0; x < (_h2oWidth - newWidth) / 2; x++)
                                {
                                    _configuredOutputBuffer[y * _renderWidth + x] = 0;
                                    _configuredOutputBuffer[(y + 1) * _renderWidth - 1 - x] = 0;
                                }
                            }
                        }

                        if (newHeight < _h2oHeight)
                        {
                            int clearSize = ((_h2oHeight - newHeight) / 2) * _renderWidth;
                            std::memset(_configuredOutputBuffer, 0, clearSize);
                            std::memset(_configuredOutputBuffer + _renderHeight * _renderWidth - clearSize, 0, clearSize);
                        }

                        _h2oWidth = newWidth;
                        _h2oHeight = newHeight;
                    }
                }
            }

            chunkOffset += chunkSize + 4;
        }

        if ((frameFlags & 0x40) != 0)
        {
            int chunkSize = GetInt(_pInputBuffer, chunkOffset, 4);
            if (video)
            {
                int tag40Offset = chunkOffset + 4;
                int t40v1 = _pInputBuffer[tag40Offset++] & 0xff;
                int t40v2 = _pInputBuffer[tag40Offset++] & 0xff;
                _minimumBitCount = t40v1;
                int t40Counter = 0;

                while (t40v1 <= t40v2)
                {
                    _pDecodingTable[t40v1 * 3] = t40Counter;
                    int blockCount = GetInt(_pInputBuffer, tag40Offset, 2) & 0xffff;
                    t40Counter += blockCount;
                    _pDecodingTable[t40v1 * 3 + 1] = t40Counter - 1;
                    _pDecodingTable[t40v1 * 3 + 2] = tag40Offset + 2;

                    t40v1++;
                    tag40Offset += blockCount * 2 + 2;
                    t40Counter <<= 1;
                }
            }
            chunkOffset += chunkSize + 4;
        }

        if ((frameFlags & 0x01) != 0)
        {
            int chunkSize = GetInt(_pInputBuffer, chunkOffset, 4);
            if (video)
            {
                int paletteOffset = chunkOffset + 4;
                int blockCount = GetInt(_pInputBuffer, paletteOffset, 2);
                paletteOffset += 2;
                int currentIndex = 0;

                for (int b = 0; b < blockCount; b++)
                {
                    currentIndex += _pInputBuffer[paletteOffset++];
                    int colourCount = _pInputBuffer[paletteOffset++];
                    if (colourCount == 0)
                    {
                        colourCount = 256;
                    }

                    for (int c = 0; c < colourCount; c++)
                    {
                        int r = _colourTranslationTable[_pInputBuffer[paletteOffset++]];
                        int g = _colourTranslationTable[_pInputBuffer[paletteOffset++]];
                        int bVal = _colourTranslationTable[_pInputBuffer[paletteOffset++]];

                        int col = 0xff000000 | bVal | (g << 8) | (r << 16);
                        
                        if (_factor == 2 && c < 8)
                        {
                            col = 0xff000000;
                        }

                        if (c >= _minColAllowChange && c < _maxColAllowChange)
                        {
                            _configuredPalette[currentIndex] = col;
                        }
                        currentIndex++;
                    }
                }
            }
            chunkOffset += chunkSize + 4;
        }

        // Flag 0x02: Algorithmic Video Unpacking and Pattern Rendering
        if ((frameFlags & 0x02) != 0)
        {
            int chunkSize = GetInt(_pInputBuffer, chunkOffset, 4);
            if (video)
            {
                int videoOffset = chunkOffset + 4;
                Unpack(videoOffset, chunkSize);

                _qw = _h2oWidth / 4;
                int qh = _h2oHeight / 4;
                _inputOffset = 0;
                _x = (_renderWidth - _h2oWidth) / 2;
                _y = (_renderHeight - _h2oHeight) / 2;
                _remainingX = _qw;
                _remainingY = qh;

                while (_inputOffset < _decodedSize && _remainingY > 0)
                {
                    int val = (GetInt(_pDecodingBuffer, _inputOffset, 2)) & 0xffff;
                    _inputOffset += 2;

                    if (val < 0x4000)
                    {
                        SkipOrFill(val);
                    }
                    else if (val < 0x5000)
                    {
                        PatternFill(val);
                    }
                    else if (val < 0x6000)
                    {
                        PatternCopy(val);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            chunkOffset += chunkSize + 4;
        }

        // Flag 0x04: DPCM Audio Chunk Processing
        if ((frameFlags & 0x04) != 0)
        {
            if (!video)
            {
                int chunkSize = GetInt(_pInputBuffer, chunkOffset, 4);
                int dataOffset = chunkOffset + 4;
                int outputOffset = 0;
                int riffOffset = 0;

                if (GetInt(_pInputBuffer, dataOffset, 4) == RIFF)
                {
                    riffOffset = dataOffset;
                    _channels = GetInt(_pInputBuffer, dataOffset + 0x16, 2);
                    _depth = GetInt(_pInputBuffer, dataOffset + 0x22, 2);
                    _remainingLength = GetInt(_pInputBuffer, dataOffset + 0x28, 4);

                    dataOffset += 0x2c;
                    chunkSize -= 0x2c;
                    initialAudioBuffer = true;
                    _startAudioOnFrame = (_width > 0 && _height > 0) ? _frame + 4 : 0;
                }

                if (_audioCompressed)
                {
                    outputOffset += DecodeH2OAudio(_pInputBuffer + dataOffset, _ppAudioOutputBuffers[_audioOutputBufferIndex] + outputOffset, chunkSize);
                }
                else
                {
                    std::memcpy(_ppAudioOutputBuffers[_audioOutputBufferIndex], _pInputBuffer + dataOffset, chunkSize);
                    outputOffset += chunkSize;
                }

                Buffer ab;
                ab.Frame = _frame;
                ab.Size = outputOffset;
                ab.pData = _ppAudioOutputBuffers[_audioOutputBufferIndex];

                if (_audioBuffers.size() >= H2O_MAX_AUDIO_BUFFERS)
                {
                    std::cerr << "CH2O WARNING! Too many audio buffers in use: " << _audioBuffers.size() << std::endl;
                }

                if (_lock.Lock())
                {
                    _audioBuffers.push_back(ab);
                    _lock.Release();
                }

                _remainingAudioLength -= outputOffset;

                if (_sourceVoice == nullptr)
                {
                    AudioFormat fmt;
                    fmt.channels = _channels;
                    fmt.samplesPerSec = GetInt(_pInputBuffer, riffOffset + 0x18, 4);
                    fmt.bitsPerSample = _depth;

                    _sourceVoice = CDXSound::CreateAudioStream(fmt);
                }

                _audioOutputBufferIndex++;
                if (_audioOutputBufferIndex >= H2O_MAX_AUDIO_BUFFERS)
                {
                    _audioOutputBufferIndex = 0;
                }
            }
        }

        if (_sourceVoice != nullptr && _startAudioOnFrame >= 0 && _frame == _startAudioOnFrame)
        {
            _sourceVoice->Start();
            _startAudioOnFrame = -1;
        }

        offset += frameSize + 4;

        // On initial audio frame, enqueue multiple buffers immediately to prevent audio underruns
        if (!video && initialAudioBuffer && _sourceVoice != nullptr)
        {
            for (int i = 0; i < H2O_MAX_AUDIO_BUFFERS / 2; i++)
            {
                if (!ProcessFrame(offset, video))
                {
                    break;
                }
            }

            if (_lock.Lock())
            {
                auto buffers = _audioBuffers.size();
                for (std::size_t i = 0; i < buffers && i < H2O_MAX_AUDIO_BUFFERS; i++)
                {
                    Buffer ab = _audioBuffers.front();
                    _audioBuffers.pop_front();

                    _sourceVoice->SubmitBuffer(ab.pData, ab.Size);
                    _audioFramesQueued++;
                }
                _lock.Release();
            }
        }

        return offset < _inputBufferLength;
    }

    return false;
}

bool CH2O::DecodeFrame()
{
    bool ret = ProcessFrame(_videoFramePointer, true) | ProcessFrame(_audioFramePointer, false);
    if (!ret)
    {
        _framePointer = _inputBufferLength;
    }
    return ret;
}

void CH2O::Unpack(int offset, int size)
{
    int bitPattern = GetInt(_pInputBuffer, offset, 4);
    offset += 4;
    int availableBitCount = 32;
    int videoEnd = offset + size;
    _decodedSize = 0;

    while (offset <= videoEnd)
    {
        int baseIndex = _minimumBitCount;
        int bits = 0;
        int requiredBitCount = _minimumBitCount;

        if (availableBitCount <= requiredBitCount)
        {
            bits = static_cast<int>((bitPattern >> (32 - availableBitCount)) & (0xffffffff >> (32 - availableBitCount)));
            requiredBitCount -= availableBitCount;

            bitPattern = GetInt(_pInputBuffer, offset, 4);
            offset += 4;
            availableBitCount = 32;
        }

        if (requiredBitCount > 0)
        {
            bits <<= requiredBitCount;
            bits |= static_cast<int>((bitPattern >> (32 - requiredBitCount)) & (0xffffffff >> (32 - requiredBitCount)));
            bitPattern <<= requiredBitCount;
            availableBitCount -= requiredBitCount;
        }

        while (bits > _pDecodingTable[baseIndex * 3 + 1])
        {
            bits <<= 1;
            bits |= ((bitPattern >> 31) & 1);
            bitPattern <<= 1;
            availableBitCount--;

            if (availableBitCount == 0)
            {
                bitPattern = GetInt(_pInputBuffer, offset, 4);
                offset += 4;
                availableBitCount = 32;
            }
            baseIndex++;
        }

        int ix = bits - _pDecodingTable[baseIndex * 3];
        int tagOffset = _pDecodingTable[baseIndex * 3 + 2];

        if (ix >= 0 && tagOffset > 0)
        {
            _pDecodingBuffer[_decodedSize++] = _pInputBuffer[tagOffset + ix * 2];
            _pDecodingBuffer[_decodedSize++] = _pInputBuffer[tagOffset + ix * 2 + 1];
        }
    }
}

void CH2O::SkipOrFill(int val)
{
    int count = (val >> 8) & 0xff;
    if (count >= 59)
    {
        count = 128 << (count - 59);
    }
    else
    {
        count++;
    }

    if ((val & 0xff) == 0)
    {
        while (count > 0)
        {
            if (count < _remainingX)
            {
                _remainingX -= count;
                _x += count * 4;
                break;
            }
            else
            {
                count -= _remainingX;
                NewLine();
            }
        }
    }
    else
    {
        uint8_t set = static_cast<uint8_t>(val & 0xff);
        while (count > 0 && _remainingY > 0)
        {
            int blockCount = std::min(count, _remainingX);

            for (int b = 0; b < blockCount; b++)
            {
                for (int y = 0; y < 4; y++)
                {
                    for (int x = 0; x < 4; x++)
                    {
                        _configuredOutputBuffer[(_y + y + _offsetY) * _renderWidth + _offsetX + _x + b * 4 + x] = set;
                    }
                }
            }

            _x += blockCount * 4;
            _remainingX -= blockCount;
            if (_remainingX <= 0)
            {
                NewLine();
            }
            count -= blockCount;
        }
    }
}

void CH2O::NewLine()
{
    _y += 4;
    _x = (_renderWidth - _h2oWidth) / 2;
    _remainingX = _qw;
    _remainingY--;
}

void CH2O::PatternFill(int val)
{
    int lineCountOrByteCount = (val & 0xfff) + 1;
    while (lineCountOrByteCount > 0 && _remainingY > 0)
    {
        int count = std::min(lineCountOrByteCount, _remainingX);
        _remainingX -= count;
        lineCountOrByteCount -= count;

        while (count > 0)
        {
            int functions = GetInt(_pDecodingBuffer, _inputOffset, 2) & 0xffff;
            int pattern = GetInt(_pDecodingBuffer, _inputOffset + 2, 2) & 0xffff;
            _inputOffset += 4;

            Write(_x, _y, GetPattern(functions & 0xf, pattern));
            Write(_x, _y + 1, GetPattern((functions >> 4) & 0xf, pattern));
            Write(_x, _y + 2, GetPattern((functions >> 8) & 0xf, pattern));
            Write(_x, _y + 3, GetPattern((functions >> 12) & 0xf, pattern));

            count--;
            _x += 4;
        }

        if (_remainingX <= 0)
        {
            NewLine();
        }
    }
}

int CH2O::GetPattern(int function, int input)
{
    int pattern[2] = { input & 0xff, (input >> 8) & 0xff };
    int ret = 0;

    for (int i = 0; i < 4; i++)
    {
        ret <<= 8;
        ret |= pattern[(function >> (3 - i)) & 1];
    }

    return ret;
}

void CH2O::Write(int x, int y, int value)
{
    _configuredOutputBuffer[(y + _offsetY) * _renderWidth + _offsetX + x + 0] = static_cast<uint8_t>(value & 0xff);
    _configuredOutputBuffer[(y + _offsetY) * _renderWidth + _offsetX + x + 1] = static_cast<uint8_t>((value >> 8) & 0xff);
    _configuredOutputBuffer[(y + _offsetY) * _renderWidth + _offsetX + x + 2] = static_cast<uint8_t>((value >> 16) & 0xff);
    _configuredOutputBuffer[(y + _offsetY) * _renderWidth + _offsetX + x + 3] = static_cast<uint8_t>((value >> 24) & 0xff);
}

void CH2O::PatternCopy(int val)
{
    int lineCountOrByteCount = (val & 0xfff) + 1;
    while (lineCountOrByteCount > 0 && _remainingY > 0)
    {
        int count = std::min(lineCountOrByteCount, _remainingX);
        _remainingX -= count;
        lineCountOrByteCount -= count;

        while (count > 0 && _remainingY > 0)
        {
            int functions = GetInt(_pDecodingBuffer, _inputOffset, 2) & 0xffff;
            _inputOffset += 2;

            for (int i = 0; i < 4; i++)
            {
                int function = (functions >> (i * 4)) & 0xf;
                for (int b = 0; b < 4; b++)
                {
                    if ((function & (1 << b)) != 0)
                    {
                        _configuredOutputBuffer[(_y + i + _offsetY) * _renderWidth + _offsetX + _x + b] = _pDecodingBuffer[_inputOffset++];
                    }
                }
            }

            _x += 4;
            if ((_inputOffset & 1) != 0)
            {
                _inputOffset++;
            }
            count--;
        }

        if (_remainingX <= 0)
        {
            NewLine();
        }
    }
}

void CH2O::SetOutputBuffer(uint8_t* pBuffer, int width, int height, int offsetX, int offsetY, int* pPalette, int minColAllowChange, int maxColAllowChange)
{
    _configuredOutputBuffer = pBuffer;
    _renderWidth = width;
    _renderHeight = height;
    _configuredPalette = pPalette;
    _minColAllowChange = minColAllowChange;
    _maxColAllowChange = maxColAllowChange;
    _offsetX = offsetX;
    _offsetY = offsetY;
}
