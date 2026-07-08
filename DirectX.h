#pragma once

#include <cstdint.h>
#include <vector>
#include <string>
#include <list>
#include "DXAdapter.h"

#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

struct ID3D11Buffer {
    GLuint glId = 0;
    uint32_t byteWidth = 0;
    uint32_t bindFlags = 0;
    std::vector<uint8_t> cpuData;
};

struct ID3D11Texture2D {
    GLuint glId = 0;
    int width, height, format;
};

struct ID3D11ShaderResourceView {
	GLuint glId = 0;
};

struct ID3D11Device {};
struct ID3D11DeviceContext {};

struct ID3D11Resource { 
	GLuint glId = 0; 
};

struct D3D11_BUFFER_DESC { 
	uint32_t ByteWidth; 
	uint32_t BindFlags; uint32_t Usage; 
};

struct D3D11_SUBRESOURCE_DATA { 
	const void* pSysMem; 
};

struct D3D11_MAPPED_SUBRESOURCE { 
	void* pData; 
};

struct D3D11_TEXTURE2D_DESC { 
	int Width; 
	int Height; 
	int Format; 
};

struct D3D11_SHADER_RESOURCE_VIEW_DESC {};

struct D3D11_VIEWPORT { 
	float TopLeftX; 
	float TopLeftY; 
	float Width; 
	float Height; 
	float MinDepth; 
	float MaxDepth; 
};

struct D3D11_RECT { 
	int left; 
	int top; 
	int right; 
	int bottom; 
};

typedef int D3D11_MAP;
typedef int DXGI_FORMAT;
typedef int D3D11_PRIMITIVE_TOPOLOGY;

#define D3D11_BIND_CONSTANT_BUFFER 0x1
#define D3D11_BIND_VERTEX_BUFFER 0x2
#define D3D11_BIND_INDEX_BUFFER 0x4
#define D3D11_USAGE_DYNAMIC 1
#define D3D11_USAGE_DEFAULT 0
#define DXGI_FORMAT_B8G8R8A8_UNORM 87
#define D3D11_MAP_WRITE_DISCARD 4

class CDirectX : public CDXBase
{
public:
    CDirectX();
    ~CDirectX();

    bool Init(void* hWnd, int width, int height, bool windowed, bool anisotropicFilter, int bufferCount = 1);
    void Dispose();

    void SetFullScreen(bool fullScreen);

    void Clear(float red = 0.0f, float green = 0.0f, float blue = 0.0f);
    void Present(uint32_t syncInterval = 0, uint32_t flags = 0);

    int CreateBuffer(D3D11_BUFFER_DESC* pDesc, D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Buffer** ppBuffer, const char* name = nullptr);
    int Map(ID3D11Resource* pResource, uint32_t subResource, D3D11_MAP mapType, uint32_t mapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource);
    void Unmap(ID3D11Resource* pResource, uint32_t subResource);

    ID3D11Device* GetDevice();
    ID3D11DeviceContext* GetDeviceContext();

    int CreateTexture2D(D3D11_TEXTURE2D_DESC* pDesc, D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D, const char* name = nullptr);
    int CreateShaderResourceView(ID3D11Resource* pResource, D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppSRView, const char* name = nullptr);

    void SetVertexBuffers(uint32_t StartSlot, uint32_t NumBuffers, ID3D11Buffer** ppVertexBuffers, const uint32_t* pStrides, const uint32_t* pOffsets);
    void SetIndexBuffer(ID3D11Buffer* pIndexBuffer, DXGI_FORMAT Format, uint32_t Offset);
    void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology);
    void Draw(uint32_t VertexCount, uint32_t StartVertexLocation);
    void DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation);

    void SetShaderResources(uint32_t StartSlot, uint32_t NumViews, ID3D11ShaderResourceView** ppShaderResourceViews);

    void EnableZBuffer();
    void DisableZBuffer();

    int GetWidth() { return _width; }
    int GetHeight() { return _height; }

    void Resize(int width, int height);

    CDXAdapter* GetAdapter();

    const char* ErrorMessage = nullptr;

    void SelectSampler(bool anisotropic);
    void SetViewport(D3D11_VIEWPORT viewport);
    void SetScissorRect(D3D11_RECT rect);

protected:
    int _width;
    int _height;

    ID3D11Device* _dev;
    ID3D11DeviceContext* _devCon;

    std::list<CDXAdapter*> _adapters;
};
