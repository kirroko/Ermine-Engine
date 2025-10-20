/* Start Header ************************************************************************/
/*!
\file       TransformMode.cpp
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu
\date       Sep 05, 2025
\brief      Implementation of transform mode functionality for 3D manipulation.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "TransformMode.h"
#include "Components.h"
#include "HierarchySystem.h"
#include "GeometryFactory.h"

namespace Ermine::editor {
    // Static member initialization
    TransformMode TransformModeManager::s_currentMode = TransformMode::Pivot;

    Vec3 TransformModeManager::GetManipulationPosition(const std::vector<EntityID>& entities) {
        if (entities.empty()) return Vec3(0, 0, 0);

        // Single entity - delegate to single entity version
        if (entities.size() == 1) {
            return GetManipulationPosition(entities[0]);
        }

        // Multiple entities
        switch (s_currentMode) {
        case TransformMode::Pivot: {
            // Use the first/primary selected entity's position
            return GetManipulationPosition(entities[0]);
        }

        case TransformMode::Center: {
            // Calculate combined bounds of all selected entities
            Vec3 minBounds(FLT_MAX, FLT_MAX, FLT_MAX);
            Vec3 maxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            bool foundAnyBounds = false;
            
            auto& ecs = ECS::GetInstance();
            auto hierarchySystem = ecs.GetSystem<HierarchySystem>();

            for (EntityID entity : entities) {
                if (!ecs.IsEntityValid(entity)) continue;
                
                // First, handle entities with a hierarchy of children
                if (ecs.HasComponent<HierarchyComponent>(entity)) {
                    auto& hierarchy = ecs.GetComponent<HierarchyComponent>(entity);
                    
                    // If the entity has children, use hierarchy center calculation
                    if (!hierarchy.children.empty() && hierarchySystem) {
                        // Get bounds from hierarchy center calculation
                        Vec3 hierarchyCenter = hierarchySystem->CalculateHierarchyCenter(entity);
                        
                        // Get approximate size of hierarchy (can be refined if needed)
                        float hierarchyRadius = 1.0f;  // Default radius
                        
                        // Expand bounds to include this hierarchy
                        minBounds.x = std::min(minBounds.x, hierarchyCenter.x - hierarchyRadius);
                        minBounds.y = std::min(minBounds.y, hierarchyCenter.y - hierarchyRadius);
                        minBounds.z = std::min(minBounds.z, hierarchyCenter.z - hierarchyRadius);
                        
                        maxBounds.x = std::max(maxBounds.x, hierarchyCenter.x + hierarchyRadius);
                        maxBounds.y = std::max(maxBounds.y, hierarchyCenter.y + hierarchyRadius);
                        maxBounds.z = std::max(maxBounds.z, hierarchyCenter.z + hierarchyRadius);
                        
                        foundAnyBounds = true;
                        continue; // Skip to next entity since we've handled this one's hierarchy
                    }
                }
                
                // For entities without children, get their world position
                Vec3 worldPos;
                if (hierarchySystem) {
                    worldPos = hierarchySystem->GetWorldPosition(entity);
                } else if (ecs.HasComponent<Transform>(entity)) {
                    auto& transform = ecs.GetComponent<Transform>(entity);
                    worldPos = transform.position;
                } else {
                    continue; // Skip entities without transform
                }
                
                // Get world scale for proper AABB calculation
                Vec3 worldScale(1.0f, 1.0f, 1.0f);
                if (hierarchySystem) {
                    worldScale = hierarchySystem->GetWorldScale(entity);
                } else if (ecs.HasComponent<Transform>(entity)) {
                    worldScale = ecs.GetComponent<Transform>(entity).scale;
                }
                
                // Default bounds for entities without a mesh
                float entityRadius = 0.5f;
                Vec3 entityMin = worldPos - Vec3(entityRadius, entityRadius, entityRadius);
                Vec3 entityMax = worldPos + Vec3(entityRadius, entityRadius, entityRadius);
                
                // If entity has a mesh, use its AABB
                if (ecs.HasComponent<Mesh>(entity)) {
                    auto& mesh = ecs.GetComponent<Mesh>(entity);
                    auto aabb = graphics::GeometryFactory::CalculateAABB(mesh);
                    
                    // Scale and translate AABB to world space
                    entityMin = worldPos + Vec3(
                        aabb.min.x * worldScale.x, 
                        aabb.min.y * worldScale.y, 
                        aabb.min.z * worldScale.z
                    );
                    entityMax = worldPos + Vec3(
                        aabb.max.x * worldScale.x, 
                        aabb.max.y * worldScale.y, 
                        aabb.max.z * worldScale.z
                    );
                }
                
                // Expand combined bounds to include this entity
                minBounds.x = std::min(minBounds.x, entityMin.x);
                minBounds.y = std::min(minBounds.y, entityMin.y);
                minBounds.z = std::min(minBounds.z, entityMin.z);
                
                maxBounds.x = std::max(maxBounds.x, entityMax.x);
                maxBounds.y = std::max(maxBounds.y, entityMax.y);
                maxBounds.z = std::max(maxBounds.z, entityMax.z);
                
                foundAnyBounds = true;
            }

            if (foundAnyBounds) {
                // Return center of combined bounds
                return Vec3(
                    (minBounds.x + maxBounds.x) * 0.5f,
                    (minBounds.y + maxBounds.y) * 0.5f,
                    (minBounds.z + maxBounds.z) * 0.5f
                );
            } else {
                // Fallback if we couldn't calculate bounds
                if (!entities.empty() && ecs.IsEntityValid(entities[0])) {
                    // Use first entity's position as fallback
                    if (hierarchySystem) {
                        return hierarchySystem->GetWorldPosition(entities[0]);
                    } else if (ecs.HasComponent<Transform>(entities[0])) {
                        return ecs.GetComponent<Transform>(entities[0]).position;
                    }
                }
                return Vec3(0, 0, 0);
            }
        }
        }

        return Vec3(0, 0, 0);
    }

    Vec3 TransformModeManager::GetManipulationPosition(EntityID entity) {
        auto& ecs = ECS::GetInstance();

        if (!ecs.IsEntityValid(entity) || !ecs.HasComponent<Transform>(entity)) {
            return Vec3(0, 0, 0);
        }

        auto hierarchySystem = ecs.GetSystem<HierarchySystem>();

        // Handle mode for single entity with potential children
        switch (s_currentMode) {
        case TransformMode::Pivot: {
            // Use entity's world position (pivot point)
            if (hierarchySystem) {
                return hierarchySystem->GetWorldPosition(entity);
            }
            else {
                auto& transform = ecs.GetComponent<Transform>(entity);
                return transform.position;
            }
        }

        case TransformMode::Center: {
            // If entity has children, use hierarchy center calculation
            if (ecs.HasComponent<HierarchyComponent>(entity)) {
                auto& hierarchy = ecs.GetComponent<HierarchyComponent>(entity);
                if (!hierarchy.children.empty() && hierarchySystem) {
                    return hierarchySystem->CalculateHierarchyCenter(entity);
                }
            }
            
            // For entities without children or if hierarchy system unavailable,
            // calculate center based on mesh AABB
            if (ecs.HasComponent<Mesh>(entity)) {
                auto& mesh = ecs.GetComponent<Mesh>(entity);
                auto aabb = graphics::GeometryFactory::CalculateAABB(mesh);
                
                // Get world position and scale for AABB transformation
                Vec3 worldPos;
                Vec3 worldScale(1.0f, 1.0f, 1.0f);
                
                if (hierarchySystem) {
                    worldPos = hierarchySystem->GetWorldPosition(entity);
                    worldScale = hierarchySystem->GetWorldScale(entity);
                } else {
                    auto& transform = ecs.GetComponent<Transform>(entity);
                    worldPos = transform.position;
                    worldScale = transform.scale;
                }
                
                // Calculate center of scaled AABB in world space
                Vec3 aabbMin = worldPos + Vec3(
                    aabb.min.x * worldScale.x, 
                    aabb.min.y * worldScale.y, 
                    aabb.min.z * worldScale.z
                );
                Vec3 aabbMax = worldPos + Vec3(
                    aabb.max.x * worldScale.x, 
                    aabb.max.y * worldScale.y, 
                    aabb.max.z * worldScale.z
                );
                
                return Vec3(
                    (aabbMin.x + aabbMax.x) * 0.5f,
                    (aabbMin.y + aabbMax.y) * 0.5f,
                    (aabbMin.z + aabbMax.z) * 0.5f
                );
            }
            
            // Fallback to entity position if no mesh
            if (hierarchySystem) {
                return hierarchySystem->GetWorldPosition(entity);
            }
            else {
                auto& transform = ecs.GetComponent<Transform>(entity);
                return transform.position;
            }
        }
        }

        return Vec3(0, 0, 0);
    }
} // namespace Ermine::editor