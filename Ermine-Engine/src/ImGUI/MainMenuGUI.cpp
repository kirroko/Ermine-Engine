/* Start Header ************************************************************************/
/*!
\file       MainMenuGUI.cpp
\author     GitHub Copilot
\date       11/2025
\brief      This file contains the implementation of the MainMenuGUI class for
            rendering the main menu scene with play and quit buttons.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "MainMenuGUI.h"
#include "AssetManager.h"
#include "SceneManager.h"
#include "EditorGUI.h"
#include "Input.h"
#include <imgui.h>
#include <GLFW/glfw3.h>

namespace Ermine::editor
{
    MainMenuGUI::MainMenuGUI() : ImGUIWindow("Main Menu")
    {
        // Load the background texture
        m_BackgroundTexture = AssetManager::GetInstance().LoadTexture("../Resources/Textures/UI/image.png");
    }

    void MainMenuGUI::Render()
    {
        if (!m_IsActive)
            return;

        // Set cursor visible when menu is active
        glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 viewport = io.DisplaySize;

        // Create fullscreen menu window
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(viewport, ImGuiCond_Always);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Main Menu", nullptr, flags);

        // Draw background image if texture is loaded
        if (m_BackgroundTexture && m_BackgroundTexture->IsValid())
        {
            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::Image(
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W) || defined(IMGUI_IMPL_OPENGL_ES2) || defined(IMGUI_IMPL_OPENGL_ES3) || defined(IMGUI_IMPL_OPENGL_LOADER_GLEW) || defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
                (ImTextureID)(intptr_t)m_BackgroundTexture->GetRendererID(),
#else
                m_BackgroundTexture->GetRendererID(),
#endif
                viewport,
                ImVec2(0, 1), ImVec2(1, 0)  // UV coordinates (flipped vertically)
            );
        }
        else
        {
            // Fallback: Draw semi-transparent background if texture fails to load
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(ImVec2(0, 0), viewport, IM_COL32(0, 0, 0, 180));
        }

        // Center content
        float centerX = viewport.x / 2.0f;
        float centerY = viewport.y / 2.0f;

        // Calculate button dimensions based on image size
        float buttonWidth = 150.0f;
        float buttonHeight = 50.0f;

        // If texture is loaded, scale buttons relative to image dimensions
        if (m_BackgroundTexture && m_BackgroundTexture->IsValid())
        {
            // Scale buttons to be proportional to the viewport/image
            // Buttons will be 15% of viewport width and 5% of viewport height
            buttonWidth = viewport.x * 0.20f;
            buttonHeight = viewport.y * 0.07f;
        }

        // Play Button
        {
            ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2.0f, centerY - buttonHeight + 375));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0.7f, 1.0f));

            if (ImGui::Button("PLAY", ImVec2(buttonWidth, buttonHeight)))
            {
                m_IsActive = false;
                EditorGUI::isPlaying = true;
                EditorGUI::s_state = EditorGUI::SimState::playing;
                SceneManager::GetInstance().SaveTemp();
                glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                if (glfwRawMouseMotionSupported())
                    glfwSetInputMode(glfwGetCurrentContext(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            }

            ImGui::PopStyleColor(3);
        }

        // Quit Button
        {
            ImGui::SetCursorPos(ImVec2(centerX - buttonWidth / 2.0f, centerY + 525));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

            if (ImGui::Button("QUIT", ImVec2(buttonWidth, buttonHeight)))
            {
                glfwSetWindowShouldClose(glfwGetCurrentContext(), GLFW_TRUE);
            }

            ImGui::PopStyleColor(3);
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    bool MainMenuGUI::RenderPlayButton()
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0.7f, 1.0f));

        bool clicked = ImGui::Button("PLAY", ImVec2(150, 50));

        ImGui::PopStyleColor(3);

        return clicked;
    }

    bool MainMenuGUI::RenderQuitButton()
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

        bool clicked = ImGui::Button("QUIT", ImVec2(150, 50));

        ImGui::PopStyleColor(3);

        return clicked;
    }

    void MainMenuGUI::RenderMenuBackground()
    {
        // Background rendering logic
    }
}
