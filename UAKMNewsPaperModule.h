#pragma once

#include "ModuleBase.h"
#include <unordered_map>
#include "Texture.h"
#include "DXButton.h"
#include <cstdint>

class CUAKMNewsPaperModule : public CModuleBase
{
public:
	CUAKMNewsPaperModule();
	virtual ~CUAKMNewsPaperModule();

	virtual void Resize(int width, int height);

	virtual void Dispose();
	virtual void Render();

	static CUAKMNewsPaperModule* pUAKMNPM;

protected:
	virtual void Initialize();

	int _display;
	int _highLight;

	float _left;
	float _right;
	float _top;
	float _bottom;
	float _scale;

	int _palette[256];

	CDXButton* _pBtnResume;
	static void OnResume(void* data);

	class CNewsPaperView
	{
	public:
		CNewsPaperView()
		{
			Data = nullptr;
			Buffer = nullptr;
		}

		~CNewsPaperView()
		{
			if (Data != nullptr)
			{
				delete[] Data;
				Data = nullptr;
			}

			if (Buffer != nullptr)
			{
				Buffer->Release();
				Buffer = nullptr;
			}
		}

		uint8_t* Data = nullptr;
		int Width = 0;
		int Height = 0;
		CTexture Texture;
		ID3D11Buffer* Buffer = nullptr;
	};

	std::unordered_map<int, CNewsPaperView*> _newsPaper;

	void UpdateTexture(CNewsPaperView* np);

	// Input related
	virtual void Cursor(float x, float y, bool relative);
	virtual void BeginAction();
	virtual void Back();
};