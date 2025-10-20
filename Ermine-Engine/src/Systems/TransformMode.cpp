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
            // Calculate combined bounds of all selected entities and their children
            Vec3 minBounds(FLT_MAX, FLT_MAX, FLT_MAX);
            Vec3 maxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            bool foundAnyBounds = false;
            
            auto& ecs = ECS::GetInstance();
            auto hierarchySystem = ecs.GetSystem<HierarchySystem>();
            
            // First pass: collect all entities including children in hierarchies
            std::vector<EntityID> allEntities;
            for (EntityID entity : entities) {
                if (!ecs.IsEntityValid(entity)) continue;
                
                // Add the entity itself
                allEntities.push_back(entity);
                
                // If it has a hierarchy, add all children too
                if (hierarchySystem && ecs.HasComponent<HierarchyComponent>(entity)) {
                    // Recursive lambda to collect all children
                    std::function<void(EntityID)> collectChildren = [&](EntityID e) {
                        if (ecs.HasComponent<HierarchyComponent>(e)) {
                            auto& hierarchy = ecs.GetComponent<HierarchyComponent>(e);
                            for (auto child : hierarchy.children) {
                                if (ecs.IsEntityValid(child)) {
                                    allEntities.push_back(child);
                                    collectChildren(child); // Recursively collect grandchildren
                                }
                            }
                        }
                    };
                    
                    collectChildren(entity);
                }
            }

            // Second pass: calculate bounds of all collected entities
            for (EntityID entity : allEntities) {
                // Get world position and scale
                Vec3 worldPos;
                Vec3 worldScale(1.0f, 1.0f, 1.0f);
                
                if (hierarchySystem) {
                    worldPos = hierarchySystem->GetWorldPosition(entity);
                    worldScale = hierarchySystem->GetWorldScale(entity);
                } 
                else if (ecs.HasComponent<Transform>(entity)) {
                    auto& transform = ecs.GetComponent<Transform>(entity);
                    worldPos = transform.position;
                    worldScale = transform.scale;
                }
                else {
                    continue; // Skip entities without transform
                }
                
                // Default entity bounds if no mesh
                float entityRadius = 0.5f;
                Vec3 entityMin = worldPos - Vec3(entityRadius, entityRadius, entityRadius);
                Vec3 entityMax = worldPos + Vec3(entityRadius, entityRadius, entityRadius);
                
                // If entity has mesh, use its AABB
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
                
                // Expand combined bounds
                minBounds.x = std::min(minBounds.x, entityMin.x);
                minBounds.y = std::min(minBounds.y, entityMin.y);
                minBounds.z = std::min(minBounds.z, entityMin.z);
                
                maxBounds.x = std::max(maxBounds.x, entityMax.x);
                maxBounds.y = std::max(maxBounds.y, entityMax.y);
                maxBounds.z = std::max(maxBounds.z, entityMax.z);
                
                foundAnyBounds = true;
            }

            if (foundAnyBounds) {
                // Calculate and return center of bounds
                Vec3 center(
                    (minBounds.x + maxBounds.x) * 0.5f,
                    (minBounds.y + maxBounds.y) * 0.5f,
                    (minBounds.z + maxBounds.z) * 0.5f
                );
                
                // Log the calculated center point for debugging
                EE_CORE_INFO("Center mode: calculated center at ({:.3f}, {:.3f}, {:.3f})",
                            center.x, center.y, center.z);
                
                return center;
            }
            
            // Fallback: use first entity's position
            if (!entities.empty() && ecs.IsEntityValid(entities[0])) {
                if (hierarchySystem) {
                    return hierarchySystem->GetWorldPosition(entities[0]);
                }
                else if (ecs.HasComponent<Transform>(entities[0])) {
                    return ecs.GetComponent<Transform>(entities[0]).position;
                }
            }
            
            // Ultimate fallback
            return Vec3(0, 0, 0);
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