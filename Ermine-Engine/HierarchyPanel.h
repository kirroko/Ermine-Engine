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