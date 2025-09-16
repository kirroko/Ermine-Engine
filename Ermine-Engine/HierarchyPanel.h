/* Start Header ************************************************************************/
/*!
\file       HierarchyPanel.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       27/03/2025
\brief      Inspector panel for viewing and editing entity properties

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
/* End Header **************************************************************************/

#pragma once
#include "Scene.h"
#include "imgui.h"

namespace Ermine {
    class HierarchyPanel {
    private:
        Scene* m_ActiveScene = nullptr;
        bool m_IsVisible = true;

        // UI helper functions
        void DrawEntityNode(EntityID entity);
        void DrawContextMenu();
        void HandleDragDrop(EntityID entity);
        const char* GetEntityIcon(EntityID entity) const;

    public:
        HierarchyPanel();
        ~HierarchyPanel();

        void SetScene(Scene* scene) { m_ActiveScene = scene; }
        Scene* GetScene() const { return m_ActiveScene; }

        void OnImGuiRender();
        void SetVisible(bool visible) { m_IsVisible = visible; }
        bool IsVisible() const { return m_IsVisible; }
    };
}