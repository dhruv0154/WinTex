#include "SimpleAnimPlayer.h"
#include "Utilities.h"

CSimpleAnimPlayer::CSimpleAnimPlayer()
{
    _currentFrame = 0;
    _animationFrames = 0;
    _animationWidth = 0;
    _animationHeight = 0;
    _firstFrameFull = false;
    _animationPointer = nullptr;
    _dataPointer = nullptr;
}

CSimpleAnimPlayer::~CSimpleAnimPlayer()
{
}

void CSimpleAnimPlayer::Init(uint8_t* pData)
{
    _lock.Lock();

    _currentFrame = 0;
    _animationFrames = GetInt(pData, 0, 2);
    _animationWidth = GetInt(pData, 2, 2);
    _animationHeight = GetInt(pData, 4, 2);
    _firstFrameFull = (GetInt(pData, 7, 1) != 0);
    _animationPointer = pData + 8;
    _dataPointer = pData;

    _lock.Release();
}

void CSimpleAnimPlayer::Merge(uint8_t* pData)
{
    _lock.Lock();

	// Assuming identical animations, extract frame count and update animation pointer
	_animationFrames = GetInt(pData, 0, 2);
	_animationPointer = pData + (_animationPointer - _dataPointer);
	_dataPointer = pData;

    _lock.Release();
}

bool CSimpleAnimPlayer::DecodeFrame(uint8_t* pScreen, int ox, int oy, int w)
{
    _lock.Lock();

    bool success = false;

    // Video
    if (_animationPointer != nullptr)
    {
        int chunkSize = GetInt(_animationPointer, 0, 2);
        _animationPointer += 2;
        uint8_t* nextFrame = _animationPointer + chunkSize;

        if (_currentFrame == 0 && _firstFrameFull)
        {
            // Initial keyframe
            for (int y = 0; y < _animationHeight; y++)
            {
                for (int x = 0; x < _animationWidth; x++)
                {
                    pScreen[(oy + y) * w + ox + x] = *(_animationPointer++);
                }
            }

            success = true;
        }
        else if (_currentFrame < _animationFrames)
        {
            // Delta-encoded frame
            int x = 0;
            int y = 0;
            while (chunkSize > 0)
            {
                int b = *(_animationPointer++);
                chunkSize--;
                
                if ((b & 0x80) != 0)
                {
                    // High bit set
                    x += (b & 0x7f);
                    while (x >= _animationWidth)
                    {
                        y++;
                        x -= _animationWidth;
                    }
                }
                else
                {
                    // High bit clear
                    for (int i = 0; i < b; i++)
                    {
                        pScreen[(oy + y) * w + ox + x++] = *(_animationPointer++);
                        if (x >= _animationWidth)
                        {
                            x = 0;
                            y++;
                        }

                        chunkSize--;
                    }
                }
            }

            _animationPointer = nextFrame;
            _currentFrame++;

            if (_currentFrame == _animationFrames)
            {
                _animationPointer = nullptr;
            }

            success = true;
        }
    }

    _lock.Release();

    return success;
}