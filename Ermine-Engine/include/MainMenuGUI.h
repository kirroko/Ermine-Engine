/* Start Header ************************************************************************/
/*!
\file       MainMenuGUI.h
\author     GitHub Copilot
\date       11/2025
\brief      This file contains the declaration of the MainMenuGUI class for
            rendering the main menu scene with play and quit buttons.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"
#include "ImGuiUIWindow.h"
#include "Texture.h"
#include "Shader.h"
#include <memory>

namespace Ermine::editor
{
    /*!
    \class MainMenuGUI
    \brief ImGui window for rendering the main menu scene with play and quit buttons
    */
    class MainMenuGUI : public ImGUIWindow
    {
    public:
        /*!
        \brief Default constructor
        */
        MainMenuGUI();

        /*!
        \brief Renders the main menu UI
        */
        void Render() override;

        /*!
        \brief Sets whether the main menu is active
        \param active True to show the menu, false to hide it
        */
        void SetActive(bool active) { m_IsActive = active; }

        /*!
        \brief Gets whether the main menu is active
        \return True if the menu is active, false otherwise
        */
        bool IsActive() const { return m_IsActive; }

    private:
        /*!
        \brief Renders the play button
        \return True if the button was clicked
        */
        bool RenderPlayButton();

        /*!
        \brief Renders the quit button
        \return True if the button was clicked
        */
        bool RenderQuitButton();

        /*!
        \brief Renders the menu title and background
        */
        void RenderMenuBackground();

        bool m_IsActive = false;                                    ///< Menu visibility state
        std::shared_ptr<graphics::Texture> m_BackgroundTexture;    ///< Background texture for the menu
        std::shared_ptr<graphics::Shader> m_MenuShader;            ///< Shader for menu rendering
    };
} // namespace Ermine::editor
