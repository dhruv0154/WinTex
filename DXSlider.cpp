#include "DXSlider.h"
#include "Globals.h"
#include "Utilities.h"
#include "DXScreen.h"
#include "ConstantBuffers.h"
#include <iomanip>
#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <cmath>

CTexture CDXSlider::_sliderTexBackground;
CTexture CDXSlider::_sliderTexSlider;
CTexture CDXSlider::_sliderTexSliderMouseOver;

CDXSlider::CDXSlider(const char* text, float minValue, float maxValue, float step, float* pValue, int precision, float sliderX) : CDXControl()
{
    _textX = 0.0f;
    _precision = precision;
    _pValue = pValue;
    _w = 112.0f;
    _h = TexFont.Height() * pConfig->FontScale;
    _minimum = minValue;
    _maximum = maxValue;
    _step = step;
    _sliderX = sliderX;

    CalculateSliderPosition();

    _pTLabel = new CDXText();
    _pTLabel->SetText(text);
    _pTLabel->SetColours(0xffffffff);

    _pTValue = new CDXText();
    _pTValue->SetColours(0xffffffff);
    _pTValue->Width(_w);
    UpdateValueText();

    std::vector<TEXTURED_VERTEX_ORTHO> pVB(12);

    float x1 = _sliderX;
    float x2 = x1 + _w;
    SetQuadVertex(pVB.data(), 0, x1, x2, 0.0f, -32.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    SetQuadVertex(pVB.data(), 1, 0.0f, 6.0f, 0.0f, -26.0f, 0.0f, 1.0f, 0.0f, 1.0f);

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.ByteWidth = sizeof(TEXTURED_VERTEX_ORTHO) * 12;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vbDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA vData = {};
    vData.pSysMem = pVB.data();
    vData.SysMemPitch = 0;
    vData.SysMemSlicePitch = 0;

    dx.CreateBuffer(&vbDesc, &vData, &_vertexBuffer, text);

    _type = ControlType::Slider;
}

CDXSlider::~CDXSlider()
{
    if (_pTLabel != nullptr)
    {
        delete _pTLabel;
        _pTLabel = nullptr;
    }

    if (_pTValue != nullptr)
    {
        delete _pTValue;
        _pTValue = nullptr;
    }
}

void CDXSlider::Init()
{
    uint32_t s1 = 0, s2 = 0, s3 = 0;
    uint8_t* p1 = GetResource(IDB_SLIDER, "PNG", &s1);
    uint8_t* p2 = GetResource(IDB_BUTTON, "PNG", &s2);
    uint8_t* p3 = GetResource(IDB_BUTTON_MOUSEOVER, "PNG", &s3);

    _sliderTexBackground.Init(p1, s1, "SLIDER1");
    _sliderTexSlider.Init(p2, s2, "SLIDER2");
    _sliderTexSliderMouseOver.Init(p3, s3, "SLIDER3");
}

void CDXSlider::Dispose()
{
    _sliderTexBackground.Dispose();
    _sliderTexSlider.Dispose();
    _sliderTexSliderMouseOver.Dispose();
}

void CDXSlider::Render()
{
    if (_vertexBuffer == nullptr) return;

    uint32_t stride = sizeof(TEXTURED_VERTEX_ORTHO);
    uint32_t offset = 0;
    dx.SetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
    dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    float16 wm = Math::Translation(0.0f, -_y, 0.0f);
    CConstantBuffers::SetWorld(dx, &wm);

    ID3D11ShaderResourceView* pRV = _sliderTexBackground.GetTextureRV();
    dx.SetShaderResources(0, 1, &pRV);
    CShaders::SelectOrthoShader();
    dx.Draw(6, 0);

    wm = Math::Translation(_sliderX + _sliderPosition, -4.0f - _y, 0.0f);
    CConstantBuffers::SetWorld(dx, &wm);

    pRV = _mouseOver ? _sliderTexSliderMouseOver.GetTextureRV() : _sliderTexSlider.GetTextureRV();
    dx.SetShaderResources(0, 1, &pRV);
    dx.Draw(6, 6);

    if (_pTLabel != nullptr)
    {
        _pTLabel->Render(_textX + _x, _y + 8.0f);
    }
    if (_pTValue != nullptr)
    {
        _pTValue->Render(_textX + _sliderX, _y + 32.0f);
    }
}

void CDXSlider::UpdateValueText()
{
    if (_pTValue == nullptr || _pValue == nullptr) return;

    Rect rc{ 0, 0, (int)TexFont.Height(), (int)_pTValue->Width() };
    std::stringstream stream;
    stream << std::fixed << std::setprecision(_precision) << *_pValue;
    std::string data = stream.str();
    
    float w = _pTValue->Width();
    _pTValue->SetText(data.c_str(), rc, CDXText::Alignment::Center);
    _pTValue->Width(w);
}

void CDXSlider::CalculateSliderPosition()
{
    float val = (_pValue != nullptr) ? *_pValue : _minimum;
    float value = std::max(_minimum, std::min(_maximum, val));
    float span = _maximum - _minimum;
    
    if (span > 0.0f)
    {
        _sliderPosition = 106.0f * (value - _minimum) / span;
    }
}

CDXControl* CDXSlider::HitTest(float x, float y)
{
    return (_visible && _enabled && x >= (_sliderX + 2.0f + _sliderPosition) && y >= (_y + 6.0f) && 
            x < (_sliderX + 8.0f + _sliderPosition) && y < (_y + 30.0f)) ? this : nullptr;
}

void CDXSlider::Drag(float x, float y)
{
    float span = _maximum - _minimum;
    if (span > 0.0f && _step > 0.0f && _pValue != nullptr)
    {
        float min_x_pos = _sliderX + 2.0f;
        float max_x_pos = _sliderX + 108.0f;

        float pos_x = std::max(min_x_pos, std::min(max_x_pos, x)) - min_x_pos;
        int positions = 1 + static_cast<int>(span / _step);
        
        float pixels_per_step = 106.0f / static_cast<float>(positions - 1);
        int pos_index = static_cast<int>(pos_x / pixels_per_step);
        
        _sliderPosition = (106.0f * static_cast<float>(pos_index)) / static_cast<float>(positions - 1);
        
        *_pValue = _minimum + _step * static_cast<float>(pos_index);
        UpdateValueText();
    }
}

void CDXSlider::SetColours(int colour1, int colour2, int colour3, int colour4)
{
    if (_pTLabel != nullptr)
    {
        _pTLabel->SetColours(colour1, colour2, colour3, colour4);
    }
    if (_pTValue != nullptr)
    {
        _pTValue->SetColours(colour1, colour2, colour3, colour4);
    }
}