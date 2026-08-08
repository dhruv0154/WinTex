#include "DXImageButton.h"
#include "Globals.h"
#include "Utilities.h"
#include "ConstantBuffers.h"
#include <vector>

CTexture CDXImageButton::_ibTexBackground;
CTexture CDXImageButton::_ibTexMouseOver;

CDXImageButton::CDXImageButton(int img, void(*onClick)(void* data)) : CDXButton()
{
    _clicked = onClick;

    _x = 0.0f;
    _y = 0.0f;
    _w = 32.0f;
    _h = 32.0f;

    std::vector<TEXTURED_VERTEX_ORTHO> pVB(6);

    float x1 = 0.0f;
    float x2 = 32.0f;
    float y1 = 0.0f;
    float y2 = -32.0f;

    float u1 = 0.0f;
    float u2 = 0.5f;
    float v1 = 0.0f;
    float v2 = 1.0f;

    if (img & 1)
    {
        u1 += 0.5f;
        u2 += 0.5f;
    }

    if (!(img & 2))
    {
        pVB[0].position = { x1, y1, -0.5f };
        pVB[0].texture  = { u1, v1 };

        pVB[1].position = { x2, y1, -0.5f };
        pVB[1].texture  = { u2, v1 };

        pVB[2].position = { x2, y2, -0.5f };
        pVB[2].texture  = { u2, v2 };

        pVB[3].position = { x1, y1, -0.5f };
        pVB[3].texture  = { u1, v1 };

        pVB[4].position = { x2, y2, -0.5f };
        pVB[4].texture  = { u2, v2 };

        pVB[5].position = { x1, y2, -0.5f };
        pVB[5].texture  = { u1, v2 };
    }
    else
    {
        pVB[0].position = { x1, y1, -0.5f };
        pVB[0].texture  = { u1, v2 };

        pVB[1].position = { x2, y1, -0.5f };
        pVB[1].texture  = { u1, v1 };

        pVB[2].position = { x2, y2, -0.5f };
        pVB[2].texture  = { u2, v1 };

        pVB[3].position = { x1, y1, -0.5f };
        pVB[3].texture  = { u1, v2 };

        pVB[4].position = { x2, y2, -0.5f };
        pVB[4].texture  = { u2, v1 };

        pVB[5].position = { x1, y2, -0.5f };
        pVB[5].texture  = { u2, v2 };
    }

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.ByteWidth = sizeof(TEXTURED_VERTEX_ORTHO) * 6;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vbDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA vData = {};
    vData.pSysMem = pVB.data();
    vData.SysMemPitch = 0;
    vData.SysMemSlicePitch = 0;

    dx.CreateBuffer(&vbDesc, &vData, &_vertexBuffer, "DXImageButton");

    _type = ControlType::ImageButton;
}

CDXImageButton::~CDXImageButton()
{
}

void CDXImageButton::Render()
{
    if (_vertexBuffer == nullptr) return;

    uint32_t stride = sizeof(TEXTURED_VERTEX_ORTHO);
    uint32_t offset = 0;
    dx.SetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);

    float16 wm = Math::Translation(_x, -_y, 0.0f);
    CConstantBuffers::SetWorld(dx, &wm);

    ID3D11ShaderResourceView* pRV = _mouseOver ? _ibTexMouseOver.GetTextureRV() : _ibTexBackground.GetTextureRV();
    dx.SetShaderResources(0, 1, &pRV);
    CShaders::SelectOrthoShader();
    dx.Draw(6, 0);
}

void CDXImageButton::Init()
{
    uint32_t s1 = 0, s2 = 0;
    uint8_t* p1 = GetResource(IDB_IMAGEBUTTON, "PNG", &s1);
    uint8_t* p2 = GetResource(IDB_IMAGEBUTTON_MOUSEOVER, "PNG", &s2);

    _ibTexBackground.Init(p1, s1, "IMAGEBUTTON1");
    _ibTexMouseOver.Init(p2, s2, "IMAGEBUTTON2");
}

void CDXImageButton::Dispose()
{
    _ibTexBackground.Dispose();
    _ibTexMouseOver.Dispose();
}
