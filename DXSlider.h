#pragma once

#include "DXControl.h"
#include "DXText.h"
#include "Texture.h"
#include <cstdint>

class CDXSlider : public CDXControl
{
public:
    CDXSlider(const char* text, float minValue, float maxValue, float step, float* pValue, int precision, float sliderX);
    virtual ~CDXSlider() override;

    static void Init();
    static void Dispose();

    virtual void Render() override;

    void SetValue(float value) { if (_pValue) { *_pValue = value; UpdateValueText(); } }
    float GetValue() const { return _pValue ? *_pValue : 0.0f; }

    void UpdateValueText();
    void CalculateSliderPosition();
    virtual CDXControl* HitTest(float x, float y) override;
    void Drag(float x, float y);

    virtual void SetColours(int colour1, int colour2, int colour3, int colour4) override;

protected:
    static CTexture _sliderTexBackground;
    static CTexture _sliderTexSlider;
    static CTexture _sliderTexSliderMouseOver;
    CDXText* _pTLabel{nullptr};
    CDXText* _pTValue{nullptr};

    float _textX{0.0f};
    float _textY{0.0f};
    float _sliderX{0.0f};

    float _sliderPosition{0.0f};

    float* _pValue{nullptr};
    float _step{0.0f};
    int _precision{0};
    float _minimum{0.0f};
    float _maximum{0.0f};
};
