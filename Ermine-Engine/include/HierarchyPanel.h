/* Start Header ************************************************************************/
/*!
\file       HierarchyPanel.h
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu
\date       Sep 05, 2025
\brief      Declares the HierarchyPanel class which provides an ImGui-based interface
            for visualizing and manipulating the scene's entity hierarchy tree.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "Scene.h"
#include "imgui.h"

namespace Ermine {
    /*!
    \class HierarchyPanel
    \brief Manages the scene hierarchy UI panel with entity tree visualization and manipulation
    */
    class HierarchyPanel {
    private:
        Scene* m_ActiveScene = nullptr;      ///< Pointer to the currently active scene
        bool m_IsVisible = true;              ///< Panel visibility state
        bool m_ShowInactive = false;          ///< Show inactive entities in hierarchy (grayed out)
        EntityID m_PendingFocusEntity = 0;    ///< Entity waiting for inspector focus after interaction

        char m_SearchBuffer[128] = {};  ///< Buffer for entity search input
        bool m_IsSearching = false;		///< Flag indicating if search mode is active

        // UI helper functions
        void DuplicateEntity(EntityID sourceEntity);
        /*!
        \brief Recursively draws an entity node and its children in the hierarchy tree
        \param entity The entity ID to draw
        \param depth Current depth level for indentation (0 = root level)
        */
        void DrawEntityNode(EntityID entity, int depth);

        /*!
        \brief Displays right-click context menu with entity creation and manipulation options
        */
        void DrawContextMenu();

        /*!
        \brief Handles drag-and-drop operations for entity parenting
        \param entity The entity that can receive dropped entities as children
        */
        void HandleDragDrop(EntityID entity);

        /*!
        \brief Handles dropping entities onto empty space to unparent them
        */
        void HandleUnparentDrop();

        /*!
        \brief Returns a text icon/prefix for the entity based on its components
        \param entity The entity to get an icon for
        \return Icon string (e.g., "[Light] ", "[Audio] ") or empty string
        */
        const char* GetEntityIcon(EntityID entity) const;

        /*!
        \brief Checks if an entity name matches the current search query
        \param name The entity name to check
        \param search The search query string
        \return True if the name matches the search query, false otherwise
        */
        bool NameMatchesSearch(const std::string& name, const char* search);

    public:
        /*!
        \brief Default constructor
        */
        HierarchyPanel() = default;

        /*!
        \brief Default destructor
        */
        ~HierarchyPanel() = default;

        /*!
        \brief Sets the active scene for the hierarchy panel to display
        \param scene Pointer to the scene to display (can be nullptr)
        */
        void SetScene(Scene* scene);

        /*!
        \brief Gets the currently active scene
        \return Pointer to the active scene, or nullptr if none is set
        */
        Scene* GetScene() const;

        /*!
        \brief Renders the hierarchy panel UI using ImGui
        \details Displays the scene entity tree with drag-and-drop support,
                 context menus, and entity selection
        */
        void OnImGuiRender();

        /*!
        \brief Sets the visibility state of the panel
        \param visible True to show the panel, false to hide it
        */
        void SetVisible(bool visible) { m_IsVisible = visible; }

        /*!
        \brief Gets the current visibility state of the panel
        \return True if the panel is visible, false otherwise
        */
        bool IsVisible() const { return m_IsVisible; }
    };
}