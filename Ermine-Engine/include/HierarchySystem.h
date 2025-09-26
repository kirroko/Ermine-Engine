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
        void MarkTransformDirty(EntityID entity);
        /**
         * @brief Checks if setting parent would create a cycle in the hierarchy.
         * @param[in] child The entity to be reparented.
         * @param[in] parent The entity to be set as parent.
         * @return True if a cycle would be created, false otherwise.
        */
        bool WouldCreateCycle(EntityID child, EntityID parent) const;

        /**
         * @brief Sets the parent of an entity, updating hierarchy and depth.
         * @param[in] child The entity to set the parent for.
         * @param[in] parent The entity to set as parent.
        */
        void SetParent(EntityID child, EntityID parent);

        /**
         * @brief Removes the parent of an entity, updating hierarchy and depth.
         * @param[in] child The entity to unset the parent for.
        */
        void UnsetParent(EntityID child);

        Matrix4x4 ComputeLocalMatrix(const Transform& transform);

        /**
         * @brief Recursively updates world transforms for an entity and its children.
         * @param[in] entity The root entity to start updating from.
        */
        void UpdateWorldTransform(EntityID entity, const Matrix4x4* parentWorld);

        void UpdateWorldTransform(EntityID entity, const Matrix4x4* parentWorld);

        void UpdateDirtyTransforms();

        /**
         * @brief Updates the hierarchy for all root entities in the system.
        */
        void UpdateHierarchy();

        void OnTransformChanged(EntityID entity);

        void ForceUpdateTransform(EntityID entity);

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
    };
}