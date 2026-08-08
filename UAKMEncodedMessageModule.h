#pragma once

#include "ModuleBase.h"
#include <unordered_map>
#include "Texture.h"
#include "DXButton.h"
#include <cstdint>

class CUAKMEncodedMessageModule : public CModuleBase
{
public:
	CUAKMEncodedMessageModule();
	virtual ~CUAKMEncodedMessageModule();

	virtual void Resize(int width, int height) { }
	virtual void Dispose();
	virtual void Render();
	virtual void KeyDown(int key, int lParam);

	CDXButton* _pBtnResume;
	static void OnResume(void* data);

protected:
	virtual void Initialize();

	static CUAKMEncodedMessageModule* pUAKMEMM;

	int _palette[256];

	ID3D11Buffer* _vertexBuffer;
	CTexture _texture;
	bool _textureDirty;

	float _scale;
	float _left;
	float _top;
	float _width;
	float _height;

	void UpdateTexture();
	uint8_t* _screen;

	std::unordered_map<char, char> _codeMap;

	void RenderChar(int x, int y, char c, bool transparent);
	void RenderText(const char* pText, int yOffset, bool transparent);

	uint8_t* _font;
	std::unordered_map<char, uint8_t*> _fontMap;

	int _col1;
	int _col2;
	int _col3;
	int _col4;

	char* _pSaveMsg;

	ID3D11Buffer* _indicatorVertexBuffer;
	float _indicatorX;
	float _indicatorY;

	int _charPos;

	bool _completed;
	bool CheckCompleted();

	// Input related
	virtual void Cursor(float x, float y, bool relative);
	virtual void BeginAction();
	virtual void Back();
};