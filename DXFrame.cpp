#include "DXFrame.h"
#include "Globals.h"
#include "Utilities.h"
#include "DXScreen.h"
#include <algorithm>
#include <vector>

CTexture CDXFrame::_texBackground;

CDXFrame::CDXFrame(const char* title, float w, float h) : CDXContainer()
{
    _pText = new CDXText();
    Rect rc;
    rc.Top = 0.0f;
    rc.Left = 0.0f;
    rc.Bottom = -h;
    rc.Right = w;
    _pText->SetText(title, rc);

    _textW = static_cast<int>(_pText->PixelWidth(title));

    _x = 0.0f;
    _y = 0.0f;
    
    _w = std::max(w, static_cast<float>(_textW));
    _h = std::max(h, static_cast<float>(_texBackground.Height()));

    std::vector<TEXTURED_VERTEX_ORTHO> pVB(54);
    
    float x1 = 0.0f;
    float x2 = 16.0f;
    float x3 = _w - 16.0f;
    float x4 = _w;
    float y1 = 0.0f;
    float y2 = -_pText->Height() - 6.0f;
    float y3 = -_h - y2;
    float y4 = -_h;

    float u1 = 0.0f;
    float u2 = 0.25f;
    float u3 = 0.75f;
    float u4 = 1.0f;
    float v1 = 0.0f;
    float v2 = 0.26f;
    float v3 = 0.75f;
    float v4 = 1.0f;

    SetQuadVertex(pVB.data(), 0, x1, x2, y1, y2, u1, u2, v1, v2);
    SetQuadVertex(pVB.data(), 1, x2, x3, y1, y2, u2, u3, v1, v2);
    SetQuadVertex(pVB.data(), 2, x3, x4, y1, y2, u3, u4, v1, v2);

    SetQuadVertex(pVB.data(), 3, x1, x2, y2, y3, u1, u2, v2, v3);
    SetQuadVertex(pVB.data(), 4, x2, x3, y2, y3, u2, u3, v2, v3);
    SetQuadVertex(pVB.data(), 5, x3, x4, y2, y3, u3, u4, v2, v3);

    SetQuadVertex(pVB.data(), 6, x1, x2, y3, y4, u1, u2, v3, v4);
    SetQuadVertex(pVB.data(), 7, x2, x3, y3, y4, u2, u3, v3, v4);
    SetQuadVertex(pVB.data(), 8, x3, x4, y3, y4, u3, u4, v3, v4);

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.ByteWidth = sizeof(TEXTURED_VERTEX_ORTHO) * 54;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vbDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA vData = {};
    vData.pSysMem = pVB.data();
    vData.SysMemPitch = 0;
    vData.SysMemSlicePitch = 0;

    dx.CreateBuffer(&vbDesc, &vData, &_vertexBuffer, "DXFrame");
    
    _type = ControlType::Frame;
}

CDXFrame::~CDXFrame()
{
    if (_pText != nullptr)
    {
        delete _pText;
        _pText = nullptr;
    }
}

void CDXFrame::Render()
{
    if (_vertexBuffer == nullptr) return;

    uint32_t stride = sizeof(TEXTURED_VERTEX_ORTHO);
    uint32_t offset = 0;
    dx.SetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
    dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   	float16 wm = Math::Translation(_x, -_y, 0.0f);
    CConstantBuffers::SetWorld(dx, &wm);

    ID3D11ShaderResourceView* pRV = _texBackground.GetTextureRV();
    dx.SetShaderResources(0, 1, &pRV);
    CShaders::SelectOrthoShader();
    dx.Draw(54, 0);

    if (_pText != nullptr)
    {
        _pText->Render(_x + (_w - _textW) / 2.0f, _y + 3.0f);
    }

    for (auto* child : _childElements)
    {
        if (child != nullptr && child->GetVisible())
        {
            child->Render();
        }
    }
}

void CDXFrame::Init()
{
    uint32_t size = 0;
    uint8_t* pData = GetResource(IDB_FRAME, "PNG", &size);
    _texBackground.Init(pData, size, "FRAME");
}

void CDXFrame::Dispose()
{
    _texBackground.Dispose();
}

void CDXFrame::SetColours(int colour1, int colour2, int colour3, int colour4)
{
    if (_pText != nullptr)
    {
        _pText->SetColours(colour1, colour2, colour3, colour4);
    }

    for (auto* child : _childElements)
    {
        if (child != nullptr)
        {
            child->SetColours(colour1, colour2, colour3, colour4);
        }
    }
}
