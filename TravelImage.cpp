#include "TravelImage.h"
#include "ShaderStructs.h"
#include "Globals.h"
#include "ConstantBuffers.h"
#include <cmath>

void CTravelImage::Render()
{
	if (Buffer != NULL)
	{
		uint32_t stride = sizeof(TEXTURED_VERTEX);
		uint32_t offset = 0;
		dx.SetVertexBuffers(0, 1, &Buffer, &stride, &offset);
		float16 wm = Math::Identity();
		CConstantBuffers::SetWorld(dx, &wm);
		ID3D11ShaderResourceView* pRV = Texture.GetTextureRV();
		dx.SetShaderResources(0, 1, &pRV);
		dx.Draw(4, 0);
	}
}

void CTravelImage::Render(float x, float y)
{
	if (Buffer != NULL)
	{
		uint32_t stride = sizeof(TEXTURED_VERTEX);
		uint32_t offset = 0;
		dx.SetVertexBuffers(0, 1, &Buffer, &stride, &offset);
		float16 wm = Math::Translation(std::floor(x) + 0.5f, std::floor(y), 0.0f);
		CConstantBuffers::SetWorld(dx, &wm);
		ID3D11ShaderResourceView* pRV = Texture.GetTextureRV();
		dx.SetShaderResources(0, 1, &pRV);
		dx.Draw(4, 0);
	}
}