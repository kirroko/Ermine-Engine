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
        
        // IMPORTANT: Ensure child transforms are updated when parenting changes
        MarkDirty(child);
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
            
            // IMPORTANT: Update transforms when unparenting
            MarkDirty(child);
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
		
		// NEW: Get or create GlobalTransform component
		GlobalTransform* globalTransform = nullptr;
		if (ECS::GetInstance().HasComponent<GlobalTransform>(entity)) {
			globalTransform = &ECS::GetInstance().GetComponent<GlobalTransform>(entity);
		} else {
			// Add GlobalTransform if it doesn't exist
			ECS::GetInstance().AddComponent<GlobalTransform>(entity, GlobalTransform());
			globalTransform = &ECS::GetInstance().GetComponent<GlobalTransform>(entity);
		}

		EE_CORE_INFO("=== Transform Update for Entity {0} ===", entity);
		EE_CORE_INFO("Local Position: ({0:.3f}, {1:.3f}, {2:.3f})", 
					 transform.position.x, transform.position.y, transform.position.z);

		// Build local transform matrix from position, rotation, and scale
		Mtx44 localMatrix = transform.GetLocalMatrix();

		// Calculate world transform based on parent relationship
		if (hierarchy.parent != 0) {
			// Get parent's GlobalTransform component
			if (ECS::GetInstance().HasComponent<GlobalTransform>(hierarchy.parent)) {
				auto& parentGlobalTransform = ECS::GetInstance().GetComponent<GlobalTransform>(hierarchy.parent);
				
				Vec3 parentWorldPos = parentGlobalTransform.GetWorldPosition();
				EE_CORE_INFO("Parent {0} World Position: ({1:.3f}, {2:.3f}, {3:.3f})", 
							 hierarchy.parent, parentWorldPos.x, parentWorldPos.y, parentWorldPos.z);
				
				// FIXED: World transform = Parent's world transform * Local transform
				globalTransform->worldMatrix = parentGlobalTransform.worldMatrix * localMatrix;
			} else {
				// Parent doesn't have GlobalTransform - treat as root
				globalTransform->worldMatrix = localMatrix;
				EE_CORE_WARN("Parent {0} missing GlobalTransform, treating child {1} as root", hierarchy.parent, entity);
			}
		}
		else {
			// Root entity: world transform equals local transform
			globalTransform->worldMatrix = localMatrix;
			EE_CORE_INFO("Entity {0} is ROOT - World = Local transform", entity);
		}

		// REMOVED: Don't update transform.transform_matrix anymore - it was causing confusion
		// The renderer will now use globalTransform->worldMatrix directly

		// Extract and log the calculated world position
		Vec3 worldPos = globalTransform->GetWorldPosition();
		EE_CORE_INFO(">>> Entity {0} World Position: ({1:.3f}, {2:.3f}, {3:.3f})", 
					 entity, worldPos.x, worldPos.y, worldPos.z);

		// Mark as clean
		hierarchy.isDirty = false;
		hierarchy.worldTransformDirty = false;
		transform.isDirty = false;
		globalTransform->isDirty = false;

		// Recursively update all children
		for (auto child : hierarchy.children) {
			EE_CORE_INFO("--- Updating child entity {0} due to parent {1} change ---", child, entity);
			UpdateWorldTransform(child);
		}
		
		EE_CORE_INFO("=== End Transform Update for Entity {0} ===\n", entity);
	}

	// NEW: Helper method to ensure entities have GlobalTransform
	void HierarchySystem::EnsureGlobalTransform(EntityID entity)
	{
		if (!ECS::GetInstance().HasComponent<GlobalTransform>(entity)) {
			ECS::GetInstance().AddComponent<GlobalTransform>(entity, GlobalTransform());
		}
	}

	// NEW: Initialize method to call when adding entities to hierarchy
	void HierarchySystem::InitializeEntity(EntityID entity)
	{
		if (!ECS::GetInstance().IsEntityValid(entity))
			return;
			
		if (!ECS::GetInstance().HasComponent<HierarchyComponent>(entity))
			return;
			
		// Ensure the entity has a GlobalTransform component
		EnsureGlobalTransform(entity);
			
		auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
		auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
		auto& globalTransform = ECS::GetInstance().GetComponent<GlobalTransform>(entity);
		
		// Mark as needing initial update
		hierarchy.isDirty = true;
		hierarchy.worldTransformDirty = true;
		transform.isDirty = true;
		globalTransform.isDirty = true;
		
		// Initialize world transform for root entities
		if (hierarchy.parent == 0)
		{
			globalTransform.worldMatrix = transform.GetLocalMatrix();
			globalTransform.isDirty = false;
		}
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

                // Clear flags immediately to prevent infinite loops
                hierarchy.isDirty = false;
                hierarchy.worldTransformDirty = false;
                transform.isDirty = false;

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

        // ADD DEBUG LOG TO FIND THE CULPRIT
        EE_CORE_WARN("MarkDirty called for entity {} - investigate why!", entity);
        
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

        // Use GlobalTransform
        if (ECS::GetInstance().HasComponent<GlobalTransform>(entity)) {
            const auto& globalTransform = ECS::GetInstance().GetComponent<GlobalTransform>(entity);
            return globalTransform.GetWorldPosition();
        }
        else {
            // Fallback to local position if no GlobalTransform
            const auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
            return transform.position;
        }
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

        // FIXED: Use GlobalTransform instead of transform.transform_matrix
        if (ECS::GetInstance().HasComponent<GlobalTransform>(entity)) {
            const auto& globalTransform = ECS::GetInstance().GetComponent<GlobalTransform>(entity);
            return globalTransform.GetWorldRotation();
        }
        else {
            // Fallback to local rotation if no GlobalTransform
            const auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
            return transform.rotation;
        }
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

        // FIXED: Use GlobalTransform instead of transform.transform_matrix
        if (ECS::GetInstance().HasComponent<GlobalTransform>(entity)) {
            const auto& globalTransform = ECS::GetInstance().GetComponent<GlobalTransform>(entity);
            return globalTransform.GetWorldScale();
        }
        else {
            // Fallback to local scale if no GlobalTransform
            const auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
            return transform.scale;
        }
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
            // FIXED: Use parent's GlobalTransform instead of hierarchy.worldTransform
            if (ECS::GetInstance().HasComponent<GlobalTransform>(hierarchy.parent)) {
                auto& parentGlobalTransform = ECS::GetInstance().GetComponent<GlobalTransform>(hierarchy.parent);

                // Get inverse of parent's world transform
                Matrix4x4 invParentWorld;
                Mtx44Inverse(invParentWorld, parentGlobalTransform.worldMatrix);

                // Convert world position to parent-local space using Vector3D
                Vector3D worldPosVec(worldPos.x, worldPos.y, worldPos.z);
                Vector3D localPos = invParentWorld * worldPosVec;

                transform.position = Vec3(localPos.x, localPos.y, localPos.z);
            }
            else {
                // Parent doesn't have GlobalTransform - treat as root
                transform.position = worldPos;
            }
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
            // FIXED: Use parent's GlobalTransform instead of hierarchy.worldTransform
            if (ECS::GetInstance().HasComponent<GlobalTransform>(hierarchy.parent)) {
                auto& parentGlobalTransform = ECS::GetInstance().GetComponent<GlobalTransform>(hierarchy.parent);

                // Extract parent's world rotation from GlobalTransform
                Quaternion parentWorldRot = parentGlobalTransform.GetWorldRotation();

                // Calculate local rotation: parentWorld^-1 * worldRot 
                Quaternion invParentRot(-parentWorldRot.x, -parentWorldRot.y, -parentWorldRot.z, parentWorldRot.w);
                transform.rotation = invParentRot * worldRot;
            }
            else {
                // Parent doesn't have GlobalTransform - treat as root
                transform.rotation = worldRot;
            }
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
     * @brief Sets the local position of an entity and marks it dirty for transform updates.
     * @param[in] entity The entity to modify.
     * @param[in] localPos The new local position.
     */
    void HierarchySystem::SetLocalPosition(EntityID entity, const Vec3& localPos)
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return;

        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
        transform.position = localPos;
        
        // Mark as dirty to trigger transform update propagation
        MarkDirty(entity);
    }

    /**
     * @brief Sets the local rotation of an entity and marks it dirty for transform updates.
     * @param[in] entity The entity to modify.
     * @param[in] localRot The new local rotation.
     */
    void HierarchySystem::SetLocalRotation(EntityID entity, const Quaternion& localRot)
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return;

        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
        transform.rotation = localRot;
        
        // Mark as dirty to trigger transform update propagation
        MarkDirty(entity);
    }

    /**
     * @brief Sets the local scale of an entity and marks it dirty for transform updates.
     * @param[in] entity The entity to modify.
     * @param[in] localScale The new local scale.
     */
    void HierarchySystem::SetLocalScale(EntityID entity, const Vec3& localScale)
    {
        if (!ECS::GetInstance().IsEntityValid(entity))
            return;

        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
        transform.scale = localScale;
        
        // Mark as dirty to trigger transform update propagation
        MarkDirty(entity);
    }
}