/* Start Header ************************************************************************/
/*!
\file       AABB.cpp
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu (30%)
\date       Oct 25, 2025
\brief      Axis-Aligned Bounding Box (AABB) transformation implementation

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "AABB.h"

namespace Ermine
{
    AABB AABB::Transform(const Mtx44& matrix) const
    {
        // Transform all 8 corners of the AABB and build new axis-aligned bounds
        Vec3 corners[8] = {
            Vec3(min.x, min.y, min.z),
            Vec3(max.x, min.y, min.z),
            Vec3(min.x, max.y, min.z),
            Vec3(max.x, max.y, min.z),
            Vec3(min.x, min.y, max.z),
            Vec3(max.x, min.y, max.z),
            Vec3(min.x, max.y, max.z),
            Vec3(max.x, max.y, max.z)
        };

        AABB result;
        for (int i = 0; i < 8; ++i)
        {
            // Transform corner (matrix * point)
            Vec3 transformed;
            transformed.x = matrix.m00 * corners[i].x + matrix.m01 * corners[i].y + 
                           matrix.m02 * corners[i].z + matrix.m03;
            transformed.y = matrix.m10 * corners[i].x + matrix.m11 * corners[i].y + 
                           matrix.m12 * corners[i].z + matrix.m13;
            transformed.z = matrix.m20 * corners[i].x + matrix.m21 * corners[i].y + 
                           matrix.m22 * corners[i].z + matrix.m23;

            result.Encapsulate(transformed);
        }

        return result;
    }
}