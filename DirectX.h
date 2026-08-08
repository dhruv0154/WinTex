#pragma once

#include "DXBase.h"
#include <cstdint>
#include <vector>
#include <string>
#include <list>
#include "DXAdapter.h"
#include "stb_image.h"
#include <iostream>

#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>

#ifndef ULONG
typedef unsigned long ULONG;
#endif

enum ResourceType { RT_Buffer, RT_Texture2D };

enum DXGI_FORMAT { 
    DXGI_FORMAT_UNKNOWN = 0,
    DXGI_FORMAT_B8G8R8A8_UNORM = 87,
    DXGI_FORMAT_R8G8B8A8_UNORM = 28,
    DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
    DXGI_FORMAT_R32G32B32_FLOAT = 6,
    DXGI_FORMAT_R32G32_FLOAT = 16,
    DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
    DXGI_FORMAT_R16_UINT = 57,
    DXGI_FORMAT_R32_UINT = 42
};

enum D3D11_MAP { 
    D3D11_MAP_READ = 1, 
    D3D11_MAP_WRITE = 2, 
    D3D11_MAP_READ_WRITE = 3, 
    D3D11_MAP_WRITE_DISCARD = 4, 
    D3D11_MAP_WRITE_NO_OVERWRITE = 5 
};

enum D3D11_PRIMITIVE_TOPOLOGY { 
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST = 4,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP = 5,
    D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP = 3
};

struct DXGI_SAMPLE_DESC {
    int Count = 1;
    int Quality = 0;
};

struct D3D11_TEXTURE2D_DESC { 
    int Width = 0; 
    int Height = 0; 
    int MipLevels = 1;
    int ArraySize = 1;
    DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN; 
    DXGI_SAMPLE_DESC SampleDesc;
    uint32_t Usage = 0;
    uint32_t BindFlags = 0;
    uint32_t CPUAccessFlags = 0;
    uint32_t MiscFlags = 0; 
};

struct D3D11_BUFFER_DESC { 
    uint32_t ByteWidth; 
    uint32_t BindFlags;
    uint32_t Usage;
    uint32_t CPUAccessFlags = 0;
    uint32_t StructureByteStride = 0;
};

struct D3D11_SUBRESOURCE_DATA { 
    const void* pSysMem; 
    uint32_t SysMemPitch = 0;
    uint32_t SysMemSlicePitch = 0;
};

struct D3D11_MAPPED_SUBRESOURCE { 
    void* pData; 
    uint32_t RowPitch;
    uint32_t DepthPitch;
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

struct ID3D11Resource { 
    GLuint glId = 0;
    std::vector<uint8_t> cpuData;
    uint32_t rowPitch = 0;
    ResourceType type = RT_Buffer;
    virtual ~ID3D11Resource() = default;

    virtual ULONG Release() { 
        delete this; 
        return 0; 
    }
};

struct ID3D11Buffer : public ID3D11Resource {
    uint32_t byteWidth = 0;
    uint32_t bindFlags = 0;
    
    ID3D11Buffer() { type = RT_Buffer; }

    virtual ~ID3D11Buffer() override {
        if (glId != 0) {
            glDeleteBuffers(1, &glId);
            glId = 0;
        }
    }
};

struct ID3D11Texture2D : public ID3D11Resource {
    int width = 0, height = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

    ID3D11Texture2D() { type = RT_Texture2D; }

    void GetDesc(D3D11_TEXTURE2D_DESC* pDesc) {
        if (pDesc) {
            pDesc->Width = width;
            pDesc->Height = height;
            pDesc->Format = format;
        }
    }

    virtual ~ID3D11Texture2D() override {
        if (glId != 0) {
            glDeleteTextures(1, &glId);
            glId = 0;
        }
    }
};

struct ID3D11ShaderResourceView {
    GLuint glId = 0;

    ULONG Release() { 
        delete this; 
        return 0; 
    }
};

struct ID3D11Device {};
struct ID3D11DeviceContext {};

enum D3D11_USAGE { D3D11_USAGE_DEFAULT, D3D11_USAGE_IMMUTABLE, D3D11_USAGE_DYNAMIC, D3D11_USAGE_STAGING };

// D3D11 Constants
#define D3D11_BIND_CONSTANT_BUFFER 0x1
#define D3D11_BIND_VERTEX_BUFFER 0x2
#define D3D11_BIND_INDEX_BUFFER 0x4
#define D3D11_BIND_SHADER_RESOURCE 0x8
#define D3D11_CPU_ACCESS_WRITE 0x10000
#define D3D11_CPU_ACCESS_READ  0x20000

struct D3DX11_IMAGE_LOAD_INFO {
    int Width = 0;
    int Height = 0;
    int Depth = 0;
    int FirstMipLevel = 0;
    int MipLevels = 1;
    uint32_t Usage = 0;
    uint32_t BindFlags = 0;
    uint32_t CpuAccessFlags = 0;
    uint32_t MiscFlags = 0;
    DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
    uint32_t Filter = 0;
    uint32_t MipFilter = 0;
    void* pSrcInfo = nullptr;
};

using ID3DX11ThreadPump = void;

inline long D3DX11CreateTextureFromMemory(ID3D11Device* pDevice, const void* pSrcData, size_t SrcDataSize, D3DX11_IMAGE_LOAD_INFO* pLoadInfo, ID3DX11ThreadPump* pPump, ID3D11Resource** ppTexture, long* pHResult) { 
    if (pSrcData == nullptr) return -1;

    ID3D11Texture2D* tex = new ID3D11Texture2D();
    *ppTexture = tex;
    
    int w = 0, h = 0, n = 0;
    unsigned char* data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(pSrcData), static_cast<int>(SrcDataSize), &w, &h, &n, 4);
    
    if (data) {
        tex->width = w;
        tex->height = h;
        tex->format = DXGI_FORMAT_R8G8B8A8_UNORM;

        glGenTextures(1, &tex->glId);
        glBindTexture(GL_TEXTURE_2D, tex->glId);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        tex->cpuData.resize(w * h * 4);
        memcpy(tex->cpuData.data(), data, w * h * 4);

        stbi_image_free(data);
    } else {
        std::cerr << "D3DX11CreateTextureFromMemory: Failed to load image. Reason: " << stbi_failure_reason() << std::endl;
        tex->width = 2;
        tex->height = 2;
        tex->format = DXGI_FORMAT_R8G8B8A8_UNORM;
        
        glGenTextures(1, &tex->glId);
        glBindTexture(GL_TEXTURE_2D, tex->glId);
        
        unsigned char fallbackPixels[] = { 
            255, 0, 0, 255,   255, 0, 0, 255,
            255, 0, 0, 255,   255, 0, 0, 255
        };
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, fallbackPixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    
    return 0;
}

inline long D3DX11CreateTextureFromFile(ID3D11Device* pDevice, const char* pSrcFile, D3DX11_IMAGE_LOAD_INFO* pLoadInfo, ID3DX11ThreadPump* pPump, ID3D11Resource** ppTexture, long* pHResult) { 
    if (pSrcFile == nullptr) return -1;

    ID3D11Texture2D* tex = new ID3D11Texture2D();
    *ppTexture = tex;
    
    int w = 0, h = 0, n = 0;
    unsigned char* data = stbi_load(pSrcFile, &w, &h, &n, 4);
    
    if (data) {
        tex->width = w;
        tex->height = h;
        tex->format = DXGI_FORMAT_R8G8B8A8_UNORM;

        glGenTextures(1, &tex->glId);
        glBindTexture(GL_TEXTURE_2D, tex->glId);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        tex->cpuData.resize(w * h * 4);
        memcpy(tex->cpuData.data(), data, w * h * 4);

        stbi_image_free(data);
    } else {
        std::cerr << "D3DX11CreateTextureFromFile: Failed to load image: " << pSrcFile << std::endl;
        tex->width = 2;
        tex->height = 2;
        tex->format = DXGI_FORMAT_R8G8B8A8_UNORM;
        
        glGenTextures(1, &tex->glId);
        glBindTexture(GL_TEXTURE_2D, tex->glId);
        
        unsigned char fallbackPixels[] = { 
            255, 0, 0, 255,   255, 0, 0, 255,
            255, 0, 0, 255,   255, 0, 0, 255
        };
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, fallbackPixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    
    return 0;
}

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
    void VSSetConstantBuffers(uint32_t StartSlot, uint32_t NumBuffers, ID3D11Buffer** ppConstantBuffers);
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
    uint32_t _currentTopology = 4;
    DXGI_FORMAT _indexFormat = DXGI_FORMAT_R16_UINT;

    ID3D11Buffer* _vsConstantBuffers[14] = {nullptr};
    void ApplyConstantBuffers();

    ID3D11Device* _dev;
    ID3D11DeviceContext* _devCon;

    std::list<CDXAdapter*> _adapters;
};
