/* Start Header ************************************************************************/
/*!
\file       HierarchySystem.h
\author     Edwin Lee Zirui, edwinzirui.lee 2301299, edwinzirui.lee\@digipen.edu
\date       Sep 05, 2025
\brief      System for managing entity hierarchy and scene graph.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "Systems.h"
#include "Components.h"

namespace Ermine
{
    class HierarchySystem : public System
    {
    public:
        /**
         * @brief Checks if setting parent would create a cycle in the hierarchy.
         * @param[in] child The entity to be reparented.
         * @param[in] parent The entity to be set as parent.
         * @return True if a cycle would be created, false otherwise.
        */
        bool WouldCreateCycle(EntityID child, EntityID parent) const;

        /**
         * @brief Decomposes a 4x4 matrix into position, rotation, and scale components.
         * @param[in] matrix The matrix to decompose.
         * @param[out] position The extracted position.
         * @param[out] rotation The extracted rotation as quaternion.
         * @param[out] scale The extracted scale.
        */
        void DecomposeMatrix(const Mtx44& matrix, Vec3& position, Quaternion& rotation, Vec3& scale);


        /**
         * @brief Sets the parent of an entity, updating hierarchy and depth.
         * @param[in] child The entity to set the parent for.
         * @param[in] parent The entity to set as parent.
        */
        void SetParent(EntityID child, EntityID parent, bool preserveWorldTransform);

        /**
         * @brief Removes the parent of an entity, updating hierarchy and depth.
         * @param[in] child The entity to unset the parent for.
        */
        void UnsetParent(EntityID child);

        /**
         * @brief Recursively updates world transforms for an entity and its children.
         * @param[in] entity The root entity to start updating from.
        */
        void UpdateWorldTransform(EntityID entity);

        /**
         * @brief Updates the hierarchy for all root entities in the system.
        */
        void UpdateHierarchy();

        /**
         * @brief Marks an entity and all its children as dirty for transform updates.
         * @param[in] entity The entity to mark as dirty.
        */
        void MarkDirty(EntityID entity);

        /**
         * @brief Gets the parent of an entity.
         * @param[in] entity The entity to query.
         * @return The parent entity ID, or 0 if none.
        */
        EntityID GetParent(EntityID entity) const;

        /**
          * @brief Gets the children of an entity.
          * @param[in] entity The entity to query.
          * @return Reference to a vector of child entity IDs.
         */
        const std::vector<EntityID>& GetChildren(EntityID entity) const;

        /**
         * @brief Gets the world position of an entity.
         * @param[in] entity The entity to query.
         * @return The world position vector.
         */
        Vec3 GetWorldPosition(EntityID entity) const;

        /**
         * @brief Gets the world rotation of an entity.
         * @param[in] entity The entity to query.
         * @return The world rotation quaternion.
         */
        Quaternion GetWorldRotation(EntityID entity) const;

        /**
         * @brief Gets the world scale of an entity.
         * @param[in] entity The entity to query.
         * @return The world scale vector.
         */
        Vec3 GetWorldScale(EntityID entity) const;

        /**
         * @brief Sets the world position of an entity, updating local transform accordingly.
         * @param[in] entity The entity to modify.
         * @param[in] worldPos The new world position.
         */
        void SetWorldPosition(EntityID entity, const Vec3& worldPos);

        /**
         * @brief Sets the world rotation of an entity, updating local transform accordingly.
         * @param[in] entity The entity to modify.
         * @param[in] worldRot The new world rotation.
         */
        void SetWorldRotation(EntityID entity, const Quaternion& worldRot);

        /**
         * @brief Sets the world scale of an entity, updating local transform accordingly.
         * @param[in] entity The entity to modify.
         * @param[in] worldScale The new world scale.
         */
        void SetWorldScale(EntityID entity, const Vec3& worldScale);

        /**
         * @brief Sets the local position of an entity and marks it dirty for transform updates.
         * @param[in] entity The entity to modify.
         * @param[in] localPos The new local position.
         */
        void SetLocalPosition(EntityID entity, const Vec3& localPos);

        /**
         * @brief Sets the local rotation of an entity and marks it dirty for transform updates.
         * @param[in] entity The entity to modify.
         * @param[in] localRot The new local rotation.
         */
        void SetLocalRotation(EntityID entity, const Quaternion& localRot);

        /**
         * @brief Sets the local scale of an entity and marks it dirty for transform updates.
         * @param[in] entity The entity to modify.
         * @param[in] localScale The new local scale.
         */
        void SetLocalScale(EntityID entity, const Vec3& localScale);

        /**
         * @brief Adds a child entity to a parent, creating the hierarchy relationship.
         * @param[in] parent The parent entity ID.
         * @param[in] child The child entity ID.
         * @return True if successful, false if it would create a cycle or entities are invalid.
         */
        bool AddChild(EntityID parent, EntityID child);

        /**
         * @brief Removes a child from its parent.
         * @param[in] child The child entity ID.
         * @return True if successful, false if child has no parent.
         */
        bool RemoveChild(EntityID child);

        /**
         * @brief Call this when an entity's local transform has been modified to ensure proper propagation.
         * @param[in] entity The entity whose transform was modified.
         */
        void OnTransformChanged(EntityID entity);

        /**
		 * @brief Ensures an entity has a GlobalTransform component
		 * @param[in] entity The entity to check
		 */
		void EnsureGlobalTransform(EntityID entity);

		/**
		 * @brief Initialize hierarchy component for a newly created entity
		 * @param[in] entity The entity to initialize
		 */
        void InitializeEntity(EntityID entity);

		/**
		 * @brief Force update transforms for all entities (useful after loading/creating scenes)
		 */
		void ForceUpdateAllTransforms();

        /**
         * @brief Rotate an entity (and its children) around a specific world-space point
         * @param entity Entity to rotate
         * @param point World-space point to rotate around
         * @param axis Rotation axis (world space)
         * @param angleDegrees Rotation angle in degrees
         */
        void RotateAroundPoint(EntityID entity, const Vec3& point,
            const Vec3& axis, float angleDegrees);

        /**
         * @brief Apply a rotation quaternion around a world-space point
         * @param entity Entity to rotate
         * @param point World-space point to rotate around
         * @param rotation Rotation quaternion to apply
         */
        void RotateAroundPointQuat(EntityID entity, const Vec3& point,
            const Quaternion& rotation);

        /**
         * @brief Calculate the geometric center of an entity and all its children
         * @param entity The root entity to calculate center for
         * @return The world-space center position of the hierarchy
         */
        Vec3 CalculateHierarchyCenter(EntityID entity);

    private:
        /**
         * @brief Helper method to recursively mark only world transforms as dirty for children
         * @param[in] entity The entity whose children need world transform updates
         */
        void MarkChildrenWorldTransformDirty(EntityID entity);
    };
}