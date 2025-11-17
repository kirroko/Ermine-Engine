/* Start Header ************************************************************************/
/*!
\file       CutsceneGUI.h
\author     Claude Code
\date       11/2025
\brief      This file contains the declaration of the CutsceneGUI class for
            rendering cutscene slideshow images with auto-advance functionality.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"
#include "ImGuiUIWindow.h"
#include "Texture.h"
#include <imgui.h>
#include <memory>
#include <vector>
#include <string>

namespace Ermine::editor
{
    /*!
    \class CutsceneGUI
    \brief ImGui window for rendering cutscene slideshow with auto-advance
    */
    class CutsceneGUI : public ImGUIWindow
    {
    public:
        /*!
        \brief Default constructor
        */
        CutsceneGUI();

        /*!
        \brief Renders the cutscene slideshow
        */
        void Render() override;

        /*!
        \brief Sets whether the cutscene is active
        \param active True to show the cutscene, false to hide it
        */
        void SetActive(bool active) { m_IsActive = active; }

        /*!
        \brief Gets whether the cutscene is active
        \return True if the cutscene is active, false otherwise
        */
        bool IsActive() const { return m_IsActive; }

        /*!
        \brief Load a sequence of images for the slideshow
        \param imagePaths Vector of file paths to the cutscene images
        \param captions Vector of caption/subtitle text for each slide
        \param duration Duration in seconds for each slide
        */
        void LoadSlideshow(const std::vector<std::string>& imagePaths, const std::vector<std::string>& captions, float duration = 3.0f);

        /*!
        \brief Start playing the slideshow
        */
        void StartSlideshow();

        /*!
        \brief Stop the slideshow and hide the cutscene
        */
        void StopSlideshow();

        /*!
        \brief Check if the slideshow has finished
        \return True if all slides have been shown
        */
        bool IsFinished() const { return m_IsFinished; }

        /*!
        \brief Set the scene to load after cutscene finishes
        \param scenePath Path to the scene file
        */
        void SetNextScene(const std::string& scenePath) { m_NextScenePath = scenePath; }

    private:
        /*!
        \brief Renders the subtitle/caption at the bottom of the screen
        \param caption The caption text to display
        \param viewport The viewport size
        */
        void RenderCaption(const std::string& caption, const ImVec2& viewport);

        bool m_IsActive = false;                                        ///< Cutscene visibility state
        bool m_IsFinished = false;                                      ///< Whether slideshow has completed
        int m_CurrentSlideIndex = 0;                                    ///< Current slide being displayed
        float m_SlideTimer = 0.0f;                                      ///< Timer for current slide
        float m_SlideDuration = 3.0f;                                   ///< Duration per slide in seconds
        std::vector<std::shared_ptr<graphics::Texture>> m_SlideTextures; ///< Loaded slide textures
        std::vector<std::string> m_Captions;                            ///< Caption text for each slide
        std::string m_NextScenePath;                                    ///< Scene to load after cutscene
    };
} // namespace Ermine::editor
