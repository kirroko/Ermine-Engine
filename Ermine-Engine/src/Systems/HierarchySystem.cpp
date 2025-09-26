/* Start Header ************************************************************************/
/*!
\file       HierarchySystem.cpp
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu
\date       Sep 05, 2025
\brief      Implementation of HierarchySystem with proper transform propagation.

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
     * @brief Marks an entity and all its children as dirty for transform updates.
     * @param[in] entity The entity to mark dirty.
    */
    void HierarchySystem::MarkTransformDirty(EntityID entity)
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return;

        auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);

        // Only propagate if not already dirty (optimization)
        if (hierarchy.worldTransformDirty)
            return;

        hierarchy.worldTransformDirty = true;

        // Recursively mark all children as dirty
        for (auto child : hierarchy.children) {
            MarkTransformDirty(child);
        }
    }

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

        // Remove from old parent if exists
        if (childHierarchy.parent != 0)
            UnsetParent(child);

        // Set new parent
        childHierarchy.parent = parent;
        childHierarchy.depth = parentHierarchy.depth + 1;
        parentHierarchy.children.push_back(child);

        // Mark child and all descendants as dirty since hierarchy changed
        MarkTransformDirty(child);
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

            // Mark as dirty since world transform will change
            MarkTransformDirty(child);
        }
    }

    /**
     * @brief Computes local transform matrix from position, rotation, scale.
     * @param[in] transform The transform component to compute from.
     * @return The computed local transform matrix.
    */
    Matrix4x4 HierarchySystem::ComputeLocalMatrix(const Transform& transform)
    {
        Matrix4x4 translation, rotation, scale, localMatrix;
        Mtx44Identity(translation);
        Mtx44Identity(rotation);
        Mtx44Identity(scale);

        // Build transform components
        Mtx44Translate(translation, transform.position.x, transform.position.y, transform.position.z);
        Mtx44SetFromQuaternion(rotation, transform.rotation);
        Mtx44Scale(scale, transform.scale.x, transform.scale.y, transform.scale.z);

        // Combine: Translation * Rotation * Scale (TRS order)
        localMatrix = translation * rotation * scale;
        return localMatrix;
    }

    /**
     * @brief Recursively updates world transforms for an entity and its children.
     * @param[in] entity The root entity to start updating from.
     * @param[in] parentWorld Optional parent world transform matrix.
    */
    void HierarchySystem::UpdateWorldTransform(EntityID entity, const Matrix4x4* parentWorld)
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return;

        auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);

        // Skip if already up to date
        if (!hierarchy.worldTransformDirty && parentWorld == nullptr)
            return;

        // Compute local transform
        Matrix4x4 localMatrix = ComputeLocalMatrix(transform);

        // Calculate world transform
        if (parentWorld != nullptr) {
            // Child: World = Parent's World * Local
            transform.transform_matrix = (*parentWorld) * localMatrix;
        }
        else if (hierarchy.parent != 0) {
            // Has parent but no parent matrix provided - get it
            auto& parentTransform = ECS::GetInstance().GetComponent<Transform>(hierarchy.parent);
            transform.transform_matrix = parentTransform.transform_matrix * localMatrix;
        }
        else {
            // Root entity - world = local
            transform.transform_matrix = localMatrix;
        }

        // Cache world transform in hierarchy component
        hierarchy.worldTransform = transform.transform_matrix;
        hierarchy.worldTransformDirty = false;

        // Recursively update children with this entity's world transform
        for (auto child : hierarchy.children) {
            UpdateWorldTransform(child, &transform.transform_matrix);
        }
    }

    /**
     * @brief Updates transforms for entities that have been marked dirty.
     * Call this every frame or when transforms need to be updated.
    */
    void HierarchySystem::UpdateDirtyTransforms()
    {
        // Update all root entities (and their children will update recursively)
        for (auto entity : m_Entities)
        {
            auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);

            // Only process roots and only if dirty
            if (hierarchy.parent == 0 && hierarchy.worldTransformDirty) {
                UpdateWorldTransform(entity, nullptr);
            }
        }
    }

    /**
     * @brief Updates the hierarchy for all root entities in the system.
     * This is the main update function - call this every frame.
    */
    void HierarchySystem::UpdateHierarchy()
    {
        // First pass: update all dirty transforms
        UpdateDirtyTransforms();
    }

    /**
     * @brief Marks a transform as dirty when it's modified.
     * Call this whenever you modify position, rotation, or scale.
     * @param[in] entity The entity whose transform was modified.
    */
    void HierarchySystem::OnTransformChanged(EntityID entity)
    {
        MarkTransformDirty(entity);
    }

    /**
     * @brief Forces an immediate transform update for an entity and its children.
     * Use this when you need transforms updated right away rather than waiting for next frame.
     * @param[in] entity The entity to force update.
    */
    void HierarchySystem::ForceUpdateTransform(EntityID entity)
    {
        auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);

        if (hierarchy.parent == 0) {
            UpdateWorldTransform(entity, nullptr);
        }
        else {
            // Get parent's world transform and update from there
            auto& parentTransform = ECS::GetInstance().GetComponent<Transform>(hierarchy.parent);
            UpdateWorldTransform(entity, &parentTransform.transform_matrix);
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