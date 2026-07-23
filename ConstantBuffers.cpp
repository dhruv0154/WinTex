#include "ConstantBuffers.h"
#include <cmath>
#include <cstring>

namespace Math {
	struct vec3 {
		float x;
		float y;
		float z;
	};

	vec3 cross(vec3 a, vec3 b) { return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x}; }
    float dot(vec3 a, vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
    vec3 normalize(vec3 v) { 
		float l = std::sqrt(dot(v,v)); 
		return {v.x/l, v.y/l, v.z/l}; 
	}

    float16 Transpose(const float16& in) {
        float16 out;
        for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) 
				out.m[i*4+j] = in.m[j*4+i];
		}
        return out;
    }

    float16 LookAtLH(vec3 eye, vec3 at, vec3 up) {
        vec3 zaxis = normalize({at.x - eye.x, at.y - eye.y, at.z - eye.z});
        vec3 xaxis = normalize(cross(up, zaxis));
        vec3 yaxis = cross(zaxis, xaxis);
        return {
            xaxis.x, yaxis.x, zaxis.x, 0.0f,
            xaxis.y, yaxis.y, zaxis.y, 0.0f,
            xaxis.z, yaxis.z, zaxis.z, 0.0f,
            -dot(xaxis, eye), -dot(yaxis, eye), -dot(zaxis, eye), 1.0f
        };
    }

    float16 OrthographicLH(float w, float h, float zn, float zf) {
        return {
            2.0f/w, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f/h, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f/(zf-zn), 0.0f,
            0.0f, 0.0f, zn/(zn-zf), 1.0f
        };
    }

    float16 PerspectiveFovLH(float fovY, float aspect, float zn, float zf) {
        float yScale = 1.0f / std::tan(fovY / 2.0f);
        float xScale = yScale / aspect;
        return {
            xScale, 0.0f, 0.0f, 0.0f,
            0.0f, yScale, 0.0f, 0.0f,
            0.0f, 0.0f, zf/(zf-zn), 1.0f,
            0.0f, 0.0f, -zn*zf/(zf-zn), 0.0f
        };
    }
}

ID3D11Buffer* CConstantBuffers::_vop = nullptr;
ID3D11Buffer* CConstantBuffers::_world = nullptr;
ID3D11Buffer* CConstantBuffers::_texFont = nullptr;
ID3D11Buffer* CConstantBuffers::_multiColouredFont = nullptr;
ID3D11Buffer* CConstantBuffers::_visibility = nullptr;
ID3D11Buffer* CConstantBuffers::_translation = nullptr;

void CConstantBuffers::SetVOP(CDirectX& dx, float16* view, float16* ortho, float16* projection)
{
    if (_vop == nullptr)
    {
        D3D11_BUFFER_DESC matrixBufferDesc;
        matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        matrixBufferDesc.ByteWidth = sizeof(VOPBufferType);
        matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        dx.CreateBuffer(&matrixBufferDesc, nullptr, &_vop, "ViewOrthoProjection");
    }

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    dx.Map(_vop, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    VOPBufferType* dataPtr = (VOPBufferType*)mappedResource.pData;
    dataPtr->view = Math::Transpose(*view);
    dataPtr->ortho = Math::Transpose(*ortho);
    dataPtr->projection = Math::Transpose(*projection);
    dx.Unmap(_vop, 0);
	dx.VSSetConstantBuffers(0, 1, &_vop);
}

void CConstantBuffers::SetWorld(CDirectX& dx, float16* world)
{
	if (_world == nullptr)
	{
		// Create constant buffer
		D3D11_BUFFER_DESC matrixBufferDesc;
		matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		matrixBufferDesc.ByteWidth = sizeof(WorldBufferType);
		matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		dx.CreateBuffer(&matrixBufferDesc, nullptr, &_world, "World");
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	if (dx.Map(_world, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource) == 0)
	{
		WorldBufferType* dataPtr = (WorldBufferType*)mappedResource.pData;
		dataPtr->world = Math::Transpose(*world);
		dx.Unmap(_world, 0);
	}

	dx.VSSetConstantBuffers(1, 1, &_world);
}

void CConstantBuffers::SetMultiColouredFont(CDirectX& dx, float4* colour1, float4* colour2, float4* colour3, float4* colour4, float4* colour5, float4* colour6)
{
	if (_multiColouredFont == nullptr)
	{
		// Create constant buffer
		D3D11_BUFFER_DESC matrixBufferDesc;
		matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		matrixBufferDesc.ByteWidth = sizeof(MultiColouredFontBufferType);
		matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		dx.CreateBuffer(&matrixBufferDesc, nullptr, &_multiColouredFont, "Font");
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	dx.Map(_multiColouredFont, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	MultiColouredFontBufferType* dataPtr = (MultiColouredFontBufferType*)mappedResource.pData;
	dataPtr->colour1 = *colour1;
	dataPtr->colour2 = *colour2;
	dataPtr->colour3 = *colour3;
	dataPtr->colour4 = *colour4;
	dataPtr->colour5 = *colour5;
	dataPtr->colour6 = *colour6;
	dx.Unmap(_multiColouredFont, 0);

	dx.VSSetConstantBuffers(2, 1, &_multiColouredFont);
}

void CConstantBuffers::SetTexFont(CDirectX& dx, float4* colour1, float4* colour2, float4* colour3, float4* colour4)
{
	if (_texFont == nullptr)
	{
		// Create constant buffer
		D3D11_BUFFER_DESC matrixBufferDesc;
		matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		matrixBufferDesc.ByteWidth = sizeof(TexFontBufferType);
		matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		dx.CreateBuffer(&matrixBufferDesc, nullptr, &_texFont, "TexFont");
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	dx.Map(_texFont, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	TexFontBufferType* dataPtr = (TexFontBufferType*)mappedResource.pData;
	dataPtr->colour1 = *colour1;
	dataPtr->colour2 = *colour2;
	dataPtr->colour3 = *colour3;
	dataPtr->colour4 = *colour4;
	dx.Unmap(_texFont, 0);

	dx.VSSetConstantBuffers(4, 1, &_texFont);
}

void CConstantBuffers::SetVisibility(CDirectX& dx, VisibilityBufferType visibility)
{
	if (_visibility == nullptr)
	{
		// Create constant buffer
		D3D11_BUFFER_DESC matrixBufferDesc;
		matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		matrixBufferDesc.ByteWidth = sizeof(VisibilityBufferType);
		matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		dx.CreateBuffer(&matrixBufferDesc, nullptr, &_visibility, "Visibility");
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	dx.Map(_visibility, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	VisibilityBufferType* dataPtr = (VisibilityBufferType*)mappedResource.pData;
	memcpy(&dataPtr->visibility[0], &visibility.visibility[0], sizeof(visibility.visibility));
	dx.Unmap(_visibility, 0);

	dx.VSSetConstantBuffers(3, 1, &_visibility);
}

void CConstantBuffers::SetTranslation(CDirectX& dx, TranslationBufferType translation)
{
	if (_translation == nullptr)
	{
		// Create constant buffer
		D3D11_BUFFER_DESC matrixBufferDesc;
		matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		matrixBufferDesc.ByteWidth = sizeof(TranslationBufferType);
		matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		dx.CreateBuffer(&matrixBufferDesc, nullptr, &_translation, "Translation");
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	dx.Map(_translation, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	TranslationBufferType* dataPtr = (TranslationBufferType*)mappedResource.pData;
	memcpy(&dataPtr->translation[0], &translation.translation[0], sizeof(float4) * 256);
	dx.Unmap(_translation, 0);

	dx.VSSetConstantBuffers(5, 1, &_translation);
}

void CConstantBuffers::Dispose()
{
	if (_vop != nullptr)
	{
		delete _vop;
		_vop = nullptr;
	}

	if (_world != nullptr)
	{
		delete _world;
		_world = nullptr;
	}

	if (_multiColouredFont != nullptr)
	{
		delete _multiColouredFont;
		_multiColouredFont = nullptr;
	}

	if (_visibility != nullptr)
	{
		delete _visibility;
		_visibility = nullptr;
	}

	if (_translation != nullptr)
	{
		delete _translation;
		_translation = nullptr;
	}

	if (_texFont != nullptr)
	{
		delete _texFont;
		_texFont = nullptr;
	}
}

void CConstantBuffers::Setup2D(CDirectX& dx)
{
	float w = (float)dx.GetWidth();
	float h = (float)dx.GetHeight();

	SetupVOP(dx, w, h, w / 2, -h / 2, -10.0f);
	Math::vec3 up = {0.0f, 1.0f, 0.0f};
	float camera_x = w / 2;
	float camera_y = -h / 2;
	float camera_z = -10.0f;
	Math::vec3 position = {camera_x, camera_y, camera_z};
	Math::vec3 lookAt = {camera_x, camera_y, 1.0f};

	float16 vm = Math::LookAtLH(position, lookAt, up);
	float16 om = Math::OrthographicLH(w, h, 0.1f, 1000.0f);

	// For 2D, we put Ortho matrix in the Perspective slot so shaders using "Projection" get Ortho
	SetVOP(dx, &vm, &om, &om);
}

void CConstantBuffers::Setup3D(CDirectX& dx)
{
	float w = (float)dx.GetWidth();
	float h = (float)dx.GetHeight();

	SetupVOP(dx, w, h, 0.0f, 0.0f, 0.0f);
}

void CConstantBuffers::SetupVOP(CDirectX& dx, float w, float h, float camera_x, float camera_y, float camera_z)
{
	Math::vec3 up = {0.0f, 1.0f, 0.0f};
	Math::vec3 position = {camera_x, camera_y, camera_z};
	Math::vec3 lookAt = {camera_x, camera_y, 1.0f};

	float16 vm = Math::LookAtLH(position, lookAt, up);
	float16 om = Math::OrthographicLH(w, h, 0.1f, 1000.0f);

	float fieldOfView = 3.141592654f / 4.0f / 0.95f;
	float screenAspect = w / h;
	float16 pm = Math::PerspectiveFovLH(fieldOfView, screenAspect, 0.1f, 1000.0f);

	SetVOP(dx, &vm, &om, &pm);
}
