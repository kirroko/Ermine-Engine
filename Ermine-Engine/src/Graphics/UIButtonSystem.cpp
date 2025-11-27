/* Start Header ************************************************************************/
/*!
\file       UIButtonSystem.cpp
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee@digipen.edu
\date       11/2025
\brief      System for handling UI button interactions (hover, click, actions)

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "UIButtonSystem.h"
#include "ECS.h"
#include "Input.h"
#include "Components.h"
#include "SceneManager.h"
#include "Logger.h"
#include "AudioManager.h"
#include "AudioSystem.h"
#include "GLFW/glfw3.h"

#ifdef EE_EDITOR
#include "EditorGUI.h"
#include "imgui.h"
#endif

namespace Ermine
{
    void UIButtonSystem::Init(int screenWidth, int screenHeight)
    {
        m_screenWidth = screenWidth;
        m_screenHeight = screenHeight;
        m_aspectRatio = (screenHeight > 0) ? static_cast<float>(screenWidth) / static_cast<float>(screenHeight) : 1.0f;
        EE_CORE_INFO("UIButtonSystem initialized ({}x{}, aspect ratio: {})", screenWidth, screenHeight, m_aspectRatio);
    }

    void UIButtonSystem::Update(float deltaTime)
    {
        auto& ecs = ECS::GetInstance();

#ifdef EE_EDITOR
        // Only allow button interactions in play mode or UI preview mode when in editor
        if (!editor::EditorGUI::isPlaying && !editor::EditorGUI::isPreviewingUI)
            return;
#endif

        // Debug: Log first update call
        static bool firstUpdate = true;
        if (firstUpdate)
        {
            EE_CORE_INFO("UIButtonSystem::Update - First update call");
            EE_CORE_INFO("  Screen: {}x{}, Aspect: {}", m_screenWidth, m_screenHeight, m_aspectRatio);
#ifdef EE_EDITOR
            EE_CORE_INFO("  Viewport: ({}, {}) size: {}x{}",
                         m_viewportMin.x, m_viewportMin.y, m_viewportSize.x, m_viewportSize.y);
#endif
            firstUpdate = false;
        }

        // Get normalized mouse position once per frame
        float mouseX, mouseY;
        GetNormalizedMousePosition(mouseX, mouseY);

        // Debug: Log mouse position
        //EE_CORE_INFO("Mouse Position - X: {}, Y: {}", mouseX, mouseY);

        // Iterate through all entities that have UIButtonComponent
        for (EntityID entity : m_Entities)
        {
            if (!ecs.IsEntityValid(entity) || !ecs.HasComponent<UIButtonComponent>(entity))
                continue;

            auto& button = ecs.GetComponent<UIButtonComponent>(entity);

            // Calculate button bounds in normalized space
            float halfW = (button.size.x * 0.5f) / m_aspectRatio;
            float halfH = button.size.y * 0.5f;
            float left = button.position.x - halfW;
            float right = button.position.x + halfW;
            float bottom = button.position.y - halfH;
            float top = button.position.y + halfH;

            // Debug: Log button bounds and state
            //EE_CORE_INFO("Button: '{}' Bounds - L: {}, R: {}, B: {}, T: {}",
            //             button.text, left, right, bottom, top);
            //EE_CORE_INFO("  Hovered: {}, Pressed: {}", button.isHovered, button.isPressed);

            // Check if mouse is inside button bounds
            bool inside = (mouseX >= left && mouseX <= right && mouseY >= bottom && mouseY <= top);

            // Update hover state
            if (inside && !button.isHovered)
            {
                button.isHovered = true;
                EE_CORE_INFO("✓ Button '{}' HOVER START", button.text);

                // Play hover sound if specified
                if (!button.hoverSoundName.empty())
                {
                    EE_CORE_INFO("Playing hover sound: '{}' at volume {}", button.hoverSoundName, button.soundVolume);
                    float volumeDB = AudioSystem::ConvertVolumeToFMOD(button.soundVolume);
                    int channelId = CAudioEngine::PlaySounds(button.hoverSoundName, Vector3D{0, 0, 0}, volumeDB);
                    EE_CORE_INFO("Hover sound channel ID: {}", channelId);
                }
            }
            else if (!inside && button.isHovered)
            {
                button.isHovered = false;
                button.isPressed = false;
            }

            // Handle click
            if (inside && Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
            {
                button.isPressed = true;
                EE_CORE_WARN("Button '{}' CLICKED!", button.text);

                // Play click sound if specified
                if (!button.clickSoundName.empty())
                {
                    EE_CORE_INFO("Playing click sound: '{}' at volume {}", button.clickSoundName, button.soundVolume);
                    float volumeDB = AudioSystem::ConvertVolumeToFMOD(button.soundVolume);
                    int channelId = CAudioEngine::PlaySounds(button.clickSoundName, Vector3D{0, 0, 0}, volumeDB);
                    EE_CORE_INFO("Click sound channel ID: {}", channelId);
                }

                ExecuteButtonAction(button);
            }
            else if (!Input::IsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
            {
                button.isPressed = false;
            }
        }
        // CRITICAL: Process pending scene load AFTER iteration completes
        if (m_HasPendingSceneLoad)
        {
            EE_CORE_INFO("Executing deferred scene load: {}", m_PendingSceneToLoad);
            try
            {
                #ifdef EE_EDITOR
                auto& sceneManager = SceneManager::GetInstance();
                sceneManager.OpenScene(m_PendingSceneToLoad);
                #else
                SceneManager::GetInstance().OpenScene(m_PendingSceneToLoad);
                #endif
            }
            catch (const std::exception& e)
            {
                EE_CORE_ERROR("Failed to load scene '{}': {}", m_PendingSceneToLoad, e.what());
            }

            m_HasPendingSceneLoad = false;
            m_PendingSceneToLoad.clear();
        }
    }

    void UIButtonSystem::ExecuteButtonAction(const UIButtonComponent& button)
    {
        EE_CORE_INFO("Button '{}' clicked with action: {}", button.text, static_cast<int>(button.action));

        switch (button.action)
        {
        case UIButtonComponent::ButtonAction::LoadScene:
            if (!button.actionData.empty())
            {
                EE_CORE_INFO("Queueing scene load: {}", button.actionData);

                // CRITICAL FIX: Don't load immediately - defer until after Update() completes
                // Store the scene path to load at the end of the frame
                m_PendingSceneToLoad = button.actionData;
                m_HasPendingSceneLoad = true;
            }
            else
            {
                EE_CORE_WARN("LoadScene action has no scene path!");
            }
            break;

        case UIButtonComponent::ButtonAction::Quit:
            EE_CORE_INFO("Quit action triggered");
            if (auto* window = glfwGetCurrentContext())
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            break;

        case UIButtonComponent::ButtonAction::Custom:
            EE_CORE_INFO("Custom action triggered: {}", button.actionData);
            break;

        case UIButtonComponent::ButtonAction::None:
        default:
            EE_CORE_WARN("Button '{}' has no action assigned", button.text);
            break;
        }
    }

    void UIButtonSystem::GetNormalizedMousePosition(float& outX, float& outY)
    {
        #ifdef EE_EDITOR
        // EDITOR MODE: Get mouse position from ImGui
        ImGuiIO& io = ImGui::GetIO();

        // Get mouse position in screen space
        float screenX = io.MousePos.x;
        float screenY = io.MousePos.y;

        // Convert to viewport-relative coordinates
        float viewportX = screenX - m_viewportMin.x;
        float viewportY = screenY - m_viewportMin.y;

        // Check if mouse is outside viewport
        if (viewportX < 0.0f || viewportY < 0.0f ||
            viewportX > m_viewportSize.x || viewportY > m_viewportSize.y)
        {
            outX = -1.0f;
            outY = -1.0f;
            return;
        }

        // Normalize to 0-1 range
        // ImGui Y goes down (0 at top), OpenGL Y goes up (0 at bottom)
        outX = viewportX / m_viewportSize.x;
        outY = 1.0f - (viewportY / m_viewportSize.y);  // Flip Y

        #else
        // GAME MODE: Get mouse position from GLFW
        auto [mouseX, mouseY] = Input::GetMousePosition();
        auto window = glfwGetCurrentContext();
        if (!window)
        {
            outX = -1.0f;
            outY = -1.0f;
            return;
        }

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        // Normalize to 0-1 range
        outX = mouseX / static_cast<float>(windowWidth);
        outY = 1.0f - (mouseY / static_cast<float>(windowHeight));  // Flip Y

        // Clamp
        outX = std::max(0.0f, std::min(1.0f, outX));
        outY = std::max(0.0f, std::min(1.0f, outY));
        #endif
    }
}
