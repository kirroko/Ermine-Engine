/* Start Header ************************************************************************/
/*!
\file       Scene.cpp
\author     Edwin Lee Zirui, edwinzirui.lee 2301299, edwinzirui.lee\@digipen.edu
\date       Jan 24, 2025
\brief      Updated components with modular material system

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "Scene.h"
#include "Components.h"
#include "ECS.h"
#include "HierarchySystem.h"
#include "Physics.h"
#include <algorithm>
#include <cctype>

namespace Ermine {
    Scene::Scene(const std::string& name) : m_Name(name) {
        EE_CORE_INFO("Created scene: {}", name);
    }

    Scene::~Scene() {
        //Clear();
    }

    EntityID Scene::CreateEntity(const std::string& name, bool needsTransform, bool needsHierarchy) {
        EntityID entity = ECS::GetInstance().CreateEntity();

        // Add essential components
        ECS::GetInstance().AddComponent(entity, ObjectMetaData(name, "Untagged", true));
        if (needsTransform) {
            ECS::GetInstance().AddComponent(entity, Transform());
        }

        if (needsHierarchy) {
            ECS::GetInstance().AddComponent(entity, HierarchyComponent());
            
            // Initialize GlobalTransform for hierarchy entities
            auto hierarchySystem = ECS::GetInstance().GetSystem<HierarchySystem>();
            if (hierarchySystem) {
                hierarchySystem->EnsureGlobalTransform(entity);
                hierarchySystem->InitializeEntity(entity);
            }
        }

        m_Entities.insert(entity);
        EE_CORE_TRACE("Created entity {} in scene {}", entity, m_Name);
        return entity;
    }

    void Scene::DestroyEntity(EntityID entity) {
        if (!HasEntity(entity)) return;

        auto hierarchySystem = ECS::GetInstance().GetSystem<HierarchySystem>();

        // Get children before destroying to avoid accessing invalid data
        std::vector<EntityID> children;
        if (ECS::GetInstance().HasComponent<HierarchyComponent>(entity)) {
            const auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
            children = hierarchy.children;
        }

        // Unparent this entity
        hierarchySystem->UnsetParent(entity);

        // Unparent all children (make them root entities)
        for (auto child : children) {
            hierarchySystem->UnsetParent(child);
        }

        // Remove from scene
        m_Entities.erase(entity);

        // Clear selection if this was selected
        if (m_SelectedEntity == entity) {
            m_SelectedEntity = 0;
        }

        ECS::GetInstance().DestroyEntity(entity);
        EE_CORE_TRACE("Destroyed entity {} from scene {}", entity, m_Name);
    }

    EntityID Scene::DuplicateEntity(EntityID sourceEntity) {
        if (!HasEntity(sourceEntity)) {
            EE_CORE_WARN("Cannot duplicate entity {} - not in scene {}", sourceEntity, m_Name);
            return 0;
        }

        auto& ecs = ECS::GetInstance();
        auto hierarchySystem = ecs.GetSystem<HierarchySystem>();

        // Clone the entity (copies all components)
        EntityID newEntity = ecs.CloneEntity(sourceEntity);

        // Update metadata name with Unity-style numbering
        if (ecs.HasComponent<ObjectMetaData>(newEntity)) {
            auto& meta = ecs.GetComponent<ObjectMetaData>(newEntity);
            
            // Extract base name (strip existing numbering if present)
            std::string baseName = meta.name;
            
            // Remove existing " (n)" pattern if present
            size_t openParen = baseName.rfind(" (");
            if (openParen != std::string::npos) {
                size_t closeParen = baseName.find(')', openParen);
                if (closeParen != std::string::npos) {
                    // Check if content between parentheses is a number
                    std::string numberStr = baseName.substr(openParen + 2, closeParen - openParen - 2);
                    bool isNumber = !numberStr.empty() && 
                                    std::all_of(numberStr.begin(), numberStr.end(), ::isdigit);
                    
                    if (isNumber) {
                        baseName = baseName.substr(0, openParen);
                    }
                }
            }
            
            // Find the highest existing number for this base name
            int highestNumber = 0;
            for (auto entity : m_Entities) {
                if (!ecs.IsEntityValid(entity) || !ecs.HasComponent<ObjectMetaData>(entity)) 
                    continue;
                
                const auto& existingMeta = ecs.GetComponent<ObjectMetaData>(entity);
                
                // Check if name starts with baseName
                if (existingMeta.name.find(baseName) == 0) {
                    // Check for " (n)" pattern
                    size_t pos = existingMeta.name.rfind(" (");
                    if (pos != std::string::npos && pos == baseName.length()) {
                        size_t closePos = existingMeta.name.find(')', pos);
                        if (closePos != std::string::npos) {
                            std::string numStr = existingMeta.name.substr(pos + 2, closePos - pos - 2);
                            if (!numStr.empty() && std::all_of(numStr.begin(), numStr.end(), ::isdigit)) {
                                int num = std::stoi(numStr);
                                highestNumber = std::max(highestNumber, num);
                            }
                        }
                    }
                }
            }
            
            // Set new name with next number
            meta.name = baseName + " (" + std::to_string(highestNumber + 1) + ")";
        }

        // Preserve hierarchy relationship (make duplicate a sibling of original)
        if (ecs.HasComponent<HierarchyComponent>(sourceEntity)) {
            auto& sourceHierarchy = ecs.GetComponent<HierarchyComponent>(sourceEntity);
            
            // If source has a parent, set same parent for duplicate
            if (sourceHierarchy.parent != 0 && ecs.IsEntityValid(sourceHierarchy.parent)) {
                hierarchySystem->SetParent(newEntity, sourceHierarchy.parent, false);
            }
            // Otherwise duplicate is also root (default from CloneEntity)
        }

        // Initialize transform system
        if (ecs.HasComponent<HierarchyComponent>(newEntity)) {
            hierarchySystem->EnsureGlobalTransform(newEntity);
            hierarchySystem->MarkDirty(newEntity);
        }

        // Update physics if needed
        if (ecs.HasComponent<PhysicComponent>(newEntity)) {
            ecs.GetSystem<Physics>()->UpdatePhysicList();
        }

        // Add to scene
        m_Entities.insert(newEntity);

        EE_CORE_INFO("Duplicated entity {} ('{}') to {} ('{}') in scene {}", 
            sourceEntity,
            ecs.GetComponent<ObjectMetaData>(sourceEntity).name,
            newEntity,
            ecs.GetComponent<ObjectMetaData>(newEntity).name,
            m_Name);

        return newEntity;
    }

    bool Scene::HasEntity(EntityID entity) const {
        return m_Entities.find(entity) != m_Entities.end();
    }

    std::vector<EntityID> Scene::GetRootEntities() const {
        EnsureSyncedWithECS();
        std::vector<EntityID> roots;

        for (auto entity : m_Entities) {
            if (!ECS::GetInstance().IsEntityValid(entity)) continue;

            if (ECS::GetInstance().HasComponent<HierarchyComponent>(entity)) {
                const auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
                if (hierarchy.parent == 0) {
                    roots.push_back(entity);
                }
            }
        }

        return roots;
    }

    void Scene::SetSelectedEntity(EntityID entity) {
        // Validate entity exists in this scene
        if (entity != 0 && !HasEntity(entity)) {
            EE_CORE_WARN("Trying to select entity {} that doesn't exist in scene {}", entity, m_Name);
            return;
        }

        EntityID previousSelection = m_SelectedEntity;
        m_SelectedEntity = entity;

        if (previousSelection != entity) {
            EE_CORE_TRACE("Selected entity {} in scene {}", entity, m_Name);
        }
    }

    std::vector<EntityID> Scene::GetAllEntities() const {
        EnsureSyncedWithECS();
        return std::vector<EntityID>(m_Entities.begin(), m_Entities.end());
    }

    void Scene::Clear() {
        // Create a copy since DestroyEntity modifies m_Entities
        std::vector<EntityID> entitiesToDestroy(m_Entities.begin(), m_Entities.end());

        for (auto entity : entitiesToDestroy) {
            ECS::GetInstance().DestroyEntity(entity);
        }

        m_Entities.clear();
        m_SelectedEntity = 0;
        EE_CORE_INFO("Cleared scene: {}", m_Name);
    }
    void Scene::EnsureSyncedWithECS(bool force) const
    {
        auto& ecs = ECS::GetInstance();

        // Count how many ECS entities *should* be in the scene snapshot
        size_t ecsCount = 0;
        for (EntityID e = 0; e < MAX_ENTITIES; ++e)
            if (ecs.IsEntityValid(e) && ecs.HasComponent<HierarchyComponent>(e))
                ++ecsCount;

        // Rebuild if forced or if snapshot is out-of-date
        if (!force && !m_Entities.empty() && m_Entities.size() == ecsCount)
            return;

        m_Entities.clear();

        for (EntityID e = 0; e < MAX_ENTITIES; ++e) {
            if (!ecs.IsEntityValid(e)) continue;
            if (!ecs.HasComponent<HierarchyComponent>(e)) continue;

            auto& hc = ecs.GetComponent<HierarchyComponent>(e);
            if (hc.parent != 0 &&
                (!ecs.IsEntityValid(hc.parent) || !ecs.HasComponent<HierarchyComponent>(hc.parent))) {
                hc.parent = 0;
                hc.depth = 0;
            }
            m_Entities.insert(e);
        }
    }
}