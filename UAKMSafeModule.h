#pragma once

#include "ModuleBase.h"
#include <unordered_map>
#include <cstdint>
#include "Texture.h"
#include "AmbientAudio.h"

class CUAKMSafeModule : public CModuleBase
{
public:
	CUAKMSafeModule(int parameter, bool alternatePalette);
	virtual ~CUAKMSafeModule();

	virtual void Resize(int width, int height);

	virtual void Dispose();
	virtual void Render();
	virtual void KeyDown(int key, int lParam);

protected:
	virtual void Initialize();

	int _parameter;
	bool _alternatePalette;

	uint8_t* _screen;

	void SetCursorArea(int x1, int y1, int x2, int y2);
	void ClipMouse(bool move);
	float _left;
	float _top;
	float _right;
	float _bottom;
	float _scale;

	int _palette[256];
	std::unordered_map<int, uint8_t*> _safeImageOffsets;
	std::unordered_map<int, uint8_t*> _safeSoundOffsets;

	uint8_t* _pImages;
	uint8_t* _pSounds;

	ID3D11Buffer* _vertexBuffer;
	CTexture _texture;
	bool _textureDirty;
	uint64_t _frameTimes[14];

	ID3D11Buffer* _handVertexBuffer;
	CTexture _handTexture;

	void UpdateTexture();
	void PartialRender(int entry, int offsetX, int offsetY, bool updateTexture);

	bool _ready;
	uint64_t _frameDelay;
	uint64_t _frameTime;
	int _startupFrame;
	int _keyDown[14];
	uint8_t _enteredCode[8];
	int _keyPos;

	void Start();
	void Enter();
	void Number(int number);
	void Exit();

	CAmbientAudio _sound;

	void Press(int key, int sound);
	bool _codeCorrect;

	bool _flashingLightOn;
	int _rollingLightPosition;
	uint64_t _rollingLightTime;

	int _openSafeSequence;

	// Input related
	virtual void BeginAction();
	virtual void Back();
};
