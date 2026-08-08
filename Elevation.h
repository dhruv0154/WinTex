#pragma once

#include <cstdint>

struct Elevation
{
    int32_t Type;
    int32_t Id;
    int32_t Trigger;
    int32_t X1;
    int32_t Y1;
    int32_t Z1;
    int32_t X2;
    int32_t Y2;
    int32_t Z2;
    int32_t StepHeight;
    int32_t StepLength;
    int32_t StepWidth;
    int32_t NumberOfSteps;
    int32_t HorizontalLength;
    int32_t DirectionX;
    int32_t DirectionZ;
};

class CElevation
{
public:
    explicit CElevation(const Elevation* pElevationData);
    ~CElevation();

    bool IsPointInElevation(float x, float y, float z) const;
    float GetElevationFromXZPosition(float x, float z) const;
    float GetClosestElevation(float y) const;

    static float ElevationModifier;
    static float ElevationCheckModifier;

protected:
    float _x1{0.0f};
    float _y1{0.0f};
    float _z1{0.0f};
    float _x2{0.0f};
    float _y2{0.0f};
    float _z2{0.0f};
    float _stepHeight{0.0f};
    float _stepLength{0.0f};
    float _stepWidth{0.0f};
    int _numberOfSteps{0};
    float _horizontalLength{0.0f};
    float _xDirection{0.0f};
    float _zDirection{0.0f};

    float _x11{0.0f};
    float _x12{0.0f};
    float _x21{0.0f};
    float _x22{0.0f};
    float _z11{0.0f};
    float _z12{0.0f};
    float _z21{0.0f};
    float _z22{0.0f};

    float _xMin{0.0f};
    float _xMax{0.0f};
    float _yMin{0.0f};
    float _yMax{0.0f};
    float _zMin{0.0f};
    float _zMax{0.0f};
};