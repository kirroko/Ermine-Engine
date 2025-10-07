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
#include "Matrix4x4.h"

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
        EE_CORE_INFO("UpdateWorldTransform called for entity {}", entity);

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
            // Get parent's world transform from hierarchy component (NOT Transform component!)
            auto& parentHierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(hierarchy.parent);
            
            // Use parentHierarchy.worldTransform instead of parentTransform.transform_matrix
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
        transform.isDirty = false;

        // Recursively update children
        for (auto child : hierarchy.children) {
            UpdateWorldTransform(child);
        }

        Vec3 worldPos;
        worldPos.x = transform.transform_matrix.m03;
        worldPos.y = transform.transform_matrix.m13;
        worldPos.z = transform.transform_matrix.m23;
        EE_CORE_INFO("Entity {} world position after update: {}, {}, {}", entity, worldPos.x, worldPos.y, worldPos.z);
    }

    /**
     * @brief Updates the hierarchy for all root entities in the system.
    */
    void HierarchySystem::UpdateHierarchy()
    {
        // Only update root entities that are dirty or have dirty children
        for (auto entity : m_Entities)
        {
            auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
            auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);

            // Only update if this entity is dirty AND it's a root entity
            if (hierarchy.parent == 0 && (hierarchy.isDirty || hierarchy.worldTransformDirty || transform.isDirty)) {
                UpdateWorldTransform(entity);
            }
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
        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
        
        // Mark both local transform and world transform as needing update
        hierarchy.isDirty = true;
        hierarchy.worldTransformDirty = true;
        transform.isDirty = true;  // Make sure Transform component is also marked dirty

        // Mark all children's world transforms as needing update (but not their local transforms)
        for (auto child : hierarchy.children) {
            auto& childHierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(child);
            auto& childTransform = ECS::GetInstance().GetComponent<Transform>(child);
            
            // Mark world transform as dirty for children - their local transforms haven't changed
            childHierarchy.worldTransformDirty = true;
            childTransform.isDirty = true; // Also mark Transform component dirty for proper rendering
            
            // Recursively mark children's world transforms as dirty
            MarkChildrenWorldTransformDirty(child);
        }
    }

    /**
     * @brief Helper method to recursively mark only world transforms as dirty for children
     * @param[in] entity The entity whose children need world transform updates
     */
    void HierarchySystem::MarkChildrenWorldTransformDirty(EntityID entity)
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return;

        const auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
        
        for (auto child : hierarchy.children) {
            auto& childHierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(child);
            auto& childTransform = ECS::GetInstance().GetComponent<Transform>(child);
            childHierarchy.worldTransformDirty = true;
            childTransform.isDirty = true; // Also mark Transform component dirty
            
            // Recursively mark grandchildren
            MarkChildrenWorldTransformDirty(child);
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
            Mtx44Inverse(invParentWorld, parentHierarchy.worldTransform);

            // Convert world position to parent-local space using Vector3D
            Vector3D worldPosVec(worldPos.x, worldPos.y, worldPos.z);
            Vector3D localPos = invParentWorld * worldPosVec;

            transform.position = Vec3(localPos.x, localPos.y, localPos.z);
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

    /**
     * @brief Adds a child entity to a parent, creating the hierarchy relationship.
     * @param[in] parent The parent entity ID.
     * @param[in] child The child entity ID.
     * @return True if successful, false if it would create a cycle or entities are invalid.
     */
    bool HierarchySystem::AddChild(EntityID parent, EntityID child)
    {
        if (!ECS::GetInstance().IsEntityValid(parent) || !ECS::GetInstance().IsEntityValid(child))
            return false;
            
        if (WouldCreateCycle(child, parent))
            return false;
            
        SetParent(child, parent);
        return true;
    }

    /**
     * @brief Removes a child from its parent.
     * @param[in] child The child entity ID.
     * @return True if successful, false if child has no parent.
     */
    bool HierarchySystem::RemoveChild(EntityID child)
    {
        if (!ECS::GetInstance().IsEntityValid(child))
            return false;
            
        auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(child);
        if (hierarchy.parent == 0)
            return false;
            
        UnsetParent(child);
        return true;
    }

    /**
     * @brief Call this when an entity's local transform has been modified to ensure proper propagation.
     * @param[in] entity The entity whose transform was modified.
     */
    void HierarchySystem::OnTransformChanged(EntityID entity)
    {
        MarkDirty(entity);
    }

    /**
     * @brief Initialize hierarchy component for a newly created entity
     * @param[in] entity The entity to initialize
     */
    void HierarchySystem::InitializeEntity(EntityID entity)
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return;
            
        if (!ECS::GetInstance().HasComponent<HierarchyComponent>(entity))
            return;
            
        auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
        
        // Mark as needing initial update
        hierarchy.isDirty = true;
        hierarchy.worldTransformDirty = true;
        transform.isDirty = true;
        
        // Initialize world transform for root entities
        if (hierarchy.parent == 0)
        {
            // Build initial local transform matrix
            Matrix4x4 translation, rotation, scale;
            Mtx44Identity(translation);
            Mtx44Identity(rotation);
            Mtx44Identity(scale);

            Mtx44Translate(translation, transform.position.x, transform.position.y, transform.position.z);
            Mtx44SetFromQuaternion(rotation, transform.rotation);
            Mtx44Scale(scale, transform.scale.x, transform.scale.y, transform.scale.z);

            Matrix4x4 localMatrix = translation * rotation * scale;
            
            // Set both hierarchy and transform matrices
            hierarchy.worldTransform = localMatrix;
            transform.transform_matrix = localMatrix;
            
            // Clear dirty flags since we just set everything up
            hierarchy.isDirty = false;
            hierarchy.worldTransformDirty = false;
            transform.isDirty = false;
        }
    }

    /**
     * @brief Force update transforms for all entities (useful after loading/creating scenes)
     */
    void HierarchySystem::ForceUpdateAllTransforms()
    {
        // Mark all entities as dirty
        for (auto entity : m_Entities)
        {
            auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
            auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
            
            hierarchy.isDirty = true;
            hierarchy.worldTransformDirty = true;
            transform.isDirty = true;
        }
        
        // Force update hierarchy
        UpdateHierarchy();
    }

    /**
     * @brief Ensures transform matrix is synchronized with world transform
     * @param[in] entity The entity to sync
     */
    void HierarchySystem::SyncTransformMatrix(EntityID entity)
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return;
            
        if (!ECS::GetInstance().HasComponent<HierarchyComponent>(entity))
            return;
            
        auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
        
        // Sync the transform matrix with the world transform
        transform.transform_matrix = hierarchy.worldTransform;
    }

    /**
     * @brief Recursively updates world transforms and tracks updated entities to avoid double-updates
     * @param[in] entity The entity to update
     * @param[in/out] updatedEntities Set of entities that have already been updated
     */
    void HierarchySystem::UpdateWorldTransformRecursive(EntityID entity, std::set<EntityID>& updatedEntities)
    {
        // Skip if already updated in this frame
        if (updatedEntities.find(entity) != updatedEntities.end()) {
            return;
        }

        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
        auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);

        // **FIX: Ensure parent is updated first**
        if (hierarchy.parent != 0) {
            UpdateWorldTransformRecursive(hierarchy.parent, updatedEntities);
        }

        // Build local transform matrix (order matters: T * R * S)
        Matrix4x4 localMatrix;
        {
            Matrix4x4 translation, rotation, scale;
            Mtx44Identity(translation);
            Mtx44Identity(rotation);
            Mtx44Identity(scale);

            // Build matrices
            Mtx44Translate(translation, transform.position.x, transform.position.y, transform.position.z);
            Mtx44SetFromQuaternion(rotation, transform.rotation);
            Mtx44Scale(scale, transform.scale.x, transform.scale.y, transform.scale.z);

            // Combine: Translation * Rotation * Scale
            localMatrix = translation * rotation * scale;
        }

        // Calculate world transform
        if (hierarchy.parent != 0) {
            auto& parentHierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(hierarchy.parent);
            
            // Combine with parent's world transform: ParentWorld * LocalTransform
            hierarchy.worldTransform = parentHierarchy.worldTransform * localMatrix;
        }
        else {
            // Root entity: world transform = local transform
            hierarchy.worldTransform = localMatrix;
        }

        // CRITICAL FIX: Update the Transform component's transform_matrix
        // The renderer uses transform.transform_matrix, so we must update it!
        transform.transform_matrix = hierarchy.worldTransform;

        // Mark as clean
        hierarchy.isDirty = false;
        hierarchy.worldTransformDirty = false;
        transform.isDirty = false;  // Also clear the Transform dirty flag

        // Mark this entity as updated
        updatedEntities.insert(entity);

        // Update children recursively
        for (auto child : hierarchy.children) {
            // Recursively update child
            UpdateWorldTransformRecursive(child, updatedEntities);
        }
    }
}