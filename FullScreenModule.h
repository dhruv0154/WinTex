#pragma once

#include "ModuleBase.h"
#include <unordered_map>
#include "Texture.h"
#include "Structs.h"
#include <cstdint>

class CFullScreenModule : public CModuleBase
{
public:
	CFullScreenModule(ModuleType type);
	virtual ~CFullScreenModule();

	virtual void Resize(int width, int height);

	virtual void Dispose();
	virtual void Render();

protected:
	virtual void Initialize();

	uint8_t* _screen;

	float _top;
	float _left;
	float _bottom;
	float _right;
	float _scale;

	int _originalPalette[256];
	int _palette[256];
	void ReadPalette(uint8_t* pPalette, int startColour = 0, int colourCount = 256);
	std::unordered_map<int, uint8_t*> _files;

	uint8_t* _data;

	ID3D11Buffer* _vertexBuffer;
	CTexture _texture;

	ID3D11Buffer* _iconVertexBuffer;
	CTexture _iconTexture;

	void UpdateTexture();

	void ClearArea(int x1, int y1, int x2, int y2);

	bool _inputEnabled;

	void RenderItem(int entry, int offset_x, int offset_y, int x1 = -1, int x2 = -1, int y1 = -1, int y2 = -1, int transparent = -1);
	Rect RenderItem(uint8_t* data, int offset_x, int offset_y, int x1 = -1, int x2 = -1, int y1 = -1, int y2 = -1, int transparent = -1);
	void RenderItemOffset(uint8_t* data, int srcOffsetX, int srcOffsetY, int dstOffsetX, int dstOffsetY, int w, int h);
	void RenderRaw(int entry, int offset_x, int offset_y, int width, int height);
	void RenderRaw(uint8_t* data, int offset_x, int offset_y, int width, int height);
	void DrawRectangle(int x1, int y1, int x2, int y2, uint8_t colour);
	void Fill(int x1, int y1, int x2, int y2, uint8_t colour);
	void ReplaceColour(int x1, int y1, int x2, int y2, uint8_t src, uint8_t dst);

	int _currentPage;
	int _currentFrame;
	uint64_t _frameTime;

	void FadeOut(int from, int to, int lowFrame, int highFrame);
	void FadeIn(int from, int to, int lowFrame, int highFrame);

	virtual void OnAction(int action) { };

	virtual void CheckKeyAction(int key, ControlCoordinates coordinates[], int count);
	virtual void CheckMouseAction(ControlCoordinates coordinates[], int count);

	// Input related
	virtual void Back();
};