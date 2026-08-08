#include "DXControl.h"
#include "Utilities.h"
#include "DirectX.h"

CDXControl::CDXControl() : CDXBase()
{
    _x = 0.0f;
    _y = 0.0f;
    _w = 0.0f;
    _h = 0.0f;

    _vertexBuffer = nullptr;
    _focus = false;
    _visible = true;

    _type = ControlType::Undefined;
    _mouseOver = false;
    _enabled = true;

    _alignment = Alignment::Default;
}

CDXControl::~CDXControl()
{
    if (_vertexBuffer != nullptr)
    {
        _vertexBuffer->Release();
        _vertexBuffer = nullptr;
    }
}

CDXControl* CDXControl::HitTest(float x, float y)
{
    return (_enabled && _visible && x >= _x && y >= _y && x < (_x + _w) && y < (_y + _h)) ? this : nullptr;
}

void CDXControl::SetVertex(TEXTURED_VERTEX_ORTHO* pVB, int index, float x, float y, float z, float u, float v)
{
    pVB[index].position = float3(x, y, z);
    pVB[index].texture = float2(u, v);
}

void CDXControl::SetQuadVertex(TEXTURED_VERTEX_ORTHO* pVB, int index, float x1, float x2, float y1, float y2, float u1, float u2, float v1, float v2)
{
    SetVertex(pVB, index * 6 + 0, x1, y1, -0.5f, u1, v1);
    SetVertex(pVB, index * 6 + 1, x2, y1, -0.5f, u2, v1);
    SetVertex(pVB, index * 6 + 2, x2, y2, -0.5f, u2, v2);

    SetVertex(pVB, index * 6 + 3, x1, y1, -0.5f, u1, v1);
    SetVertex(pVB, index * 6 + 4, x2, y2, -0.5f, u2, v2);
    SetVertex(pVB, index * 6 + 5, x1, y2, -0.5f, u1, v2);
}

CDXControl* CDXControl::GetCurrentMouseOver()
{
    return _mouseOver ? this : nullptr;
}