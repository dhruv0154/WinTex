#pragma once

#include "DirectX.h"
#include "ShaderStructs.h"

namespace Math {
	struct vec3 {
		float x;
		float y;
		float z;
	};

	inline vec3 cross(vec3 a, vec3 b) { return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x}; }
    inline float dot(vec3 a, vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
    inline vec3 normalize(vec3 v) { 
		float l = std::sqrt(dot(v,v)); 
		return {v.x/l, v.y/l, v.z/l}; 
	}

    inline float16 Transpose(const float16& in) {
        return in;
    }

    inline float16 LookAtLH(vec3 eye, vec3 at, vec3 up) {
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

    inline float16 OrthographicLH(float w, float h, float zn, float zf) {
        return {
            2.0f/w, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f/h, 0.0f, 0.0f,
            0.0f, 0.0f, 2.0f/(zf-zn), 0.0f,
            0.0f, 0.0f, -(zf+zn)/(zf-zn), 1.0f
        };
    }

    inline float16 PerspectiveFovLH(float fovY, float aspect, float zn, float zf) {
        float yScale = 1.0f / std::tan(fovY / 2.0f);
        float xScale = yScale / aspect;
        return {
            xScale, 0.0f, 0.0f, 0.0f,
            0.0f, yScale, 0.0f, 0.0f,
            0.0f, 0.0f, (zf+zn)/(zf-zn), 1.0f,
            0.0f, 0.0f, -(2.0f*zn*zf)/(zf-zn), 0.0f
        };
    }

	inline float16 Identity() {
        return {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
    }

    inline float16 Translation(float x, float y, float z) {
        return {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            x,    y,    z,    1.0f
        };
    }

    inline float16 Scaling(float x, float y, float z) {
        return {
            x,    0.0f, 0.0f, 0.0f,
            0.0f, y,    0.0f, 0.0f,
            0.0f, 0.0f, z,    0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
    }

    inline float16 RotationZ(float angle) {
        float16 M = Identity();
        float c = std::cos(angle);
        float s = std::sin(angle);
        M.m[0*4+0] = c;  M.m[0*4+1] = s;
        M.m[1*4+0] = -s; M.m[1*4+1] = c;
        return M;
    }

    inline float16 RotationX(float angle) {
        float16 M = Identity();
        float c = std::cos(angle);
        float s = std::sin(angle);
        M.m[1*4+1] = c;  M.m[1*4+2] = s;
        M.m[2*4+1] = -s; M.m[2*4+2] = c;
        return M;
    }

    inline float16 RotationY(float angle) {
        float16 M = Identity();
        float c = std::cos(angle);
        float s = std::sin(angle);
        M.m[0*4+0] = c;  M.m[0*4+2] = -s;
        M.m[2*4+0] = s;  M.m[2*4+2] = c;
        return M;
    }

    namespace TriangleTests {
        inline bool Intersects(vec3 origin, vec3 direction, vec3 v0, vec3 v1, vec3 v2, float& dist) { 
            vec3 e1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
            vec3 e2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
            vec3 p = cross(direction, e2);
            float det = dot(e1, p);

            if (det > -0.000001f && det < 0.000001f) return false;
            float invDet = 1.0f / det;

            vec3 t = {origin.x - v0.x, origin.y - v0.y, origin.z - v0.z};
            float u = dot(t, p) * invDet;
            if (u < 0.0f || u > 1.0f) return false;

            vec3 q = cross(t, e1);
            float v = dot(direction, q) * invDet;
            if (v < 0.0f || u + v > 1.0f) return false;

            dist = dot(e2, q) * invDet;
            if (dist > 0.000001f) return true;

            return false;
        }
    }
}

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
