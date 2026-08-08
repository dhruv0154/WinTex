#include "DXButton.h"
#include "Globals.h"
#include "Utilities.h"
#include "DXScreen.h"
#include "GameController.h"
#include "ConstantBuffers.h"
#include <vector>

CTexture CDXButton::_texBackground;
CTexture CDXButton::_texMouseOver;
CDXSound* CDXButton::_pSound = nullptr;

int CDXButton::Colour1 = 0;
int CDXButton::Colour2 = -1;
int CDXButton::Colour3 = -1;
int CDXButton::Colour4 = 0;

CDXButton::CDXButton(const char* text, float w, float h, void(*onClick)(void*), void* data) : CDXControl()
{
    _clicked = onClick;
    _data = data;

    _pText = new CDXText();
    _pText->SetText(text);

    float textW = _pText->PixelWidth(text);

    _x = 0.0f;
    _y = 0.0f;
    _w = ((w > 0.0f) ? w : textW);
    _h = 40.0f * pConfig->FontScale;

    std::vector<TEXTURED_VERTEX_ORTHO> pVB(54);

    float x1 = 0.0f;
    float x2 = 16.0f * pConfig->FontScale;
    float x3 = 16.0f * pConfig->FontScale + _w;
    float x4 = x3 + 16.0f * pConfig->FontScale;
    float y1 = 0.0f;
    float y2 = -16.0f * pConfig->FontScale;
    float y3 = -24.0f * pConfig->FontScale;
    float y4 = -40.0f * pConfig->FontScale;

    float u1 = 0.0f;
    float u2 = 0.25f;
    float u3 = 0.75f;
    float u4 = 1.0f;
    float v1 = 1.0f;
    float v2 = 0.75f;
    float v3 = 0.25f;
    float v4 = 0.0f;

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

    dx.CreateBuffer(&vbDesc, &vData, &_vertexBuffer, text);

    _textX = 16.0f * pConfig->FontScale - 10.0f + (_w - textW) / 2.0f;
    _w += 32.0f * pConfig->FontScale;

    _type = ControlType::Button;
}

CDXButton::~CDXButton()
{
    if (_pText != nullptr)
    {
        delete _pText;
        _pText = nullptr;
    }
}

void CDXButton::Render()
{
    if (_vertexBuffer == nullptr) return;

    uint32_t stride = sizeof(TEXTURED_VERTEX_ORTHO);
    uint32_t offset = 0;
    dx.SetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);

    float16 wm = Math::Translation(_x, -_y, 0.0f);
    CConstantBuffers::SetWorld(dx, &wm);

    ID3D11ShaderResourceView* pRV = _mouseOver ? _texMouseOver.GetTextureRV() : _texBackground.GetTextureRV();
    dx.SetShaderResources(0, 1, &pRV);
    CShaders::SelectOrthoShader();
    dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    dx.Draw(54, 0);

    if (_pText != nullptr)
    {
        int mask = (_mouseOver && _enabled) ? 0xffffffff : 0x80ffffff;
        _pText->SetColours(Colour1 & mask, Colour2 & mask, Colour3 & mask, Colour4 & mask);
        float fontHeight = TexFont.Height();
        _pText->Render(_textX + _x, _y + fontHeight * pConfig->FontScale);
    }
}

void CDXButton::MouseEnter()
{
    if (_enabled)
    {
        SetMouseOver(true);
    }
}

void CDXButton::MouseMove()
{
}

void CDXButton::MouseLeave()
{
    if (_enabled)
    {
        SetMouseOver(false);
    }
}

void CDXButton::MouseButtonDown()
{
}

void CDXButton::MouseButtonUp()
{
    if (_enabled && _mouseOver)
    {
        Click();
    }
}

void CDXButton::KeyDown()
{
}

void CDXButton::KeyUp()
{
}

void CDXButton::GotFocus()
{
}

void CDXButton::LostFocus()
{
}

void CDXButton::Init()
{
    uint32_t s1 = 0, s2 = 0;
    uint8_t* p1 = GetResource(IDB_BUTTON, "PNG", &s1);
    uint8_t* p2 = GetResource(IDB_BUTTON_MOUSEOVER, "PNG", &s2);

    _texBackground.Init(p1, s1, "BUTTON1");
    _texMouseOver.Init(p2, s2, "BUTTON2");

    _pSound = new CDXSound();
}

void CDXButton::Dispose()
{
    if (_pSound != nullptr)
    {
        delete _pSound;
        _pSound = nullptr;
    }

    _texBackground.Dispose();
    _texMouseOver.Dispose();
}

void CDXButton::SetMouseOver(bool mouseOver)
{
    bool oldMouseOver = _mouseOver;
    _mouseOver = mouseOver;

    if (mouseOver && !oldMouseOver && _pSound != nullptr)
    {
        uint32_t s = 0;
        uint8_t* pWave = GetResource(IDR_WAVE_BUTTON_MOUSEOVER, "WAVE", &s);
        _pSound->Play(pWave, s);
    }
}

void CDXButton::Click()
{
    if (_clicked != nullptr)
    {
        _clicked(_data);
    }
}

void CDXButton::SetButtonColours(int colour1, int colour2, int colour3, int colour4)
{
    Colour1 = colour1;
    Colour2 = colour2;
    Colour3 = colour3;
    Colour4 = colour4;
}