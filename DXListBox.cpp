#include "DXListBox.h"
#include "Globals.h"
#include "Utilities.h"
#include "DXScreen.h"
#include "ConstantBuffers.h"
#include <algorithm>
#include <cmath>

CTexture CDXListBox::_texBackground;

int CDXListBox::_greyColour1 = 0;
int CDXListBox::_greyColour2 = 0xffc3c3c3;
int CDXListBox::_greyColour3 = 0xffc3c3c3;
int CDXListBox::_greyColour4 = 0;
int CDXListBox::_blackColour1 = 0;
int CDXListBox::_blackColour2 = 0xff000000;
int CDXListBox::_blackColour3 = 0xff000000;
int CDXListBox::_blackColour4 = 0;

CDXListBox::CDXListBox() : CDXControl()
{
    _topIndex = 0;
    _visibleItemsCount = 0;
    _highlightIndex = -1;
    _vertexBuffer = nullptr;
}

CDXListBox::~CDXListBox()
{
    Clear();
}

void CDXListBox::Init(const std::vector<ListBoxItem>& items, float btnX)
{
    Clear();

    _items.clear();

    std::list<std::list<PointUV>> lines;
    int totalVertices = 18 * 6;
    _w = 0.0f;

    for (const auto& item : items)
    {
        ListBoxItem lbi = item;
        std::list<PointUV> points = TexFont.Create(item.Text.c_str(), true);
        
        lbi.StartVertex = totalVertices;
        lbi.VerticeCount = static_cast<int>(points.size());
        lbi.MouseOver = false;

        _items.push_back(lbi);
        totalVertices += static_cast<int>(points.size());
        lines.push_back(points);

        for (const auto& pt : points)
        {
            if (pt.x > _w) _w = pt.x;
        }
    }

    _w += 8.0f;
    float lineHeight = (TexFont.Height() + 2.0f) * pConfig->FontScale;

    std::vector<TEXTURED_VERTEX_ORTHO> pVB(totalVertices);

    float y = 0.0f;
    int pix = 18 * 6;

    float x1 = 0.0f;
    float x2 = 16.0f * pConfig->FontScale;
    float x3 = x2 + _w - 16.0f * pConfig->FontScale;
    float x4 = std::ceil(x3 + 16.0f * pConfig->FontScale);
    float y1 = 0.0f;
    float y2 = -1.0f;
    float y3 = y2 - lineHeight * static_cast<float>(items.size());
    float y4 = y3 - 1.0f;

    _w = x4;

    float u1 = 0.0f;
    float u2 = 0.25f;
    float u3 = 0.5f;
    float u4 = 0.75f;
    float v1 = 0.0f;
    float v2 = 0.25f;
    float v3 = 0.5f;
    float v4 = 0.75f;

    SetQuadVertex(pVB.data(), 0, x1, x4, y1, y2, u1, u3, v1, v2);  // Top line
    SetQuadVertex(pVB.data(), 1, x1, x2, y2, y3, u1, u2, v3, v3);  // Box, left
    SetQuadVertex(pVB.data(), 2, x2, x3, y2, y3, u2, u3, v3, v3);  // Box, center
    SetQuadVertex(pVB.data(), 3, x3, x4, y2, y3, u3, u4, v3, v3);  // Box, right
    SetQuadVertex(pVB.data(), 4, x1, x4, y3, y4, u1, u3, v1, v2);  // Bottom line

    SetQuadVertex(pVB.data(), 5, x1, x2, 0.0f, -lineHeight, u1, u2, v3, v3);   // Box, left
    SetQuadVertex(pVB.data(), 6, x2, x3, 0.0f, -lineHeight, u2, u3, v3, v3);   // Box, center
    SetQuadVertex(pVB.data(), 7, x3, x4, 0.0f, -lineHeight, u3, u4, v3, v3);   // Box, right

    for (const auto& points : lines)
    {
        _listBoxVerticeInfo.push_back(pix * 65536 + static_cast<int>(points.size()));

        for (const auto& p : points)
        {
            pVB[pix].position.x = p.x + 8.0f * pConfig->FontScale;
            pVB[pix].position.y = y - p.y + (18.0f * pConfig->FontScale);
            pVB[pix].position.z = -0.5f;
            pVB[pix].texture.x = p.u;
            pVB[pix++].texture.y = p.v;
        }

        y -= lineHeight;
    }

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.ByteWidth = sizeof(TEXTURED_VERTEX_ORTHO) * totalVertices;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vbDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA vData = {};
    vData.pSysMem = pVB.data();
    vData.SysMemPitch = 0;
    vData.SysMemSlicePitch = 0;

    dx.CreateBuffer(&vbDesc, &vData, &_vertexBuffer, "LISTBOX1");

    _h = 2.0f + static_cast<float>(_items.size()) * lineHeight;
    _y = (dx.GetHeight() - _h - 64.0f * pConfig->FontScale);

    _visibleItemsCount = static_cast<int>(items.size());
    _x = std::floor(std::max(0.0f, btnX - _w / 2.0f));
}

void CDXListBox::Clear()
{
    if (_vertexBuffer != nullptr)
    {
        _vertexBuffer->Release();
        _vertexBuffer = nullptr;
    }

    _listBoxVerticeInfo.clear();
}

void CDXListBox::Render()
{
    if (_vertexBuffer == nullptr) return;

    float lineHeight = (TexFont.Height() + 2.0f) * pConfig->FontScale;

    uint32_t stride = sizeof(TEXTURED_VERTEX_ORTHO);
    uint32_t offset = 0;
    dx.SetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
    dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11ShaderResourceView* pRV = _texBackground.GetTextureRV();
    dx.SetShaderResources(0, 1, &pRV);

    float16 wm = Math::Translation(_x, -_y, 0.0f);
    CConstantBuffers::SetWorld(dx, &wm);

    CShaders::SelectOrthoShader();
    dx.Draw(5 * 6, 0);

    CShaders::SelectTexFontShader();

    CDXFont::SelectGreyFont();
    if (_highlightIndex >= 0)
    {
        wm = Math::Translation(_x, -_y - 1.0f - static_cast<float>(_highlightIndex) * lineHeight, -0.1f);
        CConstantBuffers::SetWorld(dx, &wm);
        dx.Draw(3 * 6, 5 * 6);
    }

    CDXFont::SelectFontColour(_greyColour1, _greyColour2, _greyColour3, _greyColour4);

    // Draw text
    wm = Math::Translation(_x, -_y - 20.0f * pConfig->FontScale, -0.5f);
    CConstantBuffers::SetWorld(dx, &wm);
    pRV = TexFont.GetTextureRV();
    dx.SetShaderResources(0, 1, &pRV);

    if (!_items.empty())
    {
        if (_highlightIndex >= 0)
        {
            // Draw items pre-selection
            int startVertex = 0x6c;
            int vertexCount = 0;

            int i = 0;
            for (i = 0; i < _visibleItemsCount && i < _highlightIndex; i++)
            {
                if (_topIndex + i < static_cast<int>(_items.size()))
                {
                    ListBoxItem lbi = _items.at(_topIndex + i);
                    vertexCount += lbi.VerticeCount;
                }
            }

            if (vertexCount > 0)
            {
                dx.Draw(vertexCount, startVertex);
            }

            // Draw items post-selection
            if ((_topIndex + _highlightIndex) < _visibleItemsCount)
            {
                startVertex = -1;
                vertexCount = 0;
                for (i++; i < _visibleItemsCount; i++)
                {
                    if (_topIndex + i < static_cast<int>(_items.size()))
                    {
                        ListBoxItem lbi = _items.at(_topIndex + i);
                        vertexCount += lbi.VerticeCount;
                        if (startVertex < 0)
                        {
                            startVertex = lbi.StartVertex;
                        }
                    }
                }

                if (vertexCount > 0)
                {
                    dx.Draw(vertexCount, startVertex);
                }
            }

            // Draw highlighted selection
            CDXFont::SelectFontColour(_blackColour1, _blackColour2, _blackColour3, _blackColour4);

            if ((_topIndex + _highlightIndex) >= static_cast<int>(_items.size()))
            {
                _highlightIndex = 0;
            }
            
            if (_topIndex + _highlightIndex < static_cast<int>(_items.size()))
            {
                ListBoxItem lbi = _items.at(_topIndex + _highlightIndex);
                dx.Draw(lbi.VerticeCount, lbi.StartVertex);
            }
        }
        else
        {
            // Draw all items with same default colour
            if (_topIndex < static_cast<int>(_items.size()))
            {
                ListBoxItem lbi = _items.at(_topIndex);
                int startVertex = lbi.StartVertex;
                int vertexCount = lbi.VerticeCount;
                
                for (int i = 1; i < _visibleItemsCount; i++)
                {
                    if (_topIndex + i < static_cast<int>(_items.size()))
                    {
                        lbi = _items.at(_topIndex + i);
                        vertexCount += lbi.VerticeCount;
                    }
                }

                dx.Draw(vertexCount, startVertex);
            }
        }
    }
}

void CDXListBox::Init()
{
    uint32_t s = 0;
    uint8_t* p = GetResource(IDB_LISTBOX, "PNG", &s);

    _texBackground.Init(p, s, "LISTBOX1");
}

void CDXListBox::Dispose()
{
    _texBackground.Dispose();
}

int CDXListBox::HitTestLB(float x, float y)
{
    _highlightIndex = -1;
    float lineHeight = (TexFont.Height() + 2.0f) * pConfig->FontScale;

    if (x >= _x && x < (_x + _w) && y > _y && y < (_y + _h - 1.0f))
    {
        _highlightIndex = static_cast<int>((y - _y) / lineHeight);
        int selectedIndex = _topIndex + _highlightIndex;
        
        if (selectedIndex >= 0 && static_cast<size_t>(selectedIndex) < _items.size())
        {
            ListBoxItem lbi = _items.at(selectedIndex);
            return lbi.Id;
        }
        else
        {
            _highlightIndex = -1;
        }
    }

    return _highlightIndex;
}
