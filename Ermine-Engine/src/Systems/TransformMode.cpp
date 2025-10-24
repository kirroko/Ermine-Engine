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

        // Get entity's world position first for logging
        Vec3 entityWorldPos;
        if (hierarchySystem) {
            entityWorldPos = hierarchySystem->GetWorldPosition(entity);
        } else {
            entityWorldPos = ecs.GetComponent<Transform>(entity).position;
        }

        // Handle mode for single entity with potential children
        switch (s_currentMode) {
        case TransformMode::Pivot: {
            // Use entity's world position (pivot point)
            EE_CORE_INFO("=== PIVOT MODE ===");
            EE_CORE_INFO("Entity {}: returning pivot at ({:.3f}, {:.3f}, {:.3f})", 
                        entity, entityWorldPos.x, entityWorldPos.y, entityWorldPos.z);
            return entityWorldPos;
        }

        case TransformMode::Center: {
            EE_CORE_INFO("=== CENTER MODE ===");
            EE_CORE_INFO("Entity {}: pivot is at ({:.3f}, {:.3f}, {:.3f})", 
                        entity, entityWorldPos.x, entityWorldPos.y, entityWorldPos.z);
            
            // If entity has children, use hierarchy center calculation
            if (ecs.HasComponent<HierarchyComponent>(entity)) {
                auto& hierarchy = ecs.GetComponent<HierarchyComponent>(entity);
                if (!hierarchy.children.empty() && hierarchySystem) {
                    Vec3 hierarchyCenter = hierarchySystem->CalculateHierarchyCenter(entity);
                    EE_CORE_INFO("Entity {} has {} children", entity, hierarchy.children.size());
                    EE_CORE_INFO("Calculated hierarchy center: ({:.3f}, {:.3f}, {:.3f})", 
                               hierarchyCenter.x, hierarchyCenter.y, hierarchyCenter.z);
                    
                    float distance = Vec3Length(hierarchyCenter - entityWorldPos);
                    EE_CORE_INFO("Distance from pivot to center: {:.3f} units", distance);
                    
                    return hierarchyCenter;
                } else {
                    EE_CORE_INFO("Entity {} has no children, using mesh center", entity);
                }
            }
            
            // For entities without children, calculate center based on mesh AABB
            if (ecs.HasComponent<Mesh>(entity)) {
                auto& mesh = ecs.GetComponent<Mesh>(entity);
                auto aabb = graphics::GeometryFactory::CalculateAABB(mesh);
                
                EE_CORE_INFO("Mesh AABB: min=({:.3f}, {:.3f}, {:.3f}), max=({:.3f}, {:.3f}, {:.3f})",
                            aabb.min.x, aabb.min.y, aabb.min.z, aabb.max.x, aabb.max.y, aabb.max.z);
                
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
                
                EE_CORE_INFO("World transform: pos=({:.3f}, {:.3f}, {:.3f}), scale=({:.3f}, {:.3f}, {:.3f})",
                            worldPos.x, worldPos.y, worldPos.z, worldScale.x, worldScale.y, worldScale.z);
                
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
                
                Vec3 meshCenter = Vec3(
                    (aabbMin.x + aabbMax.x) * 0.5f,
                    (aabbMin.y + aabbMax.y) * 0.5f,
                    (aabbMin.z + aabbMax.z) * 0.5f
                );
                
                EE_CORE_INFO("Calculated mesh center: ({:.3f}, {:.3f}, {:.3f})", 
                            meshCenter.x, meshCenter.y, meshCenter.z);
                
                float distance = Vec3Length(meshCenter - worldPos);
                EE_CORE_INFO("Distance from pivot to mesh center: {:.3f} units", distance);
                
                // For symmetric meshes centered at origin, this should equal worldPos
                if (distance < 0.001f) {
                    EE_CORE_WARN("Mesh appears to be centered at pivot (distance < 0.001) - this is correct for default cubes!");
                }
                
                return meshCenter;
            }
            
            EE_CORE_INFO("No mesh or children found, falling back to pivot position");
            
            // Fallback to entity position if no mesh
            return entityWorldPos;
        }
        }

        return Vec3(0, 0, 0);
    }
} // namespace Ermine::editor