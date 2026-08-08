#include "DXCheckBox.h"
#include "Globals.h"
#include "Utilities.h"
#include "DXScreen.h"
#include "ConstantBuffers.h"
#include <algorithm>
#include <vector>

CTexture CDXCheckBox::_cbTexBackground;
CTexture CDXCheckBox::_cbTexMouseOver;
CTexture CDXCheckBox::_cbTexChecked;

CDXCheckBox::CDXCheckBox(const char* text, bool* pValue, float width) : CDXControl()
{
    _textX = 0.0f;
    _textY = 0.0f;

    _pChecked = pValue;

    _pText = new CDXText();
    _pText->SetText(text);
    _pText->SetColours(0xffffffff);

    _x = 0.0f;
    _y = 0.0f;
    
    _w = std::max(_pText->PixelWidth(text) + 79.0f, width);
    _h = 32.0f;

    std::vector<TEXTURED_VERTEX_ORTHO> pVB(6);

    float x1 = _w - 32.0f;
    float x2 = _w;
    float y1 = 0.0f;
    float y2 = -32.0f;

    SetQuadVertex(pVB.data(), 0, x1, x2, y1, y2, 0.0f, 1.0f, 0.0f, 1.0f);

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

    dx.CreateBuffer(&vbDesc, &vData, &_vertexBuffer, text);

    _h = std::max(40.0f, _h);

    _type = ControlType::CheckBox;
}

CDXCheckBox::~CDXCheckBox()
{
    if (_pText != nullptr)
    {
        delete _pText;
        _pText = nullptr;
    }
}

void CDXCheckBox::Init()
{
    uint32_t s1 = 0, s2 = 0, s3 = 0;
    uint8_t* p1 = GetResource(IDB_BUTTON, "PNG", &s1);
    uint8_t* p2 = GetResource(IDB_BUTTON_MOUSEOVER, "PNG", &s2);
    uint8_t* p3 = GetResource(IDB_CHECKMARK, "PNG", &s3);

    _cbTexBackground.Init(p1, s1, "CHECKBOX1");
    _cbTexMouseOver.Init(p2, s2, "CHECKBOX2");
    _cbTexChecked.Init(p3, s3, "CHECKBOX3");
}

void CDXCheckBox::Dispose()
{
    _cbTexBackground.Dispose();
    _cbTexMouseOver.Dispose();
    _cbTexChecked.Dispose();
}

void CDXCheckBox::Render()
{
    if (_vertexBuffer == nullptr) return;

    uint32_t stride = sizeof(TEXTURED_VERTEX_ORTHO);
    uint32_t offset = 0;
    dx.SetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);

    float16 wm = Math::Translation(_x, -_y, 0.0f);
    CConstantBuffers::SetWorld(dx, &wm);

    ID3D11ShaderResourceView* pRV = _mouseOver ? _cbTexMouseOver.GetTextureRV() : _cbTexBackground.GetTextureRV();
    dx.SetShaderResources(0, 1, &pRV);
    CShaders::SelectOrthoShader();
    dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dx.Draw(6, 0);

    if (_pChecked && *_pChecked)
    {
        pRV = _cbTexChecked.GetTextureRV();
        dx.SetShaderResources(0, 1, &pRV);
        dx.Draw(6, 0);
    }

    if (_pText != nullptr)
    {
        _pText->Render(_textX + _x, _y + 8.0f);
    }
}

void CDXCheckBox::MouseEnter()
{
    if (_enabled)
    {
        SetMouseOver(true);
    }
}

void CDXCheckBox::MouseLeave()
{
    if (_enabled)
    {
        SetMouseOver(false);
    }
}

void CDXCheckBox::MouseButtonUp()
{
    if (_enabled && _mouseOver && _pChecked != nullptr)
    {
        *_pChecked = !(*_pChecked);
    }
}

void CDXCheckBox::SetColours(int colour1, int colour2, int colour3, int colour4)
{
    if (_pText != nullptr)
    {
        _pText->SetColours(colour1, colour2, colour3, colour4);
    }
}