/* Start Header ************************************************************************/
/*!
\file       HierarchySystem.cpp
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu
\date       Sep 05, 2025
\brief      Implementation of HierarchySystem.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "HierarchySystem.h"
#include "ECS.h"

namespace Ermine
{
    /**
     * @brief Checks if setting parent would create a cycle in the hierarchy.
     * @param[in] child The entity to be reparented.
     * @param[in] parent The entity to be set as parent.
     * @return True if a cycle would be created, false otherwise.
    */
    bool HierarchySystem::WouldCreateCycle(EntityID child, EntityID parent) const
    {
        EntityID current = parent;
        while (current != 0) {
            if (current == child) return true;
            current = GetParent(current);
        }
        return false;
    }

    /**
     * @brief Sets the parent of an entity, updating hierarchy and depth.
     * @param[in] child The entity to set the parent for.
     * @param[in] parent The entity to set as parent.
    */
    void HierarchySystem::SetParent(EntityID child, EntityID parent)
    {
        if (!ECS::GetInstance().IsEntityValid(child) || !ECS::GetInstance().IsEntityValid(parent))
            return;

        if (WouldCreateCycle(child, parent))
            return; // Prevent cycles

        auto& childHierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(child);
        auto& parentHierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(parent);

        if (childHierarchy.parent != 0)
            UnsetParent(child);

        childHierarchy.parent = parent;
        childHierarchy.depth = parentHierarchy.depth + 1;
        childHierarchy.isDirty = true;
        parentHierarchy.children.push_back(child);
    }

    /**
     * @brief Removes the parent of an entity, updating hierarchy and depth.
     * @param[in] child The entity to unset the parent for.
    */
    void HierarchySystem::UnsetParent(EntityID child)
    {
        if (!ECS::GetInstance().IsEntityValid(child))
            return;

        auto& childHierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(child);

        if (childHierarchy.parent != 0)
        {
            auto& parentHierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(childHierarchy.parent);
            auto it = std::find(parentHierarchy.children.begin(), parentHierarchy.children.end(), child);
            if (it != parentHierarchy.children.end())
                parentHierarchy.children.erase(it);

            childHierarchy.parent = 0;
            childHierarchy.depth = 0;
            childHierarchy.isDirty = true;
        }
    }

    /**
     * @brief Recursively updates world transforms for an entity and its children.
     * @param[in] entity The root entity to start updating from.
    */
    void HierarchySystem::UpdateWorldTransform(EntityID entity)
    {
        auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);

        if (hierarchy.parent != 0) {
            auto& parentTransform = ECS::GetInstance().GetComponent<Transform>(hierarchy.parent);
            transform.transform_matrix = parentTransform.transform_matrix * transform.transform_matrix;
        }
        for (auto child : hierarchy.children) {
            UpdateWorldTransform(child);
        }
    }

    /**
     * @brief Updates the hierarchy for all root entities in the system.
    */
    void HierarchySystem::UpdateHierarchy()
    {
        for (auto entity : m_Entities)
        {
            auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
            if (hierarchy.parent == 0) // Only update roots
                UpdateWorldTransform(entity);
        }
    }

    /**
     * @brief Gets the parent of an entity.
     * @param[in] entity The entity to query.
     * @return The parent entity ID, or 0 if none.
    */
    EntityID HierarchySystem::GetParent(EntityID entity) const
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return 0;
        return ECS::GetInstance().GetComponent<HierarchyComponent>(entity).parent;
    }

    /**
      * @brief Gets the children of an entity.
      * @param[in] entity The entity to query.
      * @return Reference to a vector of child entity IDs.
     */
    const std::vector<EntityID>& HierarchySystem::GetChildren(EntityID entity) const
    {
        static std::vector<EntityID> empty;
        if (!ECS::GetInstance().IsEntityValid(entity))
            return empty;
        return ECS::GetInstance().GetComponent<HierarchyComponent>(entity).children;
    }
}