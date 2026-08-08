#pragma once

struct float2 { 
	float x, y;
	float2() = default;
	float2(float x, float y) : x(x), y(y) {}
};
struct float3 { 
	float x, y, z;
	float3() = default;
	float3(float x, float y, float z) : x(x), y(y), z(z) {}
};
struct float4 { 
	float x, y, z, w;
	float4() = default;
	float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};
struct float16 { 
	float m[16]; 
	float16 operator*(const float16& a) {
    	float16 R;
    	for(int i=0; i<4; i++) {
    	    for(int j=0; j<4; j++) {
    	       R.m[i*4+j] = this->m[i*4+0]*a.m[0*4+j] +
                             this->m[i*4+1]*a.m[1*4+j] +
                             this->m[i*4+2]*a.m[2*4+j] +
                             this->m[i*4+3]*a.m[3*4+j];
    	    }
    	}
    	return R;
	}
};

struct TEXTURED_VERTEX_ORTHO
{
	float3 position;
	float2 texture;
};

struct TEXTURED_VERTEX
{
	float3 position;
	float2 texture;
	float2 object;			// Use for visibility, then create a visibility buffer for the objects and lookup in the shader
	float4 objectParameters;	// Use for triangle transparency indicator in shader (some textures are used both as opaque and transparent)
};

struct MULTICOLOURED_FONT_VERTEX
{
	float3 position;
	float2 texture;
	float4 colour;
};

struct COLOURED_VERTEX
{
	float4 position;
	float4 colour;
	float4 object;
};

struct COLOURED_VERTEX_ORTHO
{
	float4 position;
	float4 colour;
};

struct VOPBufferType
{
	float16 view;
	float16 ortho;
	float16 projection;
};

struct WorldBufferType
{
	float16 world;
};

struct MultiColouredFontBufferType
{
	float4 colour1;
	float4 colour2;
	float4 colour3;
	float4 colour4;
	float4 colour5;
	float4 colour6;
};

struct TexFontBufferType
{
	float4 colour1;
	float4 colour2;
	float4 colour3;
	float4 colour4;
};

struct VisibilityBufferType
{
	float4 visibility[4096];
};

struct TranslationBufferType
{
	float4 translation[256];
};
