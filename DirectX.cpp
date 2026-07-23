#include "DirectX.h"
#include "Globals.h"
#include <sstream>
#include <string>
#include <iostream>

static SDL_Window* g_Window = nullptr;
static SDL_GLContext g_GLContext = nullptr;
static GLuint g_VAO = 0;

CDirectX::CDirectX() {
    _dev = new ID3D11Device();
    _devCon = new ID3D11DeviceContext();
    _width = 0;
    _height = 0;
    _adapters.push_back(new CDXAdapter());
}

CDirectX::~CDirectX() {
    Dispose();
}

bool CDirectX::Init(void* hWnd, int width, int height, bool windowed, bool anisotropicFilter, int bufferCount) {
    _width = width;
    _height = height;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
    if (!windowed) flags |= SDL_WINDOW_FULLSCREEN;

    g_Window = SDL_CreateWindow("WinTex SDL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
    if (!g_Window) {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return false;
    }

    g_GLContext = SDL_GL_CreateContext(g_Window);
    if (!g_GLContext) {
        std::cout << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize generic GL state
    glGenVertexArrays(1, &g_VAO);
    glBindVertexArray(g_VAO);

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // Disable culling to be safe

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

void CDirectX::Dispose() {
    if (g_VAO) { glDeleteVertexArrays(1, &g_VAO); g_VAO = 0; }
    if (_dev) { delete _dev; _dev = nullptr; }
    if (_devCon) { delete _devCon; _devCon = nullptr; }
    if (g_GLContext) { SDL_GL_DeleteContext(g_GLContext); g_GLContext = nullptr; }
    if (g_Window) { SDL_DestroyWindow(g_Window); g_Window = nullptr; }

    for (auto adapter : _adapters) { delete adapter; }
    _adapters.clear();
    SDL_Quit();
}

void CDirectX::SetFullScreen(bool fullScreen) {
    if (g_Window) {
        SDL_SetWindowFullscreen(g_Window, fullScreen ? SDL_WINDOW_FULLSCREEN : 0);
    }
}

void CDirectX::Clear(float red, float green, float blue) {
    //if (red == 0.0f && green == 0.0f && blue == 0.0f) {
    //    red = 1.0f; green = 0.0f; blue = 1.0f; // Magenta for debugging
    //}
    glClearColor(red, green, blue, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void CDirectX::Present(uint32_t syncInterval, uint32_t flags) {
    if (g_Window) {
        SDL_GL_SwapWindow(g_Window);
    }
}

int CDirectX::CreateBuffer(D3D11_BUFFER_DESC* pDesc, D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Buffer** ppBuffer, const char* name) { 
    *ppBuffer = new ID3D11Buffer();
    (*ppBuffer)->byteWidth = pDesc->ByteWidth;
    (*ppBuffer)->bindFlags = pDesc->BindFlags;
    (*ppBuffer)->cpuData.resize(pDesc->ByteWidth);

    if (pDesc->BindFlags & D3D11_BIND_CONSTANT_BUFFER) {
        // Check if this is a large buffer that needs UBO (Visibility/Translation)
        bool createUBO = false;
        if (name != nullptr) {
            if (strcmp(name, "Visibility") == 0 || strcmp(name, "Translation") == 0 || strcmp(name, "TexFont") == 0) {
                createUBO = true;
            }
        }

        if (createUBO) {
             glGenBuffers(1, &(*ppBuffer)->glId);
             glBindBuffer(GL_UNIFORM_BUFFER, (*ppBuffer)->glId);
             // Initialize with CPU data if available, or just allocate
             if (pInitialData) {
                 glBufferData(GL_UNIFORM_BUFFER, pDesc->ByteWidth, pInitialData->pSysMem, GL_DYNAMIC_DRAW);
                 memcpy((*ppBuffer)->cpuData.data(), pInitialData->pSysMem, pDesc->ByteWidth);
             } else {
                 glBufferData(GL_UNIFORM_BUFFER, pDesc->ByteWidth, nullptr, GL_DYNAMIC_DRAW);
             }
             glBindBuffer(GL_UNIFORM_BUFFER, 0);
        } else {
             // CPU-only for emulation of small constant buffers (matrices etc)
             if (pInitialData) {
                 memcpy((*ppBuffer)->cpuData.data(), pInitialData->pSysMem, pDesc->ByteWidth);
             }
        }
        return 0;
    }

    // For Vertex/Index buffers, create GL buffer
    glGenBuffers(1, &(*ppBuffer)->glId);
    
    GLenum target = (pDesc->BindFlags & D3D11_BIND_VERTEX_BUFFER) ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;
    
    glBindBuffer(target, (*ppBuffer)->glId);
    if (pInitialData) {
        glBufferData(target, pDesc->ByteWidth, pInitialData->pSysMem, (pDesc->Usage == D3D11_USAGE_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        // Also keep copy in cpuData for Map/Unmap if needed?
        // Actually Map/Unmap implementation in Win32Compat.h uses cpuData and uploads on Unmap.
        // So we should initialize cpuData with pInitialData.
        memcpy((*ppBuffer)->cpuData.data(), pInitialData->pSysMem, pDesc->ByteWidth);
    } else {
        glBufferData(target, pDesc->ByteWidth, nullptr, (pDesc->Usage == D3D11_USAGE_DYNAMIC) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    }
    glBindBuffer(target, 0);

    return 0;
}

int CDirectX::Map(ID3D11Resource* pResource, uint32_t subResource, D3D11_MAP mapType, uint32_t mapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource) { 
    ID3D11Buffer* buf = (ID3D11Buffer*)pResource;

    if (buf && pMappedResource) {
        pMappedResource->pData = buf->cpuData.data();
        return 0;
    }
    return -1;
}

void CDirectX::Unmap(ID3D11Resource* pResource, uint32_t subResource) {

    ID3D11Buffer* buf = (ID3D11Buffer*)pResource;
    
    if (buf && buf->glId != 0) {
        GLenum target = (buf->bindFlags & D3D11_BIND_VERTEX_BUFFER) ? GL_ARRAY_BUFFER : 
                        (buf->bindFlags & D3D11_BIND_INDEX_BUFFER) ? GL_ELEMENT_ARRAY_BUFFER : GL_UNIFORM_BUFFER;
        
        glBindBuffer(target, buf->glId);
        glBufferSubData(target, 0, buf->byteWidth, buf->cpuData.data());
        glBindBuffer(target, 0);
    }
}

ID3D11Device* CDirectX::GetDevice() { return _dev; }
ID3D11DeviceContext* CDirectX::GetDeviceContext() { return _devCon; }

int CDirectX::CreateTexture2D(D3D11_TEXTURE2D_DESC* pDesc, D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D, const char* name) { 
    *ppTexture2D = new ID3D11Texture2D();
    (*ppTexture2D)->width = pDesc->Width;
    (*ppTexture2D)->height = pDesc->Height;
    (*ppTexture2D)->format = pDesc->Format;

    glGenTextures(1, &(*ppTexture2D)->glId);
    glBindTexture(GL_TEXTURE_2D, (*ppTexture2D)->glId);
    
    GLint internalFormat = GL_RGBA;
    GLenum format = GL_RGBA;
    if (pDesc->Format == DXGI_FORMAT_B8G8R8A8_UNORM) format = GL_BGRA;
    GLenum type = GL_UNSIGNED_BYTE;
    
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, pDesc->Width, pDesc->Height, 0, format, type, pInitialData ? pInitialData->pSysMem : nullptr);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    return 0;
}

int CDirectX::CreateShaderResourceView(ID3D11Resource* pResource, D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppSRView, const char* name) { 
    *ppSRView = new ID3D11ShaderResourceView();
    // In D3D11, SRV is a view of a resource. In GL, we just use the texture ID.
    // We can copy the GL ID from the resource.
    if (pResource) (*ppSRView)->glId = pResource->glId;
    return 0;
}

void CDirectX::SetVertexBuffers(uint32_t StartSlot, uint32_t NumBuffers, ID3D11Buffer** ppVertexBuffers, const uint32_t* pStrides, const uint32_t* pOffsets) {
    if (NumBuffers > 0 && ppVertexBuffers[0]) {
        glBindBuffer(GL_ARRAY_BUFFER, ppVertexBuffers[0]->glId);
        
        uint32_t stride = pStrides[0];
        
        // Disable all arrays first to be safe (or at least the ones we might use)
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        // Also disable 2 and 3 just in case they were enabled
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(3);

        if (stride == 20) { // TEXTURED_VERTEX_ORTHO
             glEnableVertexAttribArray(0); // Position (XMFLOAT3)
             glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
             glEnableVertexAttribArray(1); // TexCoord (XMFLOAT2)
             glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)12);
        }
        else if (stride == 44) { // TEXTURED_VERTEX
             glEnableVertexAttribArray(0); // Position (XMFLOAT3)
             glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
             glEnableVertexAttribArray(1); // TexCoord (XMFLOAT2)
             glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)12);
             glEnableVertexAttribArray(2); // Object (XMFLOAT2)
             glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)20);
             glEnableVertexAttribArray(3); // ObjectParameters (XMFLOAT4)
             glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)28);
        }
        else if (stride == 32) { // COLOURED_VERTEX_ORTHO
             glEnableVertexAttribArray(0); // Position (XMFLOAT4)
             glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
             glEnableVertexAttribArray(1); // Color (XMFLOAT4)
             glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)16);
        }
        else if (stride == 48) { // COLOURED_VERTEX
             glEnableVertexAttribArray(0); // Position (XMFLOAT4)
             glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
             glEnableVertexAttribArray(1); // Color (XMFLOAT4)
             glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)16);
        }
        else {
             // Default fallback (assume TEXTURED_VERTEX_ORTHO or similar)
             glEnableVertexAttribArray(0); 
             glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
             glEnableVertexAttribArray(1); 
             glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)12);
        }
    }
}

void CDirectX::SetIndexBuffer(ID3D11Buffer* pIndexBuffer, DXGI_FORMAT Format, uint32_t Offset) {
    if (pIndexBuffer) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pIndexBuffer->glId);
}

void CDirectX::VSSetConstantBuffers(uint32_t StartSlot, uint32_t NumBuffers, ID3D11Buffer** ppConstantBuffers) {
    for (uint32_t i = 0; i < NumBuffers; i++) {
        if (StartSlot + i < 14) {
            _vsConstantBuffers[StartSlot + i] = ppConstantBuffers[i];
        }
    }
}

void CDirectX::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology) {
    _currentTopology = Topology;
}

void CDirectX::ApplyConstantBuffers() {
    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    if (prog == 0) return;

    for (int i = 0; i < 14; i++)
    {
        ID3D11Buffer* buf = _vsConstantBuffers[i];
        if (!buf) continue;

        if (i == 0)
        {
            GLint view = glGetUniformLocation(prog, "View");
            GLint ortho = glGetUniformLocation(prog, "Ortho");
            GLint proj = glGetUniformLocation(prog, "Projection");

            if (view != -1) glUniformMatrix4fv(view, 1, GL_FALSE, (float*)(buf->cpuData.data()));
            if (ortho != -1) glUniformMatrix4fv(ortho, 1, GL_FALSE, (float*)(buf->cpuData.data() + 64)); 
            if (proj != -1) glUniformMatrix4fv(proj, 1, GL_FALSE, (float*)(buf->cpuData.data() + 128));
        }

        else if (i == 1) 
        {
            GLint locWorld = glGetUniformLocation(prog, "World");
            if (locWorld != -1) {
                glUniformMatrix4fv(locWorld, 1, GL_FALSE, (float*)(buf->cpuData.data()));
            }
        } 
        else if (i == 3) 
        {
            if (buf->glId != 0) glBindBufferBase(GL_UNIFORM_BUFFER, 3, buf->glId);
        } 
        else if (i == 4) 
        {
            if (buf->glId != 0) glBindBufferBase(GL_UNIFORM_BUFFER, 4, buf->glId);
        } 
        else if (i == 5) 
        {
            if (buf->glId != 0) glBindBufferBase(GL_UNIFORM_BUFFER, 5, buf->glId);
        }
    }
}

void CDirectX::Draw(uint32_t VertexCount, uint32_t StartVertexLocation) {
    ApplyConstantBuffers();

    GLuint glTopology = GL_TRIANGLES;
    if (_currentTopology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP)
    {
        glTopology = GL_TRIANGLE_STRIP;
    }

    glDrawArrays(glTopology, StartVertexLocation, VertexCount);
}

void CDirectX::DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, int32_t BaseVertexLocation) {
    ApplyConstantBuffers();

    GLenum glTopology = GL_TRIANGLES; 
    if (_currentTopology == 5) glTopology = GL_TRIANGLE_STRIP;

    void* offset = (void*)(uintptr_t)(StartIndexLocation * sizeof(GLuint));
    glDrawElementsBaseVertex(glTopology, IndexCount, GL_UNSIGNED_INT, offset, BaseVertexLocation);
}

void CDirectX::SetShaderResources(uint32_t StartSlot, uint32_t NumViews, ID3D11ShaderResourceView** ppShaderResourceViews) {
    if (NumViews > 0 && ppShaderResourceViews[0]) {
        glActiveTexture(GL_TEXTURE0 + StartSlot);

        glBindTexture(GL_TEXTURE_2D, ppShaderResourceViews[0]->glId);
    }
}

void CDirectX::EnableZBuffer() {
    glEnable(GL_DEPTH_TEST);
}

void CDirectX::DisableZBuffer() {
    glDisable(GL_DEPTH_TEST);
}

void CDirectX::Resize(int width, int height) {
    _width = width;
    _height = height;
    glViewport(0, 0, width, height);
}

CDXAdapter* CDirectX::GetAdapter() { return nullptr; }

void CDirectX::SelectSampler(bool anisotropic) {}
void CDirectX::SetViewport(D3D11_VIEWPORT viewport) {
    glViewport((GLint)viewport.TopLeftX, (GLint)viewport.TopLeftY, (GLsizei)viewport.Width, (GLsizei)viewport.Height);
}
void CDirectX::SetScissorRect(D3D11_RECT rect) {}