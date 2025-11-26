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
#include "GLFW/glfw3.h"

#ifdef EE_EDITOR
#include "EditorGUI.h"
#include "imgui.h"
#endif

namespace Ermine
{
    void UIButtonSystem::Init()
    {
        EE_CORE_INFO("UIButtonSystem initialized");
    }

    void UIButtonSystem::Update(float deltaTime)
    {
        auto& ecs = ECS::GetInstance();
        
        // Get normalized mouse position once per frame
        float mouseX, mouseY;
        GetNormalizedMousePosition(mouseX, mouseY);
        
        #ifdef EE_EDITOR
        // Debug: Log viewport info and mouse position every frame when preview is active
        if (editor::EditorGUI::isPreviewingUI)
        {
            EE_CORE_TRACE("=== FRAME DEBUG ===");
            EE_CORE_TRACE("Viewport Min: ({}, {})", m_viewportMin.x, m_viewportMin.y);
            EE_CORE_TRACE("Viewport Size: ({}, {})", m_viewportSize.x, m_viewportSize.y);
            EE_CORE_TRACE("Normalized Mouse: ({}, {})", mouseX, mouseY);
        }
        #endif
        
        // Iterate through all entities that have UIButtonComponent
        for (EntityID entity : m_Entities)
        {
            // Safety check: ensure entity exists and has UIButtonComponent
            if (!ecs.IsEntityValid(entity) || !ecs.HasComponent<UIButtonComponent>(entity))
            {
                continue;
            }

            auto& buttonComp = ecs.GetComponent<UIButtonComponent>(entity);

            // Check if mouse is over button using button's own position and size
            bool isHovered = IsMouseOverButton(buttonComp, mouseX, mouseY);
            
            #ifdef EE_EDITOR
            // Debug: Log button info when preview is active
            if (editor::EditorGUI::isPreviewingUI)
            {
                EE_CORE_TRACE("Button '{}': Pos=({}, {}), Size=({}, {}), Hovered={}", 
                    buttonComp.text, 
                    buttonComp.position.x, buttonComp.position.y,
                    buttonComp.size.x, buttonComp.size.y,
                    isHovered);
            }
            #endif
            
            // Update hover state
            if (isHovered && !buttonComp.isHovered)
            {
                buttonComp.isHovered = true;
                EE_CORE_INFO("Button '{}' HOVER START", buttonComp.text);
            }
            else if (!isHovered && buttonComp.isHovered)
            {
                buttonComp.isHovered = false;
                buttonComp.isPressed = false;
            }

            // Check for mouse click
            if (isHovered && Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
            {
                buttonComp.isPressed = true;
                EE_CORE_WARN("Button '{}' CLICKED!", buttonComp.text);
                ExecuteButtonAction(buttonComp);
            }
            else if (!Input::IsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
            {
                buttonComp.isPressed = false;
            }
        }
    }

    bool UIButtonSystem::IsMouseOverButton(const UIButtonComponent& button, float mouseX, float mouseY)
    {
        // Get window aspect ratio (same as used in rendering)
        auto window = glfwGetCurrentContext();
        if (!window) return false;

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        float aspectRatio = (windowHeight > 0) ? static_cast<float>(windowWidth) / static_cast<float>(windowHeight) : 1.0f;

        // Apply same aspect ratio correction as rendering
        float halfWidth = button.size.x * 0.5f;
        float halfHeight = button.size.y * 0.5f;

        // Adjust width for aspect ratio (same as RenderButton does)
        float adjustedHalfWidth = halfWidth / aspectRatio;

        // Calculate bounds with aspect ratio correction
        float left = button.position.x - adjustedHalfWidth;
        float right = button.position.x + adjustedHalfWidth;
        float top = button.position.y + halfHeight;
        float bottom = button.position.y - halfHeight;

        #ifdef EE_EDITOR
        // Debug: Log detailed button bounds calculation
        if (editor::EditorGUI::isPreviewingUI)
        {
            EE_CORE_TRACE("  Window: {}x{}, Aspect: {}", windowWidth, windowHeight, aspectRatio);
            EE_CORE_TRACE("  Half Size: ({}, {}), Adjusted HalfWidth: {}", halfWidth, halfHeight, adjustedHalfWidth);
            EE_CORE_TRACE("  Bounds: Left={}, Right={}, Top={}, Bottom={}", left, right, top, bottom);
            EE_CORE_TRACE("  Mouse: ({}, {}), InBounds: {}", mouseX, mouseY, 
                (mouseX >= left && mouseX <= right && mouseY >= bottom && mouseY <= top));
        }
        #endif

        return (mouseX >= left && mouseX <= right && mouseY >= bottom && mouseY <= top);
    }

    void UIButtonSystem::ExecuteButtonAction(const UIButtonComponent& button)
    {
        EE_CORE_INFO("Button '{}' clicked with action: {}", button.text, static_cast<int>(button.action));

        switch (button.action)
        {
        case UIButtonComponent::ButtonAction::LoadScene:
            if (!button.actionData.empty())
            {
                EE_CORE_INFO("Loading scene: {}", button.actionData);
                #ifdef EE_EDITOR
                auto& sceneManager = SceneManager::GetInstance();
                sceneManager.OpenScene(button.actionData);
                #else
                SceneManager::GetInstance().OpenScene(button.actionData);
                #endif
            }
            else
            {
                EE_CORE_WARN("LoadScene action has no scene path!");
            }
            break;

        case UIButtonComponent::ButtonAction::Quit:
            EE_CORE_INFO("Quit action triggered");
            // Get GLFW window and request close
            if (auto* window = glfwGetCurrentContext())
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            break;

        case UIButtonComponent::ButtonAction::Custom:
            EE_CORE_INFO("Custom action triggered: {}", button.actionData);
            // Custom actions could be handled by scripts or events
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
        // In editor: use viewport-relative mouse position
        ImGuiIO& io = ImGui::GetIO();
        
        // Debug: Log raw ImGui mouse position
        if (editor::EditorGUI::isPreviewingUI)
        {
            EE_CORE_TRACE("  Raw ImGui MousePos: ({}, {})", io.MousePos.x, io.MousePos.y);
        }
        
        // Get mouse position relative to viewport
        float localX = io.MousePos.x - m_viewportMin.x;
        float localY = io.MousePos.y - m_viewportMin.y;
        
        // Debug: Log local position calculation
        if (editor::EditorGUI::isPreviewingUI)
        {
            EE_CORE_TRACE("  Local (relative to viewport): ({}, {})", localX, localY);
        }
        
        // Check if mouse is within viewport bounds
        if (localX < 0.0f || localY < 0.0f || 
            localX > m_viewportSize.x || localY > m_viewportSize.y)
        {
            outX = -1.0f;
            outY = -1.0f;
            
            if (editor::EditorGUI::isPreviewingUI)
            {
                EE_CORE_TRACE("  Mouse OUTSIDE viewport bounds -> (-1, -1)");
            }
            return;
        }
        
        // Normalize to 0-1 range
        outX = localX / m_viewportSize.x;
        outY = 1.0f - (localY / m_viewportSize.y); // Flip Y axis
        
        if (editor::EditorGUI::isPreviewingUI)
        {
            EE_CORE_TRACE("  Normalized result: ({}, {}) [Y flipped]", outX, outY);
        }
        
        #else
        // In game: use full window mouse position
        auto [mouseX, mouseY] = Input::GetMousePosition();
        auto window = glfwGetCurrentContext();
        if (!window) {
            outX = -1.0f;
            outY = -1.0f;
            return;
        }

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        
        // Normalize to 0-1 range where (0,0) = bottom-left of WINDOW
        outX = mouseX / static_cast<float>(windowWidth);
        outY = 1.0f - (mouseY / static_cast<float>(windowHeight)); // Flip Y axis
        
        // Clamp to valid range
        const float TOLERANCE = 0.001f;
        if (outX < -TOLERANCE || outX > 1.0f + TOLERANCE || outY < -TOLERANCE || outY > 1.0f + TOLERANCE) {
            outX = -1.0f;
            outY = -1.0f;
        } else {
            outX = (outX < 0.0f) ? 0.0f : (outX > 1.0f) ? 1.0f : outX;
            outY = (outY < 0.0f) ? 0.0f : (outY > 1.0f) ? 1.0f : outY;
        }
        #endif
    }
}