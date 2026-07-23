#include "AnimBase.h"
#include "Utilities.h"
#include "Globals.h"
#include "ShaderStructs.h"
#include <cstring>

CAnimBase::CAnimBase()
{
	_pInputBuffer = nullptr;
	_pVideoOutputBuffer = nullptr;
	_inputBufferLength = 0;
	_videoFramePointer = 0;
	_audioFramePointer = 0;
	_pPalette = nullptr;

	_width = 0;
	_height = 0;
	_rate = 0;
	_frameTime = 0;
	_lastFrameUpdate = 0;
	_frame = 0;

	_vertexBuffer = nullptr;

	_sourceVoice = nullptr;
	_remainingAudioLength = 0;

	_audioFramesQueued = 0;
	_audioFramesProcessed = 0;
	_videoFramesProcessed = 0;

	_framePointer = 0;
	_done = false;

	for (int i = 0; i < 64; i++)
	{
		_colourTranslationTable[i] = (uint8_t)(4.04762 * i);
	}
}

CAnimBase::~CAnimBase()
{
	if (_sourceVoice != nullptr)
	{
		_sourceVoice->Stop();
		delete _sourceVoice;
		_sourceVoice = nullptr;
	}

	if (_pInputBuffer != nullptr)
	{
		delete[] _pInputBuffer;
		_pInputBuffer = nullptr;
	}

	if (_pPalette != nullptr)
	{
		delete[] _pPalette;
		_pPalette = nullptr;
	}

	if (_pVideoOutputBuffer != nullptr)
	{
		delete[] _pVideoOutputBuffer;
		_pVideoOutputBuffer = nullptr;
	}

	if (_vertexBuffer != nullptr)
	{
		delete _vertexBuffer;
		_vertexBuffer = nullptr;
	}
}

bool CAnimBase::Init(uint8_t* pData, int length)
{
	_pInputBuffer = pData;
	_inputBufferLength = length;
	_screenWidth = dx.GetWidth();
	_screenHeight = dx.GetHeight();
	_pPalette = new int[256];
	memset(_pPalette, 0, sizeof(int) * 256);

	return true;
}

void CAnimBase::CreateBuffers(int width, int height, int factor)
{
	if (width > 0 && height > 0)
	{
		int bufferHeight = (((height + 7) / 8) * 8);

		_pVideoOutputBuffer = new uint8_t[width * bufferHeight];
		memset(_pVideoOutputBuffer, 0, width * bufferHeight);

		// Setup vertex buffer, keep aspect ratio
		float sx = (float)_screenWidth / (float)(width * factor);
		float sy = (float)_screenHeight / (float)(height * factor);
		float scale = std::min(sx, sy);
		float sw = width * scale;
		float sh = height * scale;
		float ox = (_screenWidth - sw);
		float oy = (_screenHeight - sh);

		float left, right, top, bottom;

		left = std::floor((float)(ox / 2.0f)) + 0.5f;
		right = std::floor(left + sw) + 0.5f;
		top = std::floor((float)(-oy / 2.0f)) + 0.5f;
		bottom = std::floor(top - sh) + 0.5f;

		TEXTURED_VERTEX* vertices = new TEXTURED_VERTEX[4];
		if (vertices != nullptr)
		{
			vertices[0].position = float3{right, top, 0.0f};
			vertices[0].texture = float2{1.0f, 0.0f};

			vertices[1].position = float3{right, bottom, 0.0f};
			vertices[1].texture = float2{1.0f, 1.0f};

			vertices[2].position = float3{left, top, 0.0f};
			vertices[2].texture = float2{0.0f, 0.0f};

			vertices[3].position = float3{left, bottom, 0.0f};
			vertices[3].texture = float2{0.0f, 1.0f};

			D3D11_BUFFER_DESC vertexBufferDesc;
			vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			vertexBufferDesc.ByteWidth = sizeof(TEXTURED_VERTEX) * 4;
			vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

			D3D11_SUBRESOURCE_DATA vertexData;
			vertexData.pSysMem = vertices;

			dx.CreateBuffer(&vertexBufferDesc, &vertexData, &_vertexBuffer, "AnimBaseBuffer");

			delete[] vertices;
		}
	}
}

void CAnimBase::Render()
{
	if (_vertexBuffer != nullptr)
	{
		dx.DisableZBuffer();

		uint32_t stride = sizeof(TEXTURED_VERTEX);
		uint32_t offset = 0;
		dx.SetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
		dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		ID3D11ShaderResourceView* pRV = _texture.GetTextureRV();

		float16 wm = {{
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
		}};

		CShaders::SelectOrthoShader();
		CConstantBuffers::SetWorld(dx, &wm);

		dx.SetShaderResources(0, 1, &pRV);
		dx.Draw(4, 0);

		dx.EnableZBuffer();
	}
}

bool CAnimBase::Init(BinaryData bd)
{
	return Init(bd.Data, bd.Length);
}

bool CAnimBase::Update()
{
	bool updated = false;

	if (!_done)
	{
		if (_sourceVoice != nullptr && _audioBuffers.size() > 0) 
		{
			if (_lock.Lock())
			{
				while (_audioBuffers.size() > 0)
				{
					Buffer ab = _audioBuffers.front();
					_audioBuffers.pop_back();
					_sourceVoice->SubmitBuffer(ab.pData, ab.Size);
				}
				_lock.Release();
			}
		}

		uint64_t tick = SDL_GetTicks64();
		uint64_t diff = tick - _lastFrameUpdate;

		if (diff >= _frameTime)
		{
			//wchar_t buffer[10];
			//OutputDebugString(L"Decoding video frame ");
			//OutputDebugString(_itow(_frame, buffer, 10));
			//OutputDebugString(L" @ ");
			//OutputDebugString(_itow(_videoFramePointer, buffer, 16));
			//OutputDebugString(L"\r\n");
			//if (DecodeVideoFrame())
			//if (_lock.Lock())
			{
				if (DecodeFrame())
				{
					_frame++;

					// Replace texture if new video frame is required (frame time has lapsed, video frame exists)
					ID3D11Texture2D* pTex = _texture.GetTexture();
					if (pTex != nullptr)
					{
						D3D11_MAPPED_SUBRESOURCE subRes;
						if (dx.Map(pTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes) == 0)
						{
							int* pScr = (int*)subRes.pData;
							for (int y = 0; y < _height; y++)
							{
								for (int x = 0; x < _width; x++)
								{
									pScr[y * subRes.RowPitch / 4 + x] = _pPalette[_pVideoOutputBuffer[y * _width + x]];
								}
							}

							dx.Unmap(pTex, 0);
						}
						else
						{
							int debug = 0;
						}
					}
				}

				if (_lastFrameUpdate == 0) _lastFrameUpdate = diff - _frameTime;
				_lastFrameUpdate += _frameTime;

				bool audioFinished = false;

				if (_sourceVoice != nullptr)
				{
					audioFinished = (_sourceVoice->GetPendingBufferCount() == 0);
				}

				if (_framePointer >= _inputBufferLength && _audioFramesProcessed == _audioFramesQueued)
				{
					_done = true;
				}

				//if ((_framePointer == 0 || _framePointer >= _inputBufferLength) && _audioFramesProcessed == _audioFramesQueued)
				//{
				//
				//	if ((_videoFramePointer > 0 && _videoFramePointer < _inputBufferLength) || _audioFramePointer > 0 && _audioFramePointer < _inputBufferLength)
				//	{
				//		return;
				//	}
				//	_done = true;
				//}

				//_lock.Release();
			}

			updated = true;
		}
	}

	return updated;
}

void CAnimBase::Skip()
{
	// Any audio must be stopped
	if (_sourceVoice != nullptr)
	{
		_sourceVoice->Stop();
		_audioFramesProcessed = _audioFramesQueued;
	}

	_audioBuffers.clear();

	_done = true;
}

void CAnimBase::Resize(int width, int height)
{
	float factor = 1.0f;

	if (width > 0 && height > 0)
	{
		if (_vertexBuffer != nullptr)
		{
			delete _vertexBuffer;
			_vertexBuffer = nullptr;
		}

		_screenWidth = width;
		_screenHeight = height;

		// Recreate vertex buffer
		float sx = (float)_screenWidth / (float)(_width * factor);
		float sy = (float)_screenHeight / (float)(_height * factor);
		float scale = std::min(sx, sy);
		float sw = _width * scale;
		float sh = _height * scale;
		float ox = (_screenWidth - sw);
		float oy = (_screenHeight - sh);

		float left, right, top, bottom;

		left = floor((float)(ox / 2.0f)) + 0.5f;
		right = floor(left + sw) + 0.5f;
		top = floor((float)(-oy / 2.0f)) + 0.5f;
		bottom = floor(top - sh) + 0.5f;

		TEXTURED_VERTEX* vertices = new TEXTURED_VERTEX[4];
		if (vertices != nullptr)
		{
			vertices[0].position = float3(right, top, 0.0f);
			vertices[0].texture = float2(1.0f, 0.0f);

			vertices[1].position = float3(right, bottom, 0.0f);
			vertices[1].texture = float2(1.0f, 1.0f);

			vertices[2].position = float3(left, top, 0.0f);
			vertices[2].texture = float2(0.0f, 0.0f);

			vertices[3].position = float3(left, bottom, 0.0f);
			vertices[3].texture = float2(0.0f, 1.0f);

			D3D11_BUFFER_DESC vertexBufferDesc;
			vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			vertexBufferDesc.ByteWidth = sizeof(TEXTURED_VERTEX) * 4;
			vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

			D3D11_SUBRESOURCE_DATA vertexData;
			vertexData.pSysMem = vertices;

			dx.CreateBuffer(&vertexBufferDesc, &vertexData, &_vertexBuffer, "AnimBaseBuffer");

			delete[] vertices;
		}
	}
}
