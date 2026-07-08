#pragma once
#include "DXBase.h"
#include "DirectX.h"

// Dummy structs and defines

struct D3D11_INPUT_ELEMENT_DESC {
    const char* SemanticName;
    uint32_t SemanticIndex;
    DXGI_FORMAT Format;
    uint32_t InputSlot;
    uint32_t AlignedByteOffset;
    int InputSlotClass;
    uint32_t InstanceDataStepRate;
};

#define D3D11_APPEND_ALIGNED_ELEMENT 0xffffffff // -1
#define D3D11_INPUT_PER_VERTEX_DATA 0
#define DXGI_FORMAT_R32G32B32_FLOAT 2
#define DXGI_FORMAT_R32G32_FLOAT 16
#define DXGI_FORMAT_R32G32B32A32_FLOAT 2

class CDXShader : public CDXBase
{
public:
	CDXShader(CDirectX* pDX, int resource, LPCSTR vsFunctionName, LPCSTR vsProfileName, LPCSTR psFunctionName, LPCSTR psProfileName, D3D11_INPUT_ELEMENT_DESC* ied, int numDescriptors);
	~CDXShader();

	void Dispose();

	GLuint _glProgramId;

	void Activate(CDirectX* pDX);
};
