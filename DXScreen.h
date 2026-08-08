#pragma once

#include "DXControl.h"
#include "DXContainer.h"
#include "DXFont.h"
#include "Globals.h"
#include "ConstantBuffers.h"
#include <list>
#include <cstdint>

class CDXScreen : public CDXContainer
{
    friend class CDXControl;

public:
    CDXScreen();
    virtual ~CDXScreen();

    static void Init();
    static void Dispose();

    void ClearMouseOver();

    static float16 WorldMatrix;
    static float16 ViewMatrix;
    static float16 ProjectionMatrix;
    static float16 OrthoMatrix;

    static Math::vec3 UpVector;
    static Math::vec3 PositionVector;
    static Math::vec3 LookAtVector;

    static float FieldOfView;
    static float ScreenAspect;
};
