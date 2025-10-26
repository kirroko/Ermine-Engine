/* Start Header ************************************************************************/
/*!
\file       TransformMode.cpp
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee@digipen.edu
\date       Jan 25, 2025
\brief      Implementation of transform mode helpers for gizmo positioning

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

        if (mode == TransformMode::Pivot) {
            // Return the entity's world position (pivot point)
            auto hierarchySystem = ecs.GetSystem<HierarchySystem>();
            return hierarchySystem->GetWorldPosition(entity);
        }
        else { // Center mode
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

        // Get the world position as a fallback
        auto hierarchySystem = ecs.GetSystem<HierarchySystem>();
        Vec3 worldPos = hierarchySystem->GetWorldPosition(entity);

        // Try to calculate geometric center from mesh/model bounds
        Vec3 minBounds(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        Vec3 maxBounds(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
        bool hasBounds = false;

        // Check for Mesh component (primitives)
        if (ecs.HasComponent<Mesh>(entity))
        {
            const auto& mesh = ecs.GetComponent<Mesh>(entity);
            if (mesh.kind == MeshKind::Primitive)
            {
                // For primitives, use their defined size
                Vec3 halfSize = mesh.primitive.size * 0.5f;
                minBounds = worldPos - halfSize;
                maxBounds = worldPos + halfSize;
                hasBounds = true;
            }
        }

        // Check for ModelComponent (imported meshes)
        if (!hasBounds && ecs.HasComponent<ModelComponent>(entity))
        {
            const auto& modelComp = ecs.GetComponent<ModelComponent>(entity);
            if (modelComp.m_model)
            {
                // Get model vertices and calculate bounds
                auto vertices = modelComp.m_model->GetMeshVertices();
                if (!vertices.empty())
                {
                    for (const auto& vertex : vertices)
                    {
                        // Transform vertices to world space
                        Vec3 worldVertex = worldPos + Vec3(vertex.x, vertex.y, vertex.z);
                        
                        minBounds.x = std::min(minBounds.x, worldVertex.x);
                        minBounds.y = std::min(minBounds.y, worldVertex.y);
                        minBounds.z = std::min(minBounds.z, worldVertex.z);
                        
                        maxBounds.x = std::max(maxBounds.x, worldVertex.x);
                        maxBounds.y = std::max(maxBounds.y, worldVertex.y);
                        maxBounds.z = std::max(maxBounds.z, worldVertex.z);
                    }
                    hasBounds = true;
                }
            }
        }

        // Return bounding box center if we have bounds, otherwise fallback to pivot
        if (hasBounds) {
            return Vec3(
                (minBounds.x + maxBounds.x) * 0.5f,
                (minBounds.y + maxBounds.y) * 0.5f,
                (minBounds.z + maxBounds.z) * 0.5f
            );
        }

        return worldPos; // Fallback to pivot if no mesh data
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

        // Calculate average of all entity positions
        Vec3 centerSum;
        int validCount = 0;

        for (auto entity : entities)
        {
            Vec3 pos = GetManipulationPosition(entity, mode);
            centerSum += pos;
            validCount++;
        }

        if (validCount > 0) {
            return centerSum / static_cast<float>(validCount);
        }

        return Vec3();
    }
}
