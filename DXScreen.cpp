#include "DXScreen.h"
#include "Globals.h"
#include "DXBitmap.h"
#include "DXButton.h"
#include "Utilities.h"
#include <cmath>

float16 CDXScreen::WorldMatrix;
float16 CDXScreen::ViewMatrix;
float16 CDXScreen::ProjectionMatrix;
float16 CDXScreen::OrthoMatrix;

Math::vec3 CDXScreen::UpVector;
Math::vec3 CDXScreen::PositionVector;
Math::vec3 CDXScreen::LookAtVector;

float CDXScreen::FieldOfView = 0.0f;
float CDXScreen::ScreenAspect = 0.0f;

CDXScreen::CDXScreen() : CDXContainer()
{
}

CDXScreen::~CDXScreen()
{
}

void CDXScreen::Init()
{
    uint32_t size = 0;
    uint8_t* pFont = GetResource(isUAKM ? IDB_FONT_UAKM : IDB_FONT_PD, "PNG", &size);
    TexFont.Init(pFont, size);

    UpVector = { 0.0f, 1.0f, 0.0f };
    float x = static_cast<float>(dx.GetWidth()) / 2.0f;
    float y = static_cast<float>(dx.GetHeight()) / 2.0f;
    
    PositionVector = { x, y, -10.0f };
    LookAtVector   = { x, y, 1.0f };

    ViewMatrix  = Math::LookAtLH(PositionVector, LookAtVector, UpVector);
    OrthoMatrix = Math::OrthographicLH(static_cast<float>(dx.GetWidth()), static_cast<float>(dx.GetHeight()), 0.1f, 1000.0f);

    FieldOfView  = 3.141592654f / 4.0f;
    ScreenAspect = static_cast<float>(dx.GetWidth()) / static_cast<float>(dx.GetHeight());

    ProjectionMatrix = Math::PerspectiveFovLH(FieldOfView, ScreenAspect, 0.1f, 1000.0f);
}

void CDXScreen::Dispose()
{
    TexFont.Dispose();
}

void CDXScreen::ClearMouseOver()
{
    for (auto* child : _childElements)
    {
        if (child != nullptr)
        {
            child->SetMouseOver(false);
        }
    }
}
