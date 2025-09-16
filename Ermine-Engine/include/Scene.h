/* Start Header ************************************************************************/
/*!
\file       Scene.h
\author     Edwin Lee Zirui, edwinzirui.lee 2301299, edwinzirui.lee\@digipen.edu
\date       Jan 24, 2025
\brief      Updated components with modular material system

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "ECS.h"
#include <string>
#include <vector>
#include <unordered_set>

namespace Ermine {
    class Scene {
    private:
        std::string m_Name;
        std::unordered_set<EntityID> m_Entities;
        EntityID m_SelectedEntity = 0;

    public:
        explicit Scene(const std::string& name = "Untitled Scene");
        ~Scene();

        // Entity management
        EntityID CreateEntity(const std::string& name = "Entity");
        void DestroyEntity(EntityID entity);
        bool HasEntity(EntityID entity) const;

        // Hierarchy queries
        std::vector<EntityID> GetRootEntities() const;
        std::vector<EntityID> GetAllEntities() const;

        // Selection
        void SetSelectedEntity(EntityID entity) { m_SelectedEntity = entity; }
        EntityID GetSelectedEntity() const { return m_SelectedEntity; }

        // Scene properties
        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }

        // Cleanup
        void Clear();
        size_t GetEntityCount() const { return m_Entities.size(); }
    };
}