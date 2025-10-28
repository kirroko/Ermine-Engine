/* Start Header ************************************************************************/
/*!
\file       AABBHelper.h
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee@digipen.edu
\date       Oct 25, 2025
\brief      Helper utility class for calculating Axis-Aligned Bounding Boxes (AABB)
            from various components. Provides functions to compute local and world-space
            bounds for entities with meshes, models, and hierarchical transforms.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "AABB.h"
#include "Components.h"
#include "ECS.h"

namespace Ermine
{
    class AABBHelper
    {
    public:
        /**
         * @brief Calculate world-space AABB for an entity
         * Handles Mesh, ModelComponent, and hierarchy transforms
         */
        static AABB CalculateWorldAABB(EntityID entity);

        /**
         * @brief Calculate local-space AABB from primitive mesh
         */
        static AABB CalculateLocalAABB(const Mesh& mesh);

        /**
         * @brief Calculate local-space AABB from imported model
         */
        static AABB CalculateLocalAABB(const ModelComponent& model);

    private:
        static Vec3 TransformVertex(const Vec3& vertex, 
                                   const Vec3& pos, 
                                   const Quaternion& rot, 
                                   const Vec3& scale);
    };
}