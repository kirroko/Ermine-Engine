#pragma once
#include "PreCompile.h"
#include "MathVector.h"

namespace Ermine
{
    struct MaterialData {
        Vec3 Ka, Kd, Ks;
        float Shininess;
        int useBF;
    };

    struct LightGPU
    {
        Vec4 position_type;     // xyz = position (view space), w = type (0=POINT,1=DIRECTIONAL,2=SPOT)
        Vec4 color_intensity;   // rgb = color, a = intensity
        Vec4 direction_range;   // xyz = direction (view space, normalized), w = range
        Vec4 spot_angles;       // x = innerCos, y = outerCos, z,w = padding
    };

    enum class LightType
    {
        POINT = 0,
        DIRECTIONAL = 1,
        SPOT = 2
	};

}