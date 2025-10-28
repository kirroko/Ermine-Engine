/* Start Header ************************************************************************/
/*!
\file       TransformMode.cpp
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee@digipen.edu
\date       Jan 25, 2025
\brief      Implementation of transform mode helpers for gizmo positioning
            Refactored to use AABBHelper for efficient AABB calculations

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "TransformMode.h"
#include "Components.h"
#include "ECS.h"
#include "HierarchySystem.h"
#include "Model.h"
#include "AABB.h"
#include "AABBHelper.h"

namespace Ermine
{
    /**
     * @brief Calculate the position where the gizmo should appear
     * @param entity The entity to manipulate
     * @param mode Current transform mode
     * @return Position for gizmo placement
     */
    Vec3 TransformModeHelper::GetManipulationPosition(EntityID entity, TransformMode mode)
    {
        auto& ecs = ECS::GetInstance();
        if (!ecs.IsEntityValid(entity)) return Vec3();

        auto hierarchySystem = ecs.GetSystem<HierarchySystem>();

        if (mode == TransformMode::Pivot) {
            // Return the entity's world position (pivot point)
            return hierarchySystem->GetWorldPosition(entity);
        }
        else { // Center mode
            // Return the entity's geometric center
            return CalculateGeometricCenter(entity);
        }
    }

    /**
     * @brief Calculate the geometric center of an entity's mesh/model
     * @param entity The entity to calculate center for
     * @return Bounding box center in world space
     */
    Vec3 TransformModeHelper::CalculateGeometricCenter(EntityID entity)
    {
        auto& ecs = ECS::GetInstance();
        if (!ecs.IsEntityValid(entity)) return Vec3();

        // Use AABBHelper to calculate world-space AABB
        AABB worldAABB = AABBHelper::CalculateWorldAABB(entity);
        
        // Return center if valid, otherwise fallback to pivot
        if (worldAABB.IsValid()) {
            return worldAABB.GetCenter();
        }
        
        // Fallback: return pivot position if no mesh data
        auto hierarchySystem = ecs.GetSystem<HierarchySystem>();
        return hierarchySystem->GetWorldPosition(entity);
    }

    /**
     * @brief Calculate center position for multiple selected entities
     * @param entities List of selected entities
     * @param mode Current transform mode
     * @return Average position based on mode
     */
    Vec3 TransformModeHelper::GetMultiSelectionCenter(const std::vector<EntityID>& entities, TransformMode mode)
    {
        if (entities.empty()) return Vec3();
        if (entities.size() == 1) return GetManipulationPosition(entities[0], mode);

        auto& ecs = ECS::GetInstance();
        auto hierarchySystem = ecs.GetSystem<HierarchySystem>();

        if (mode == TransformMode::Pivot)
        {
            // Pivot mode: Use the first entity's pivot
            // NOTE: Unity uses the LAST selected entity, but we don't track selection order yet
            return GetManipulationPosition(entities[0], mode);
        }
        else // Center mode
        {
            // Calculate combined bounding box of ALL selected entities
            AABB combinedAABB;
            bool hasValidAABB = false;
            
            for (auto entityId : entities)
            {
                if (!ecs.IsEntityValid(entityId)) continue;
                
                // Use AABBHelper for each entity
                AABB entityAABB = AABBHelper::CalculateWorldAABB(entityId);
                if (entityAABB.IsValid()) {
                    combinedAABB.Encapsulate(entityAABB);
                    hasValidAABB = true;
                }
            }

            if (hasValidAABB) {
                return combinedAABB.GetCenter();
            }

            // Fallback: average of all entity pivots
            Vec3 centerSum;
            int validCount = 0;
            for (auto entityId : entities)
            {
                if (ecs.IsEntityValid(entityId))
                {
                    centerSum += hierarchySystem->GetWorldPosition(entityId);
                    validCount++;
                }
            }

            return (validCount > 0) ? centerSum / static_cast<float>(validCount) : Vec3();
        }
    }
}
