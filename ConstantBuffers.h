#pragma once

#include "DirectX.h"
#include "ShaderStructs.h"

class CConstantBuffers
{
public:
    static void SetVOP(CDirectX& dx, float16* view, float16* ortho, float16* projection);
    static void SetWorld(CDirectX& dx, float16* world);
    static void SetTexFont(CDirectX& dx, float4* colour1, float4* colour2, float4* colour3, float4* colour4);
    static void SetVisibility(CDirectX& dx, VisibilityBufferType visibility);
    static void SetTranslation(CDirectX& dx, TranslationBufferType translation);
    static void SetMultiColouredFont(CDirectX& dx, float4* colour1, float4* colour2, float4* colour3, float4* colour4, float4* colour5, float4* colour6);

    static void Setup2D(CDirectX& dx);
    static void Setup3D(CDirectX& dx);

    static void Dispose();

    static ID3D11Buffer* _vop;
    static ID3D11Buffer* _world;
    static ID3D11Buffer* _texFont;
    static ID3D11Buffer* _multiColouredFont;

    static ID3D11Buffer* _visibility;
    static ID3D11Buffer* _translation;

private:
    static void SetupVOP(CDirectX& pDX, float w, float h, float camera_x, float camera_y, float camera_z);
};
