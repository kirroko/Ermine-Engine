/* Start Header ************************************************************************/
/*!
\file       AABB.h
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee@digipen.edu
\date       Oct 25, 2025
\brief      Axis-Aligned Bounding Box (AABB) structure for spatial culling and 
            collision detection. Provides efficient methods for bounds calculation,
            point encapsulation, and transformation operations.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "MathVector.h"
#include "Matrix4x4.h"
#include <limits>

namespace Ermine
{
    struct AABB
    {
        Vec3 min{ std::numeric_limits<float>::max(), 
                  std::numeric_limits<float>::max(), 
                  std::numeric_limits<float>::max() };
        Vec3 max{ std::numeric_limits<float>::lowest(), 
                  std::numeric_limits<float>::lowest(), 
                  std::numeric_limits<float>::lowest() };

        AABB() = default;
        AABB(const Vec3& minPoint, const Vec3& maxPoint)
            : min(minPoint), max(maxPoint) {}

        /**
         * @brief Get the center point of the AABB
         * @return Center position as Vec3
         */
        Vec3 GetCenter() const {
            return Vec3(
                (min.x + max.x) * 0.5f,
                (min.y + max.y) * 0.5f,
                (min.z + max.z) * 0.5f
            );
        }

        /**
         * @brief Get the half-extents of the AABB
         * @return Half-size in each dimension
         */
        Vec3 GetExtents() const {
            return Vec3(
                (max.x - min.x) * 0.5f,
                (max.y - min.y) * 0.5f,
                (max.z - min.z) * 0.5f
            );
        }

        /**
         * @brief Check if the AABB is valid (min <= max)
         * @return True if valid, false otherwise
         */
        bool IsValid() const {
            return min.x <= max.x && min.y <= max.y && min.z <= max.z;
        }

        /**
         * @brief Expand this AABB to include a point
         * @param point The point to encapsulate
         */
        void Encapsulate(const Vec3& point) {
            min.x = std::min(min.x, point.x);
            min.y = std::min(min.y, point.y);
            min.z = std::min(min.z, point.z);
            max.x = std::max(max.x, point.x);
            max.y = std::max(max.y, point.y);
            max.z = std::max(max.z, point.z);
        }

        /**
         * @brief Expand this AABB to include another AABB
         * @param other The AABB to encapsulate
         */
        void Encapsulate(const AABB& other) {
            if (!other.IsValid()) return; // Don't encapsulate invalid AABBs
            
            min.x = std::min(min.x, other.min.x);
            min.y = std::min(min.y, other.min.y);
            min.z = std::min(min.z, other.min.z);
            max.x = std::max(max.x, other.max.x);
            max.y = std::max(max.y, other.max.y);
            max.z = std::max(max.z, other.max.z);
        }

        /**
         * @brief Transform this AABB by a matrix
         * @param matrix The transformation matrix
         * @return A new transformed AABB
         */
        AABB Transform(const Mtx44& matrix) const;
    };
}