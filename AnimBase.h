#pragma once

#include "Globals.h"
#include "DXSound.h"
#include "LZ.h"
#include "DirectX.h"
#include "Texture.h"
#include "Mutex.h"
#include <SDL2/SDL.h>
#include <cstdint>
#include <list>

class CAnimBase : public CDXBase
{
public:
	CAnimBase();
	~CAnimBase();

	virtual bool Init(uint8_t* pData, int length);
	bool Init(BinaryData bd);

	void Render();
	virtual bool Update();

	virtual bool IsWave() { return false; }
	virtual bool HasVideo() { return true; }

	virtual bool ShouldClearDXBuffer() { return true; }

	virtual bool IsDone() { return _done; }
	virtual void Skip();
	int Frame() { return _frame; }

	int Width() { return _width; }
	int Height() { return _height; }

	void Resize(int width, int height);

protected:
	std::list<Buffer> _audioBuffers;
	uint8_t* _pInputBuffer;
	uint8_t* _pVideoOutputBuffer;
	int _inputBufferLength;
	int _videoFramePointer;
	int _audioFramePointer;
	int* _pPalette;

	int _width;
	int _height;
	int _rate;
	int _frameTime;
	int _frame;

	uint64_t _lastFrameUpdate;

	virtual bool DecodeFrame() { return false; }
	int _framePointer;

	virtual void CreateBuffers(int width, int height, int factor = 1);

	ID3D11Buffer* _vertexBuffer;

	CAudioStream* _sourceVoice;
	int _remainingAudioLength;

	int _audioFramesQueued;
	int _audioFramesProcessed;
	int _videoFramesProcessed;

	int _screenWidth;
	int _screenHeight;

	CTexture _texture;

	bool _done;

	uint8_t _colourTranslationTable[64];

	CMutex _lock;
};
