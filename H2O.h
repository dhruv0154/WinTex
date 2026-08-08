#pragma once

#include "AnimBase.h"

class CH2O : public CAnimBase
{
public:
	CH2O(int factor = 1);
	~CH2O();

	virtual bool Init(uint8_t* pData, int length);
	virtual bool HasVideo() { return (_videoFramePointer != 0); }

	void SetOutputBuffer(uint8_t* pBuffer, int width, int height, int offsetX, int offsetY, int* pPalette, int minColAllowChange, int maxColAllowChange);

protected:
	int _factor;

	virtual bool DecodeFrame();

	virtual int DecodeH2OAudio(uint8_t* source, uint8_t* destination, int chunkLength);

	bool ProcessFrame(int& offset, bool video);

	int _channels;
	int _depth;
	int _remainingLength;
	bool _audioCompressed;

	uint8_t** _ppAudioOutputBuffers;
	int _audioOutputBufferIndex;

	int _minimumBitCount;
	int* _pDecodingTable;
	uint8_t* _pDecodingBuffer;
	int _decodedSize;

	void Unpack(int offset, int size);

	void SkipOrFill(int val);
	void PatternFill(int val);
	int GetPattern(int function, int input);
	void Write(int x, int y, int value);
	void PatternCopy(int val);
	void NewLine();

	int _inputOffset;
	int _x;
	int _y;
	int _remainingX;
	int _remainingY;
	int _qw;

	int _startAudioOnFrame;

	uint8_t* _configuredOutputBuffer;
	int* _configuredPalette;
	int _minColAllowChange;
	int _maxColAllowChange;
	int _renderWidth;
	int _renderHeight;
	int _offsetX;
	int _offsetY;

	int _h2oWidth;
	int _h2oHeight;
};
