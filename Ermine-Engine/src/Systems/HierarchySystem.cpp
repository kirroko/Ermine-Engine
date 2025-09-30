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

        // Create transformation matrices from position, rotation, scale
        Matrix4x4 translation, rotation, scale, localMatrix;
        Mtx44Identity(translation);
        Mtx44Identity(rotation);
        Mtx44Identity(scale);

        // Build local transform matrix
        Mtx44Translate(translation, transform.position.x, transform.position.y, transform.position.z);
        Mtx44SetFromQuaternion(rotation, transform.rotation);
        Mtx44Scale(scale, transform.scale.x, transform.scale.y, transform.scale.z);

        // Combine: Translation * Rotation * Scale
        localMatrix = translation * rotation * scale;

        // Calculate world transform
        if (hierarchy.parent != 0) {
            // Get parent's world transform
            auto& parentHierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(hierarchy.parent);
            auto& parentTransform = ECS::GetInstance().GetComponent<Transform>(hierarchy.parent);

            // World = Parent's World * Local
            transform.transform_matrix = parentTransform.transform_matrix * localMatrix;

            // Cache in hierarchy component if using the optimization
            hierarchy.worldTransform = transform.transform_matrix;
        }
        else {
            // Root entity - world transform = local transform
            transform.transform_matrix = localMatrix;
            hierarchy.worldTransform = localMatrix;
        }

        // Mark as clean
        hierarchy.worldTransformDirty = false;

        // Recursively update children
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