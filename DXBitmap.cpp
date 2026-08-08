#include "DXBitmap.h"
#include "Globals.h"
#include "Utilities.h"
#include "DXScreen.h"
#include "ConstantBuffers.h"
#include <algorithm>
#include <vector>

CDXBitmap::CDXBitmap() : CDXControl()
{
    _type = ControlType::Bitmap;
}

CDXBitmap::CDXBitmap(const char* fileName, Alignment alignment) : CDXControl()
{
    _type = ControlType::Bitmap;
    _texture.Init(fileName);
    _alignment = alignment;
    Init();
}

CDXBitmap::CDXBitmap(uint8_t* pImage, uint32_t size, Alignment alignment) : CDXControl()
{
    _type = ControlType::Bitmap;
    _texture.Init(pImage, size, "BITMAP");
    _alignment = alignment;
    Init();
}

CDXBitmap::CDXBitmap(int width, int height, Alignment alignment) : CDXControl()
{
    _type = ControlType::Bitmap;
    _texture.Init(width, height);
    _alignment = alignment;
    Init();
}

CDXBitmap::~CDXBitmap()
{
    if (_vertexBuffer != nullptr)
    {
        _vertexBuffer->Release();
        _vertexBuffer = nullptr;
    }
}

void CDXBitmap::Init()
{
    _w = static_cast<float>(_texture.Width());
    _h = static_cast<float>(_texture.Height());

    double sw = dx.GetWidth();
    double sh = dx.GetHeight();
    float imageRatioX = static_cast<float>(_w / sw);
    float imageRatioY = static_cast<float>(_h / sh);

    float imageRatio = ((_alignment & Alignment::Crop) != Alignment::Default) ? 
        std::min(imageRatioX, imageRatioY) : std::max(imageRatioX, imageRatioY);
        
    if (imageRatio > 1.0f || ((_alignment & Alignment::Scale) != Alignment::Default))
    {
        _w /= imageRatio;
        _h /= imageRatio;
    }

    std::vector<TEXTURED_VERTEX_ORTHO> pVB(6);

    float x1 = 0.0f;
    float x2 = x1 + _w;
    float y1 = 0.0f;
    float y2 = y1 - _h;

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

    dx.CreateBuffer(&vbDesc, &vData, &_vertexBuffer, "DXBitmap");
}

void CDXBitmap::Render()
{
    if (_vertexBuffer == nullptr) return;

    ID3D11ShaderResourceView* pRV = _texture.GetTextureRV();

    uint32_t stride = sizeof(TEXTURED_VERTEX_ORTHO);
    uint32_t offset = 0;
    dx.SetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
    dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    float16 wm = Math::Translation(_x, -_y, -2.0f);

    CConstantBuffers::SetWorld(dx, &wm);
    dx.SetShaderResources(0, 1, &pRV);
    CShaders::SelectOrthoShader();
    dx.Draw(6, 0);
}
