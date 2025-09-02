/* Start Header ************************************************************************/
/*!
\file       HierarchySystem.h
\author     Edwin Lee Zirui, edwinzirui.lee 2301299
\date       Sep 02, 2025
\brief      System for managing entity hierarchy and scene graph.

Copyright (C) 2025 DigiPen Institute of Technology.
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
        void SetParent(EntityID child, EntityID parent);
        void UnsetParent(EntityID child);
        void UpdateHierarchy();
        EntityID GetParent(EntityID entity) const;
        const std::vector<EntityID>& GetChildren(EntityID entity) const;
    };
}