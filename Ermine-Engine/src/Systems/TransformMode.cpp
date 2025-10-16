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
            // Calculate center of all selected entities
            Vec3 centerSum(0, 0, 0);
            int validCount = 0;

            auto& ecs = ECS::GetInstance();
            auto hierarchySystem = ecs.GetSystem<HierarchySystem>();

            for (EntityID entity : entities) {
                if (ecs.IsEntityValid(entity) && ecs.HasComponent<Transform>(entity)) {
                    Vec3 worldPos;
                    if (hierarchySystem) {
                        worldPos = hierarchySystem->GetWorldPosition(entity);
                    }
                    else {
                        // Fallback to local position if no hierarchy system
                        auto& transform = ecs.GetComponent<Transform>(entity);
                        worldPos = transform.position;
                    }

                    centerSum += worldPos;
                    validCount++;
                }
            }

            if (validCount > 0) {
                return centerSum / static_cast<float>(validCount);
            }
            else {
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
            // Calculate center of entity + all children
            if (hierarchySystem) {
                return hierarchySystem->CalculateHierarchyCenter(entity);
            }
            else {
                // Fallback: just use entity position if no hierarchy system
                auto& transform = ecs.GetComponent<Transform>(entity);
                return transform.position;
            }
        }
        }

        return Vec3(0, 0, 0);
    }
} // namespace Ermine::editor