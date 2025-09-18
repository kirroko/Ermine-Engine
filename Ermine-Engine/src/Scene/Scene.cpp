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
#include "HierarchySystem.h"

namespace Ermine {
    Scene::Scene(const std::string& name) : m_Name(name) {
        EE_CORE_INFO("Created scene: {}", name);
    }

    Scene::~Scene() {
        Clear();
    }

    EntityID Scene::CreateEntity(const std::string& name) {
        EntityID entity = ECS::GetInstance().CreateEntity();

        // Add essential components
        ECS::GetInstance().AddComponent(entity, ObjectMetaData(name, "Untagged", true));
        ECS::GetInstance().AddComponent(entity, Transform());
        ECS::GetInstance().AddComponent(entity, HierarchyComponent());

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

    bool Scene::HasEntity(EntityID entity) const {
        return m_Entities.find(entity) != m_Entities.end();
    }

    std::vector<EntityID> Scene::GetRootEntities() const {
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
}