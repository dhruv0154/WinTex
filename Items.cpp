#include "Items.h"
#include "Globals.h"
#include "Utilities.h"
#include "GameController.h"
#include "ConstantBuffers.h"
#include <cstring>
#include <cmath>
#include <iostream>
#include <SDL2/SDL.h>

#define ITEMS_TEXTURE_PIXELS    128
#define ITEMS_TEXTURE_WIDTH     (ITEMS_TEXTURE_PIXELS * 16)
#define ITEMS_TEXTURE_HEIGHT    (ITEMS_TEXTURE_PIXELS * 20)

ID3D11Buffer* CItems::_vertexBuffer = nullptr;
ID3D11Texture2D* CItems::_texture = nullptr;
ID3D11ShaderResourceView* CItems::_textureRV = nullptr;

std::unordered_map<int, CItems::Inventory*> CItems::_items;

int CItems::Colour1 = 0xff000000;
int CItems::Colour2 = -1;
int CItems::Colour3 = -1;
int CItems::Colour4 = 0xff000000;

CItems::CItems()
{
    _vertexBuffer = nullptr;
    _texture = nullptr;
    _textureRV = nullptr;
}

CItems::~CItems()
{
    Dispose();
}

bool CItems::Init()
{
    bool ret = false;

    int w = ITEMS_TEXTURE_WIDTH;
    int h = ITEMS_TEXTURE_HEIGHT;

    D3D11_TEXTURE2D_DESC desc;
    std::memset(&desc, 0, sizeof(desc));
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;

    if (dx.CreateTexture2D(&desc, nullptr, &_texture) == 0)
    {
        if (dx.CreateShaderResourceView(_texture, nullptr, &_textureRV) == 0)
        {
            D3D11_MAPPED_SUBRESOURCE subRes;
            if (dx.Map(_texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes) == 0)
            {
                uint8_t* pScr = static_cast<uint8_t*>(subRes.pData);
                if (pScr != nullptr)
                {
                    std::memset(pScr, 0, h * subRes.RowPitch);

                    CFile file;
                    if (file.Open("INV.AP"))
                    {
                        uint32_t length = file.Seek(0, CFile::SeekMethod::End);
                        uint8_t* pFileData = new uint8_t[length];
                        if (pFileData)
                        {
                            file.Seek(0, CFile::SeekMethod::Begin);
                            file.Read(pFileData, length);

                            int maxItems = GetInt(pFileData, 0, 2) - 3;
                            uint8_t* pPalette = pFileData + GetInt(pFileData, 2, 4);

                            for (int i = 0; i < 256 * 3; i++)
                            {
                                pPalette[i] = static_cast<uint8_t>((static_cast<int>(pPalette[i]) & 0xff) * 4.04762f);
                            }

                            for (int i = 0; i < 20; i++)
                            {
                                for (int j = 0; j < 16; j++)
                                {
                                    int index = 2 + (i * 16 + j);
                                    if (index < maxItems)
                                    {
                                        int offset = GetInt(pFileData, 2 + index * 4, 4);
                                        int compressedSize = GetInt(pFileData, 6 + index * 4, 4) - offset;
                                        uint8_t* pCompressed = pFileData + offset;

                                        BinaryData bd = CLZ::Decompress(pCompressed, compressedSize);
                                        if (bd.Data != nullptr)
                                        {
                                            if (GetInt(bd.Data, 0, 2) == 0x100)
                                            {
                                                int imgW = GetInt(bd.Data, 2, 2);
                                                int imgH = GetInt(bd.Data, 4, 2);
                                                int dataOffset = 16;

                                                for (int y = 0; y < imgH; y++)
                                                {
                                                    int c1 = GetInt(bd.Data, dataOffset, 2);
                                                    int c2 = GetInt(bd.Data, dataOffset + 2, 2);
                                                    dataOffset += 4;

                                                    for (int x = 0; x < imgW && x < c2; x++)
                                                    {
                                                        int colIx = bd.Data[dataOffset + x];
                                                        if (colIx != 0)
                                                        {
                                                            int dst = (i * ITEMS_TEXTURE_PIXELS + y) * subRes.RowPitch + (j * ITEMS_TEXTURE_PIXELS + x + c1) * 4;
                                                            pScr[dst + 0] = pPalette[colIx * 3 + 2];
                                                            pScr[dst + 1] = pPalette[colIx * 3 + 1];
                                                            pScr[dst + 2] = pPalette[colIx * 3 + 0];
                                                            pScr[dst + 3] = 255;
                                                        }
                                                    }

                                                    dataOffset += c2;
                                                }
                                            }

                                            delete[] bd.Data;

                                            int id = i * 16 + j;
                                            _items[id] = new Inventory(id, w, h);
                                        }
                                    }
                                }
                            }

                            delete[] pFileData;

                            TEXTURED_VERTEX_ORTHO* pVB = new TEXTURED_VERTEX_ORTHO[6];
                            if (pVB != nullptr)
                            {
                                float x1 = 0.0f;
                                float x2 = dx.GetWidth() * 1.0f;
                                float y1 = 0.0f;
                                float y2 = -dx.GetHeight() * 1.6f;

                                pVB[0].position = float3(x1, y1, -0.25f);
                                pVB[0].texture = float2(0.0f, 0.0f);
                                pVB[1].position = float3(x2, y1, -0.25f);
                                pVB[1].texture = float2(1.0f, 0.0f);
                                pVB[2].position = float3(x2, y2, -0.25f);
                                pVB[2].texture = float2(1.0f, 1.0f);

                                pVB[3].position = float3(x1, y1, -0.25f);
                                pVB[3].texture = float2(0.0f, 0.0f);
                                pVB[4].position = float3(x2, y2, -0.25f);
                                pVB[4].texture = float2(1.0f, 1.0f);
                                pVB[5].position = float3(x1, y2, -0.25f);
                                pVB[5].texture = float2(0.0f, 1.0f);

                                D3D11_BUFFER_DESC vbDesc;
                                std::memset(&vbDesc, 0, sizeof(vbDesc));
                                vbDesc.Usage = D3D11_USAGE_DYNAMIC;
                                vbDesc.ByteWidth = sizeof(TEXTURED_VERTEX_ORTHO) * 6;
                                vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                                vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                                vbDesc.StructureByteStride = 0;

                                D3D11_SUBRESOURCE_DATA vData;
                                std::memset(&vData, 0, sizeof(vData));
                                vData.pSysMem = pVB;
                                vData.SysMemPitch = 0;
                                vData.SysMemSlicePitch = 0;

                                dx.CreateBuffer(&vbDesc, &vData, &_vertexBuffer, "Items");

                                delete[] pVB;
                                ret = true;
                            }
                        }
                    }
                    else
                    {
                        std::cerr << "[Items Error] Could not open archive file INV.AP" << std::endl;
                        SDL_Log("Could not open archive file INV.AP");
                    }
                }

                dx.Unmap(_texture, 0);
            }
        }
    }

    return ret;
}

void CItems::Dispose()
{
    if (_textureRV != nullptr)
    {
        _textureRV->Release();
        _textureRV = nullptr;
    }

    if (_texture != nullptr)
    {
        _texture->Release();
        _texture = nullptr;
    }

    if (_vertexBuffer != nullptr)
    {
        _vertexBuffer->Release();
        _vertexBuffer = nullptr;
    }

    for (auto& it : _items)
    {
        delete it.second;
    }
    _items.clear();
}

void CItems::Render()
{
    if (_vertexBuffer == nullptr)
    {
        return;
    }

    unsigned int stride = sizeof(TEXTURED_VERTEX_ORTHO);
    unsigned int offset = 0;
    dx.SetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);

    dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    float16 wm = Math::Translation(0.0f, 0.0f, -1.0f);

    CConstantBuffers::SetWorld(dx, &wm);
    dx.SetShaderResources(0, 1, &_textureRV);
    CShaders::SelectOrthoShader();
    dx.Draw(6, 0);
}

CItems::Inventory::Inventory(int id, int w, int h)
{
    Id = id;
    ImageBuffer = nullptr;

    TEXTURED_VERTEX_ORTHO* pVB = new TEXTURED_VERTEX_ORTHO[6];
    if (pVB != nullptr)
    {
        int x = id % 16;
        int y = id / 16;
        float tx1 = (x * ITEMS_TEXTURE_PIXELS) / static_cast<float>(ITEMS_TEXTURE_WIDTH);
        float tx2 = ((x + 1) * ITEMS_TEXTURE_PIXELS) / static_cast<float>(ITEMS_TEXTURE_WIDTH);
        float ty1 = (y * ITEMS_TEXTURE_PIXELS) / static_cast<float>(ITEMS_TEXTURE_HEIGHT);
        float ty2 = ((y + 1) * ITEMS_TEXTURE_PIXELS) / static_cast<float>(ITEMS_TEXTURE_HEIGHT);

        // Replaced XMFLOAT3/XMFLOAT2 with custom float3/float2
        pVB[0].position = float3(0.0f, 0.0f, -0.5f);
        pVB[0].texture = float2(tx1, ty1);
        pVB[1].position = float3(ITEMS_TEXTURE_PIXELS, 0.0f, -0.5f);
        pVB[1].texture = float2(tx2, ty1);
        pVB[2].position = float3(ITEMS_TEXTURE_PIXELS, -ITEMS_TEXTURE_PIXELS, -0.5f);
        pVB[2].texture = float2(tx2, ty2);

        pVB[3].position = float3(0.0f, 0.0f, -0.5f);
        pVB[3].texture = float2(tx1, ty1);
        pVB[4].position = float3(ITEMS_TEXTURE_PIXELS, -ITEMS_TEXTURE_PIXELS, -0.5f);
        pVB[4].texture = float2(tx2, ty2);
        pVB[5].position = float3(0.0f, -ITEMS_TEXTURE_PIXELS, -0.5f);
        pVB[5].texture = float2(tx1, ty2);

        D3D11_BUFFER_DESC vbDesc;
        std::memset(&vbDesc, 0, sizeof(vbDesc));
        vbDesc.Usage = D3D11_USAGE_DYNAMIC;
        vbDesc.ByteWidth = sizeof(TEXTURED_VERTEX_ORTHO) * 6;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vbDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA vData;
        std::memset(&vData, 0, sizeof(vData));
        vData.pSysMem = pVB;
        vData.SysMemPitch = 0;
        vData.SysMemSlicePitch = 0;

        dx.CreateBuffer(&vbDesc, &vData, &ImageBuffer, "Inventory");

        delete[] pVB;
    }

    std::string text = CGameController::GetItemName(id);
    if (!text.empty())
    {
        Text.SetText(text.c_str());
        Text.SetColours(Colour1, Colour2, Colour3, Colour4);
    }
}

CItems::Inventory::~Inventory()
{
    if (ImageBuffer != nullptr)
    {
        ImageBuffer->Release();
        ImageBuffer = nullptr;
    }
}

void CItems::Inventory::Render(float x, float y)
{
    RenderImage(x, y);
}

void CItems::Inventory::RenderName(float x, float y, int colour1, int colour2, int colour3, int colour4, bool highlight)
{
    Text.SetColours(colour1, colour2, colour3, colour4);
    Text.Render(x, y);
}

void CItems::Inventory::RenderImage(float x, float y, bool highlight)
{
    if (ImageBuffer == nullptr)
    {
        return;
    }
    unsigned int stride = sizeof(TEXTURED_VERTEX_ORTHO);
    unsigned int offset = 0;
    dx.SetVertexBuffers(0, 1, &ImageBuffer, &stride, &offset);

    dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Replaced XMMatrixTranslation with custom Math::Translation returning float16
    float16 wm = Math::Translation(x, -y, -1.0f);
    if (highlight)
    {
        // TODO: Draw selection rectangle
    }

    CConstantBuffers::SetWorld(dx, &wm);
    dx.SetShaderResources(0, 1, &_textureRV);
    CShaders::SelectOrthoShader();
    dx.Draw(6, 0);
}

void CItems::RenderItem(int id, float x, float y)
{
    auto it = _items.find(id);
    if (it != _items.end() && it->second != nullptr)
    {
        it->second->Render(x, y);
    }
}

void CItems::RenderItemName(int id, float x, float y, bool highlight)
{
    auto it = _items.find(id);
    if (it != _items.end() && it->second != nullptr)
    {
        it->second->RenderName(x, y, Colour1, Colour2, Colour3, Colour4, highlight);
    }
}

void CItems::RenderItemImage(int id, float x, float y, bool highlight)
{
    auto it = _items.find(id);
    if (it != _items.end() && it->second != nullptr)
    {
        it->second->RenderImage(x, y, highlight);
    }
}

int CItems::GetWidestName()
{
    float widest = 0.0f;
    for (const auto& it : _items)
    {
        if (it.second != nullptr)
        {
            float width = it.second->NameWidth();
            if (width > widest)
            {
                widest = width;
            }
        }
    }

    return static_cast<int>(std::ceil(widest));
}

int CItems::GetItemNameWidth(int id)
{
    auto it = _items.find(id);
    if (it != _items.end() && it->second != nullptr)
    {
        return static_cast<int>(it->second->NameWidth());
    }
    return 0;
}

void CItems::SetItemName(int id, const std::string& text)
{
    auto it = _items.find(id);
    if (it != _items.end() && it->second != nullptr)
    {
        it->second->SetItemName(text);
    }
}

void CItems::ResetText()
{
    for (auto& it : _items)
    {
        if (it.second != nullptr)
        {
            it.second->ResetText();
        }
    }
}

void CItems::Inventory::ResetText()
{
    Text.ResetText();
}