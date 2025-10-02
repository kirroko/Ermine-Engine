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

        // Only update if:
        // 1. This entity's local transform is dirty
        // 2. Entity's world transform is marked dirty
        // 3. Parent's world transform is dirty
        bool needsUpdate = hierarchy.isDirty || hierarchy.worldTransformDirty;

        if (hierarchy.parent != 0) {
            auto& parentHierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(hierarchy.parent);
            needsUpdate = needsUpdate || parentHierarchy.worldTransformDirty;
        }

        if (!needsUpdate) {
            // Still need to check children even if this entity doesn't need updating
            // This handles cases where a child was modified but parent wasn't
            for (auto child : hierarchy.children) {
                UpdateWorldTransform(child);
            }
            return;
        }

        // Build local transform matrix in correct order: Scale * Rotation * Translation
        Matrix4x4 localMatrix;
        {
            Matrix4x4 translation, rotation, scale;
            Mtx44Identity(translation);
            Mtx44Identity(rotation);
            Mtx44Identity(scale);

            Mtx44Scale(scale, transform.scale.x, transform.scale.y, transform.scale.z);
            Mtx44SetFromQuaternion(rotation, transform.rotation);
            Mtx44Translate(translation, transform.position.x, transform.position.y, transform.position.z);

            // Final local = Translation * Rotation * Scale
            localMatrix = translation * rotation * scale;
        }

        // Calculate world transform
        if (hierarchy.parent != 0) {
            auto& parentHierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(hierarchy.parent);
            
            // Use parent's cached world transform instead of walking up the hierarchy
            transform.transform_matrix = parentHierarchy.worldTransform * localMatrix;
            hierarchy.worldTransform = transform.transform_matrix;
        }
        else {
            // Root entity - world transform = local transform 
            transform.transform_matrix = localMatrix;
            hierarchy.worldTransform = localMatrix;
        }

        // Mark as clean
        hierarchy.isDirty = false;
        hierarchy.worldTransformDirty = false;

        // Recursively update all children since our world transform changed
        for (auto child : hierarchy.children) {
            // Mark child's world transform as dirty since parent changed
            auto& childHierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(child);
            childHierarchy.worldTransformDirty = true;
            
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
     * @brief Marks an entity and all its children as dirty for transform updates.
     * @param[in] entity The entity to mark as dirty.
    */
    void HierarchySystem::MarkDirty(EntityID entity)
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return;

        auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
        
        // Mark both local transform and world transform as needing update
        hierarchy.isDirty = true;
        hierarchy.worldTransformDirty = true;

        // Mark all children's world transforms as needing update
        for (auto child : hierarchy.children) {
            auto& childHierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(child);
            childHierarchy.worldTransformDirty = true;
            MarkDirty(child); 
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

    /**
     * @brief Gets the world position of an entity.
     * @param[in] entity The entity to query.
     * @return The world position vector.
     */
    Vec3 HierarchySystem::GetWorldPosition(EntityID entity) const
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return Vec3(0.0f, 0.0f, 0.0f);

        const auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
        
        // Extract position from world transform matrix
        Vec3 worldPos;
        worldPos.x = transform.transform_matrix.m03;
        worldPos.y = transform.transform_matrix.m13;
        worldPos.z = transform.transform_matrix.m23;
        
        return worldPos;
    }

    /**
     * @brief Gets the world rotation of an entity.
     * @param[in] entity The entity to query.
     * @return The world rotation quaternion.
     */
    Quaternion HierarchySystem::GetWorldRotation(EntityID entity) const
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);

        const auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
        
        // Extract rotation from world transform matrix
        Matrix4x4 rotationMatrix = transform.transform_matrix;
        
        // Remove translation
        rotationMatrix.m03 = 0.0f;
        rotationMatrix.m13 = 0.0f;
        rotationMatrix.m23 = 0.0f;
        rotationMatrix.m33 = 1.0f;
        
        // Remove scale by normalizing the rotation part
        Vec3 xAxis(rotationMatrix.m00, rotationMatrix.m10, rotationMatrix.m20);
        Vec3 yAxis(rotationMatrix.m01, rotationMatrix.m11, rotationMatrix.m21);
        Vec3 zAxis(rotationMatrix.m02, rotationMatrix.m12, rotationMatrix.m22);
        
        // Normalize axes
        float xLen = sqrtf(xAxis.x * xAxis.x + xAxis.y * xAxis.y + xAxis.z * xAxis.z);
        float yLen = sqrtf(yAxis.x * yAxis.x + yAxis.y * yAxis.y + yAxis.z * yAxis.z);
        float zLen = sqrtf(zAxis.x * zAxis.x + zAxis.y * zAxis.y + zAxis.z * zAxis.z);
        
        if (xLen > 0.0f) { xAxis.x /= xLen; xAxis.y /= xLen; xAxis.z /= xLen; }
        if (yLen > 0.0f) { yAxis.x /= yLen; yAxis.y /= yLen; yAxis.z /= yLen; }
        if (zLen > 0.0f) { zAxis.x /= zLen; zAxis.y /= zLen; zAxis.z /= zLen; }
        
        // Reconstruct rotation matrix
        rotationMatrix.m00 = xAxis.x; rotationMatrix.m10 = xAxis.y; rotationMatrix.m20 = xAxis.z;
        rotationMatrix.m01 = yAxis.x; rotationMatrix.m11 = yAxis.y; rotationMatrix.m21 = yAxis.z;
        rotationMatrix.m02 = zAxis.x; rotationMatrix.m12 = zAxis.y; rotationMatrix.m22 = zAxis.z;
        
        return Mtx44GetQuaternion(rotationMatrix);
    }

    /**
     * @brief Gets the world scale of an entity.
     * @param[in] entity The entity to query.
     * @return The world scale vector.
     */
    Vec3 HierarchySystem::GetWorldScale(EntityID entity) const
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return Vec3(1.0f, 1.0f, 1.0f);

        const auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
        
        // Extract scale from world transform matrix
        Vec3 xAxis(transform.transform_matrix.m00, transform.transform_matrix.m10, transform.transform_matrix.m20);
        Vec3 yAxis(transform.transform_matrix.m01, transform.transform_matrix.m11, transform.transform_matrix.m21);
        Vec3 zAxis(transform.transform_matrix.m02, transform.transform_matrix.m12, transform.transform_matrix.m22);
        
        Vec3 worldScale;
        worldScale.x = sqrtf(xAxis.x * xAxis.x + xAxis.y * xAxis.y + xAxis.z * xAxis.z);
        worldScale.y = sqrtf(yAxis.x * yAxis.x + yAxis.y * yAxis.y + yAxis.z * yAxis.z);
        worldScale.z = sqrtf(zAxis.x * zAxis.x + zAxis.y * zAxis.y + zAxis.z * zAxis.z);
        
        return worldScale;
    }

    /**
     * @brief Sets the world position of an entity, updating local transform accordingly.
     * @param[in] entity The entity to modify.
     * @param[in] worldPos The new world position.
     */
    void HierarchySystem::SetWorldPosition(EntityID entity, const Vec3& worldPos)
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return;

        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
        auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);

        if (hierarchy.parent != 0) {
            // Convert world position to local space using inverse parent transform
            auto& parentHierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(hierarchy.parent);
            
            // Get inverse of parent's world transform
            Matrix4x4 invParentWorld;
            Mtx44Inverse(invParentWorld, parentHierarchy.worldTransform); // Fixed function name

            // Convert world position to parent-local space
            Vec4 worldPos4(worldPos.x, worldPos.y, worldPos.z, 1.0f);
            Vec4 localPos4 = invParentWorld * worldPos4;

            transform.position = Vec3(localPos4.x, localPos4.y, localPos4.z);
        }
        else {
            // Root entity - world position = local position
            transform.position = worldPos;
        }

        MarkDirty(entity);
    }

    /**
     * @brief Sets the world rotation of an entity, updating local transform accordingly.
     * @param[in] entity The entity to modify.
     * @param[in] worldRot The new world rotation.
     */
    void HierarchySystem::SetWorldRotation(EntityID entity, const Quaternion& worldRot) 
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return;

        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
        auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);

        if (hierarchy.parent != 0) {
            // Get parent's world rotation
            auto& parentHierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(hierarchy.parent);

            // Extract parent's world rotation
            Matrix4x4 parentWorld = parentHierarchy.worldTransform;
            
            // Remove translation
            parentWorld.m03 = 0.0f;
            parentWorld.m13 = 0.0f;
            parentWorld.m23 = 0.0f;
            parentWorld.m33 = 1.0f;

            // Remove scale
            Vec3 xAxis(parentWorld.m00, parentWorld.m10, parentWorld.m20);
            Vec3 yAxis(parentWorld.m01, parentWorld.m11, parentWorld.m21);
            Vec3 zAxis(parentWorld.m02, parentWorld.m12, parentWorld.m22);

            float xLen = sqrtf(xAxis.x * xAxis.x + xAxis.y * xAxis.y + xAxis.z * xAxis.z);
            float yLen = sqrtf(yAxis.x * yAxis.x + yAxis.y * yAxis.y + yAxis.z * yAxis.z);
            float zLen = sqrtf(zAxis.x * zAxis.x + zAxis.y * zAxis.y + zAxis.z * zAxis.z);

            if (xLen > 0.0f) { xAxis.x /= xLen; xAxis.y /= xLen; xAxis.z /= xLen; }
            if (yLen > 0.0f) { yAxis.x /= yLen; yAxis.y /= yLen; yAxis.z /= yLen; }
            if (zLen > 0.0f) { zAxis.x /= zLen; zAxis.y /= zLen; zAxis.z /= zLen; }

            parentWorld.m00 = xAxis.x; parentWorld.m10 = xAxis.y; parentWorld.m20 = xAxis.z;
            parentWorld.m01 = yAxis.x; parentWorld.m11 = yAxis.y; parentWorld.m21 = yAxis.z;
            parentWorld.m02 = zAxis.x; parentWorld.m12 = zAxis.y; parentWorld.m22 = zAxis.z;

            Quaternion parentWorldRot = Mtx44GetQuaternion(parentWorld);
            
            // Calculate local rotation: parentWorld^-1 * worldRot 
            Quaternion invParentRot(-parentWorldRot.x, -parentWorldRot.y, -parentWorldRot.z, parentWorldRot.w);
            transform.rotation = invParentRot * worldRot;
        }
        else {
            // Root entity - world rotation = local rotation
            transform.rotation = worldRot;
        }

        MarkDirty(entity);
    }

    /**
     * @brief Sets the world scale of an entity, updating local transform accordingly.
     * @param[in] entity The entity to modify.
     * @param[in] worldScale The new world scale.
     */
    void HierarchySystem::SetWorldScale(EntityID entity, const Vec3& worldScale)
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return;

        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
        auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);

        if (hierarchy.parent != 0) {
            // Get parent's world scale
            Vec3 parentWorldScale = GetWorldScale(hierarchy.parent);

            // Calculate local scale needed to achieve desired world scale
            transform.scale.x = (parentWorldScale.x != 0.0f) ? worldScale.x / parentWorldScale.x : worldScale.x;
            transform.scale.y = (parentWorldScale.y != 0.0f) ? worldScale.y / parentWorldScale.y : worldScale.y;
            transform.scale.z = (parentWorldScale.z != 0.0f) ? worldScale.z / parentWorldScale.z : worldScale.z;
        }
        else {
            // Root entity - world scale = local scale
            transform.scale = worldScale;
        }

        MarkDirty(entity);
    }
}