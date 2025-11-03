/* Start Header ************************************************************************/
/*!
\file       HierarchyInspector.h
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu (30%)
\co-author  WEE HONG RU, Curtis, h.wee, 2301266, h.wee\@digipen.edu (70%)
\date       10/09/2025
\brief      Inspector panel for viewing and editing entity properties

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "PreCompile.h"
#include "Scene.h"
#include "HierarchyPanel.h"

namespace Ermine::editor {
    /*!
    \class HierarchyInspector
    \brief Provides an inspector panel UI for viewing and editing components of selected entities
    */
    class HierarchyInspector {
    public:
        /*!
        \brief Default constructor
        */
        HierarchyInspector() = default;

        /*!
        \brief Renders the inspector panel UI using ImGui
        \details Displays all components of the currently selected entity with editing controls
        */
        void OnImGuiRender();

        /*!
        \brief Sets the active scene for the inspector to work with
        \param scene Pointer to the scene to inspect (can be nullptr)
        */
        void SetScene(Scene* scene) { m_ActiveScene = scene; }

        /*!
        \brief Gets the currently active scene
        \return Pointer to the active scene, or nullptr if none is set
        */
        Scene* GetScene() const { return m_ActiveScene; }

        /*!
        \brief Sets the visibility state of the inspector panel
        \param visible True to show the panel, false to hide it
        */
        void SetVisible(bool visible) { m_IsVisible = visible; }

        /*!
        \brief Gets the current visibility state of the inspector panel
        \return True if the panel is visible, false otherwise
        */
        bool IsVisible() const { return m_IsVisible; }

    private:
        /*!
        \brief Draws the entity name header and metadata section
        \param entity The entity whose header to draw
        */
        void DrawEntityHeader(EntityID entity);

        /*!
        \brief Draws UI controls for the Transform component
        \param entity The entity whose Transform component to display
        */
        void DrawTransformComponent(EntityID entity);

        /*!
        \brief Draws UI controls for the Mesh component
        \param entity The entity whose Mesh component to display
        */
        void DrawMeshComponent(EntityID entity);

        /*!
        \brief Draws UI controls for the Material component
        \param entity The entity whose Material component to display
        */
        void DrawMaterialComponent(EntityID entity);

        /*!
        \brief Draws UI controls for the Light component
        \param entity The entity whose Light component to display
        */
        void DrawLightComponent(EntityID entity);

        /*!
        \brief Draws UI controls for the Hierarchy component
        \param entity The entity whose Hierarchy component to display
        */
        void DrawHierarchyComponent(EntityID entity);

        /*!
        \brief Draws UI controls for the Physics component
        \param entity The entity whose Physics component to display
        */
        void DrawPhysicsComponent(EntityID entity);

        /*!
        \brief Draws UI controls for the Audio component
        \param entity The entity whose Audio component to display
        */
        void DrawAudioComponent(EntityID entity);

        /*!
        \brief Draws UI controls for the Script component
        \param entity The entity whose Script component to display
        */
        void DrawScriptComponent(EntityID entity);

        /*!
        \brief Draws UI controls for the Model component
        \param entity The entity whose Model component to display
        */
        void DrawModelComponent(EntityID entity);

        /*!
        \brief Draws UI controls for the Animation component
        \param entity The entity whose Animation component to display
        */
        void DrawAnimationComponent(EntityID entity);

        /*!
        \brief Displays a menu for adding new components to an entity
        \param entity The entity to add components to
        */
        void DrawAddComponentMenu(EntityID entity);

        void DrawStateMachineComponent(EntityID entity);

        void DrawNavMeshComponent(EntityID entity);

        void DrawNavMeshAgentComponent(EntityID entity);

        void DrawParticleEmitterComponent(EntityID entity);

        Scene* m_ActiveScene = nullptr;  ///< Pointer to the currently active scene
        bool m_IsVisible = true;         ///< Inspector panel visibility state
    };
} // namespace Ermine::editor