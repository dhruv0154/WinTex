#include "Elevation.h"
#include <cmath>
#include <algorithm>

float CElevation::ElevationModifier = 1.5f;
float CElevation::ElevationCheckModifier = 1.5f;

CElevation::CElevation(const Elevation* pElevationData)
{
    if (pElevationData == nullptr)
    {
        return;
    }

    _x1 = static_cast<float>(pElevationData->X1) / 65536.0f;
    _y1 = static_cast<float>(pElevationData->Y1) / 65536.0f;
    _z1 = static_cast<float>(pElevationData->Z1) / 65536.0f;
    _x2 = static_cast<float>(pElevationData->X2) / 65536.0f;
    _y2 = static_cast<float>(pElevationData->Y2) / 65536.0f;
    _z2 = static_cast<float>(pElevationData->Z2) / 65536.0f;

    _stepHeight = static_cast<float>(pElevationData->StepHeight) / 65536.0f;
    _stepWidth = static_cast<float>(pElevationData->StepWidth) / 65536.0f;

    _numberOfSteps = pElevationData->NumberOfSteps;
    _horizontalLength = static_cast<float>(pElevationData->HorizontalLength) / 65536.0f;
    _xDirection = static_cast<float>(pElevationData->DirectionX) / 65536.0f;
    _zDirection = static_cast<float>(pElevationData->DirectionZ) / 65536.0f;

    float halfStepWidthX = std::abs(_zDirection) * _stepWidth / 1.0f;
    float halfStepWidthZ = std::abs(_xDirection) * _stepWidth / 1.0f;

    _x11 = _x1 + halfStepWidthX;
    _x12 = _x1 - halfStepWidthX;
    _x21 = _x2 + halfStepWidthX;
    _x22 = _x2 - halfStepWidthX;
    _z11 = _z1 + halfStepWidthZ;
    _z12 = _z1 - halfStepWidthZ;
    _z21 = _z2 + halfStepWidthZ;
    _z22 = _z2 - halfStepWidthZ;

    _xMin = std::min({_x11, _x12, _x21, _x22});
    _xMax = std::max({_x11, _x12, _x21, _x22});
    _zMin = std::min({_z11, _z12, _z21, _z22});
    _zMax = std::max({_z11, _z12, _z21, _z22});
    _yMin = std::min(_y1, _y2);
    _yMax = std::max(_y1, _y2);
}

CElevation::~CElevation()
{
}

bool CElevation::IsPointInElevation(float x, float y, float z) const
{
    return (x >= _xMin && x <= _xMax && 
            z >= _zMin && z <= _zMax && 
            (y + ElevationCheckModifier) >= _yMin && 
            (y - ElevationCheckModifier) <= _yMax);
}

float CElevation::GetElevationFromXZPosition(float x, float z) const
{
    if (_horizontalLength == 0.0f || _numberOfSteps == 0)
    {
        return _y1 - ElevationModifier;
    }

    double p = (1.0 - std::abs(_zDirection * (_z21 - z) + _xDirection * (_x21 - x)) / _horizontalLength);
    double sp = 1.0 / _numberOfSteps;
    double t = p / sp;
    double t2 = static_cast<double>(static_cast<int>(t));
    double t3 = t - t2;

    if (t3 >= 0.2 && t3 < 0.5)
    {
        t2 += (t3 - 0.2) / 0.3;
    }
    else if (t3 >= 0.5)
    {
        t2 += 1.0;
    }

    double d = (_y1 + _stepHeight * t2) - ElevationModifier;

    return static_cast<float>(d);
}

float CElevation::GetClosestElevation(float y) const
{
    float dy1 = y - (_y1 - 1.0f);
    float dy2 = y - (_y2 - 1.0f);

    return (((dy1 * dy1) < (dy2 * dy2)) ? _y1 : _y2) - ElevationModifier;
}