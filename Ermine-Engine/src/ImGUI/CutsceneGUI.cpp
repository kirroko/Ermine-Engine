/* Start Header ************************************************************************/
/*!
\file       CutsceneGUI.cpp
\author     Claude Code
\date       11/2025
\brief      This file contains the implementation of the CutsceneGUI class for
            rendering cutscene slideshow images with auto-advance functionality.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "CutsceneGUI.h"
#include "AssetManager.h"
#include "SceneManager.h"
#include "FrameController.h"
#include <imgui.h>

namespace Ermine::editor
{
    CutsceneGUI::CutsceneGUI() : ImGUIWindow("Cutscene")
    {
    }

    void CutsceneGUI::Render()
    {
        if (!m_IsActive || m_SlideTextures.empty())
            return;

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 viewport = io.DisplaySize;

        // Create fullscreen cutscene window
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
        ImGui::Begin("Cutscene", nullptr, flags);

        // Update timer
        float deltaTime = FrameController::GetDeltaTime();
        m_SlideTimer += deltaTime;

        // Check if it's time to advance to next slide
        if (m_SlideTimer >= m_SlideDuration)
        {
            m_CurrentSlideIndex++;
            m_SlideTimer = 0.0f;

            // Check if slideshow is finished
            if (m_CurrentSlideIndex >= static_cast<int>(m_SlideTextures.size()))
            {
                m_IsFinished = true;
                m_IsActive = false;

                // Load next scene if specified
                if (!m_NextScenePath.empty())
                {
                    SceneManager::GetInstance().OpenScene(m_NextScenePath);
                }

                ImGui::End();
                ImGui::PopStyleVar();
                return;
            }
        }

        // Draw current slide
        if (m_CurrentSlideIndex < static_cast<int>(m_SlideTextures.size()))
        {
            auto& currentTexture = m_SlideTextures[m_CurrentSlideIndex];

            if (currentTexture && currentTexture->IsValid())
            {
                ImGui::SetCursorPos(ImVec2(0, 0));
                ImGui::Image(
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W) || defined(IMGUI_IMPL_OPENGL_ES2) || defined(IMGUI_IMPL_OPENGL_ES3) || defined(IMGUI_IMPL_OPENGL_LOADER_GLEW) || defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
                    (ImTextureID)(intptr_t)currentTexture->GetRendererID(),
#else
                    currentTexture->GetRendererID(),
#endif
                    viewport,
                    ImVec2(0, 1), ImVec2(1, 0)  // UV coordinates (flipped vertically)
                );
            }

            // Render caption if available
            if (m_CurrentSlideIndex < static_cast<int>(m_Captions.size()) && !m_Captions[m_CurrentSlideIndex].empty())
            {
                RenderCaption(m_Captions[m_CurrentSlideIndex], viewport);
            }
        }

        // Optional: Draw progress indicator (small dots at bottom)
        if (m_SlideTextures.size() > 1)
        {
            float dotRadius = 5.0f;
            float dotSpacing = 15.0f;
            float totalWidth = (m_SlideTextures.size() * dotSpacing);
            float startX = (viewport.x - totalWidth) / 2.0f;
            float y = viewport.y - 30.0f;

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            for (int i = 0; i < static_cast<int>(m_SlideTextures.size()); i++)
            {
                float x = startX + (i * dotSpacing);
                ImU32 color = (i == m_CurrentSlideIndex)
                    ? IM_COL32(255, 255, 255, 255)  // Current slide - white
                    : IM_COL32(128, 128, 128, 128); // Other slides - gray

                drawList->AddCircleFilled(ImVec2(x, y), dotRadius, color);
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void CutsceneGUI::LoadSlideshow(const std::vector<std::string>& imagePaths, const std::vector<std::string>& captions, float duration)
    {
        m_SlideTextures.clear();
        m_Captions.clear();
        m_SlideDuration = duration;
        m_CurrentSlideIndex = 0;
        m_SlideTimer = 0.0f;
        m_IsFinished = false;

        // Load all textures
        for (const auto& path : imagePaths)
        {
            auto texture = AssetManager::GetInstance().LoadTexture(path);
            if (texture && texture->IsValid())
            {
                m_SlideTextures.push_back(texture);
            }
            else
            {
                EE_CORE_WARN("Failed to load cutscene slide: {}", path);
            }
        }

        // Store captions
        m_Captions = captions;

        if (m_SlideTextures.empty())
        {
            EE_CORE_ERROR("No valid slides loaded for cutscene!");
        }
    }

    void CutsceneGUI::StartSlideshow()
    {
        if (m_SlideTextures.empty())
        {
            EE_CORE_WARN("Cannot start slideshow - no slides loaded!");
            return;
        }

        m_IsActive = true;
        m_IsFinished = false;
        m_CurrentSlideIndex = 0;
        m_SlideTimer = 0.0f;
    }

    void CutsceneGUI::StopSlideshow()
    {
        m_IsActive = false;
        m_IsFinished = true;
    }

    void CutsceneGUI::RenderCaption(const std::string& caption, const ImVec2& viewport)
    {
        // Caption styling - like movie subtitles
        const float padding = 20.0f;
        const float captionHeight = 80.0f;
        const float captionY = viewport.y - captionHeight - padding;

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Calculate text size for proper background sizing
        ImFont* font = ImGui::GetFont();
        float fontSize = font->FontSize * 1.2f; // Slightly larger text
        ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, viewport.x - (padding * 4), caption.c_str());

        // Center the caption horizontally
        float textX = (viewport.x - textSize.x) / 2.0f;
        float backgroundX = textX - padding;
        float backgroundWidth = textSize.x + (padding * 2);

        // Draw semi-transparent black background for readability
        ImVec2 bgMin(backgroundX, captionY);
        ImVec2 bgMax(backgroundX + backgroundWidth, captionY + captionHeight);
        drawList->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 180), 5.0f); // Rounded corners

        // Draw caption text in white
        ImVec2 textPos(textX, captionY + (captionHeight - textSize.y) / 2.0f);
        drawList->AddText(font, fontSize, textPos, IM_COL32(255, 255, 255, 255), caption.c_str(), nullptr, viewport.x - (padding * 4));
    }
}
