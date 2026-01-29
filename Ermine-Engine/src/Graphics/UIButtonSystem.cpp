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

        auto& ecs = ECS::GetInstance();
        for (EntityID e = 0; e < MAX_ENTITIES; ++e)
        {
            if (!ecs.IsEntityValid(e)) continue;
            if (!ecs.HasComponent<ObjectMetaData>(e)) continue;

            auto& meta = ecs.GetComponent<ObjectMetaData>(e);
            if (meta.name == "PauseMenu" || meta.name == "PauseBackground" || meta.name == "ResumeButton")
            {
                EE_CORE_INFO("Entity '{}' (ID: {}) has selfActive = {}", meta.name, e, meta.selfActive);
            }
        }

        EE_CORE_INFO("UIButtonSystem::s_isGamePaused = {}", s_isGamePaused);
        EE_CORE_INFO("UIButtonSystem initialized ({}x{}, aspect ratio: {})", screenWidth, screenHeight, m_aspectRatio);

    }

    EntityID UIButtonSystem::GetGlobalAudioEntity()
    {
        auto& ecs = ECS::GetInstance();

        // Check if cached entity is still valid
        if (ecs.IsEntityValid(m_GlobalAudioEntity) &&
            ecs.HasComponent<GlobalAudioComponent>(m_GlobalAudioEntity))
        {
            return m_GlobalAudioEntity;
        }

        // Find and cache GlobalAudio entity
        for (EntityID e = 0; e < MAX_ENTITIES; ++e)
        {
            if (!ecs.IsEntityValid(e)) continue;
            if (!ecs.HasComponent<GlobalAudioComponent>(e)) continue;

            m_GlobalAudioEntity = e;
            EE_CORE_INFO("UIButtonSystem: Cached GlobalAudio entity (ID: {})", e);
            return e;
        }

        // Not found - invalidate cache
        m_GlobalAudioEntity = MAX_ENTITIES;
        return MAX_ENTITIES;
    }

    void UIButtonSystem::Update(float deltaTime)
    {
        auto& ecs = ECS::GetInstance();

#ifdef EE_EDITOR
        // Only allow button interactions in play mode or UI preview mode when in editor
        if (!editor::EditorGUI::isPlaying && !editor::EditorGUI::isPreviewingUI)
            return;
#endif

        // ==================== PAUSE MENU TOGGLE ====================
        static bool pWasPressed = false;
        bool pIsPressed = Input::IsKeyDown(GLFW_KEY_P);

        if (pIsPressed && !pWasPressed)
        {
            TogglePauseMenu();
        }
        pWasPressed = pIsPressed;
        // ===========================================================

        EntityID globalAudioEntity = GetGlobalAudioEntity();
        GlobalAudioComponent* globalAudio = nullptr;

        if (globalAudioEntity != MAX_ENTITIES)
        {
            globalAudio = &ecs.GetComponent<GlobalAudioComponent>(globalAudioEntity);
        }

        // Get normalized mouse position once per frame
        float mouseX, mouseY;
        GetNormalizedMousePosition(mouseX, mouseY);

        // Iterate through all entities that have UIButtonComponent
        for (EntityID entity : m_Entities)
        {
            if (!ecs.IsEntityValid(entity) || !ecs.HasComponent<UIButtonComponent>(entity))
                continue;

            // ✅ FIX: Check if entity is active in hierarchy (including parents)
            if (!IsEntityActiveInHierarchy(entity))
                continue;

            auto& button = ecs.GetComponent<UIButtonComponent>(entity);

            // Calculate button bounds in normalized space
            float halfW = (button.size.x * 0.5f) / m_aspectRatio;
            float halfH = button.size.y * 0.5f;
            float left = button.position.x - halfW;
            float right = button.position.x + halfW;
            float bottom = button.position.y - halfH;
            float top = button.position.y + halfH;

            // Check if mouse is inside button bounds
            bool inside = (mouseX >= left && mouseX <= right && mouseY >= bottom && mouseY <= top);

            // Update hover state
            if (inside && !button.isHovered)
            {
                button.isHovered = true;

                if (globalAudio)
                {
                    AudioSystem::PlayGlobalSFX(*globalAudio, "Hover");
                    EE_CORE_INFO("Playing hover sound");
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
                if (globalAudio)
                {
                    AudioSystem::PlayGlobalSFX(*globalAudio, "Click");
                    EE_CORE_INFO("Playing click sound");
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

    bool UIButtonSystem::IsEntityActiveInHierarchy(EntityID entity)
    {
        auto& ecs = ECS::GetInstance();

        // Check if entity itself is valid
        if (!ecs.IsEntityValid(entity))
            return false;

        // Check self active state
        if (ecs.HasComponent<ObjectMetaData>(entity))
        {
            auto& meta = ecs.GetComponent<ObjectMetaData>(entity);
            if (!meta.selfActive)
                return false;
        }

        // Check parent chain via HierarchyComponent
        if (ecs.HasComponent<HierarchyComponent>(entity))
        {
            auto& hierarchy = ecs.GetComponent<HierarchyComponent>(entity);

            // If has a valid parent, check if parent is active
            if (hierarchy.parent != HierarchyComponent::INVALID_PARENT)
            {
                // Recursively check parent's active state
                return IsEntityActiveInHierarchy(hierarchy.parent);
            }
        }

        // No parent or parent is active - entity is active
        return true;
    }

    void UIButtonSystem::TogglePauseMenu()
    {
        auto& ecs = ECS::GetInstance();

        for (EntityID e = 0; e < MAX_ENTITIES; ++e)
        {
            if (!ecs.IsEntityValid(e)) continue;
            if (!ecs.HasComponent<ObjectMetaData>(e)) continue;

            auto& meta = ecs.GetComponent<ObjectMetaData>(e);
            if (meta.name == "PauseMenu")
            {
                meta.selfActive = !meta.selfActive;
                s_isGamePaused = meta.selfActive;
                EE_CORE_INFO("Game {}", s_isGamePaused ? "PAUSED" : "RESUMED");

#if defined(EE_EDITOR)
                // Update editor state if in editor
                if (editor::EditorGUI::isPlaying)
                {
                    editor::EditorGUI::s_state = s_isGamePaused
                        ? editor::EditorGUI::SimState::paused
                        : editor::EditorGUI::SimState::playing;
                }
#endif

                return;
            }
        }

        EE_CORE_WARN("PauseMenu entity not found in scene!");
    }

    bool UIButtonSystem::IsGamePaused()
    {
        return s_isGamePaused;
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
            if (button.actionData == "Resume" || button.actionData == "resume_game")
            {
                TogglePauseMenu();
                EE_CORE_INFO("Resume button clicked");
            }
            else
            {
                EE_CORE_INFO("Custom action triggered: {}", button.actionData);
            }
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