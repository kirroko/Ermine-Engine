/* Start Header ************************************************************************/
/*!
\file       HierarchyPanel.h
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu
\date       27/03/2025
\brief      Hierarchy panel for viewing and editing entity

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "Scene.h"
#include "imgui.h"

namespace Ermine {
    class HierarchyPanel {
    private:
        Scene* m_ActiveScene = nullptr;
        bool m_IsVisible = true;
        EntityID m_PendingFocusEntity = 0;
        //float m_FocusTimer = 0.0f;

        // UI helper functions
        void DuplicateEntity(EntityID sourceEntity);
        void DrawEntityNode(EntityID entity, int depth);
        void DrawContextMenu();
        void HandleDragDrop(EntityID entity);
        void HandleUnparentDrop();
        const char* GetEntityIcon(EntityID entity) const;

    public:
        HierarchyPanel() = default;
        ~HierarchyPanel() = default;

        void SetScene(Scene* scene);
        Scene* GetScene() const;

        void OnImGuiRender();
        void SetVisible(bool visible) { m_IsVisible = visible; }
        bool IsVisible() const { return m_IsVisible; }
    };
}