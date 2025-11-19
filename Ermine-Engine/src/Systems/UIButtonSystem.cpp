/* Start Header ************************************************************************/
/*!
\file       UIButtonSystem.cpp
\author     Claude Code
\date       11/2025
\brief      Implementation of UI button system for menu interactions

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "UIButtonSystem.h"
#include "ECS.h"
#include "Input.h"
#include "SceneManager.h"
#include "Logger.h"
#include "Window.h"
#include <GLFW/glfw3.h>

namespace Ermine
{
    void UIButtonSystem::Init()
    {
        // Set system signature to only process entities with UIButtonComponent
        SignatureID signature;
        signature.set(ECS::GetInstance().GetComponentType<UIButtonComponent>());
        ECS::GetInstance().SetSystemSignature<UIButtonSystem>(signature);

        EE_CORE_INFO("UIButtonSystem initialized");
    }

    void UIButtonSystem::Update(float deltaTime)
    {
        (void)deltaTime; // Unused

        // Get normalized mouse position
        float mouseX, mouseY;
        GetNormalizedMousePosition(mouseX, mouseY);

        // Get current mouse button state
        bool isMousePressed = Input::IsMouseButtonDown(0); // Left mouse button

        // Debug: Log mouse position and entity count
        static int frameCount = 0;
        if (frameCount % 60 == 0) // Log every 60 frames
        {
            EE_CORE_INFO("UIButtonSystem: Mouse pos ({:.3f}, {:.3f}), Entities: {}", mouseX, mouseY, m_Entities.size());
        }
        frameCount++;

        // Process all button entities
        for (EntityID entity : m_Entities)
        {
            auto& button = ECS::GetInstance().GetComponent<UIButtonComponent>(entity);

            // Debug: Log button bounds
            float halfWidth = button.size.x * 0.5f;
            float halfHeight = button.size.y * 0.5f;
            float minX = button.position.x - halfWidth;
            float maxX = button.position.x + halfWidth;
            float minY = button.position.y - halfHeight;
            float maxY = button.position.y + halfHeight;

            if (frameCount % 60 == 0)
            {
                EE_CORE_INFO("  Button '{}' bounds: X[{:.3f}-{:.3f}] Y[{:.3f}-{:.3f}]",
                    button.text.empty() ? "unnamed" : button.text, minX, maxX, minY, maxY);
            }

            // Check if mouse is over button
            bool wasHovered = button.isHovered;
            button.isHovered = IsMouseOverButton(button, mouseX, mouseY);

            // Log hover state changes
            if (button.isHovered && !wasHovered)
            {
                EE_CORE_TRACE("Button '{}' hovered", button.text);
            }

            // Handle button press/release
            if (button.isHovered)
            {
                if (isMousePressed && !m_wasMousePressed)
                {
                    // Button pressed
                    button.isPressed = true;
                    EE_CORE_INFO("Button '{}' pressed", button.text);
                }
                else if (!isMousePressed && m_wasMousePressed && button.isPressed)
                {
                    // Button released (clicked!)
                    button.isPressed = false;
                    EE_CORE_INFO("Button '{}' clicked! Executing action...", button.text);
                    ExecuteButtonAction(button);
                }
            }
            else
            {
                // Mouse not over button, reset pressed state
                button.isPressed = false;
            }
        }

        m_wasMousePressed = isMousePressed;
    }

    bool UIButtonSystem::IsMouseOverButton(const UIButtonComponent& button, float mouseX, float mouseY)
    {
        // Calculate button bounds (centered on position)
        float halfWidth = button.size.x * 0.5f;
        float halfHeight = button.size.y * 0.5f;

        float minX = button.position.x - halfWidth;
        float maxX = button.position.x + halfWidth;
        float minY = button.position.y - halfHeight;
        float maxY = button.position.y + halfHeight;

        return (mouseX >= minX && mouseX <= maxX && mouseY >= minY && mouseY <= maxY);
    }

    void UIButtonSystem::ExecuteButtonAction(const UIButtonComponent& button)
    {
        switch (button.action)
        {
        case UIButtonComponent::ButtonAction::LoadScene:
            if (!button.actionData.empty())
            {
                EE_CORE_INFO("Loading scene: {}", button.actionData);
                SceneManager::GetInstance().OpenScene(button.actionData);
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
        // Get mouse position in pixels
        auto [mousePixelX, mousePixelY] = Input::GetMousePosition();

        // Get window size
        auto* window = glfwGetCurrentContext();
        if (!window)
        {
            outX = outY = 0.0f;
            return;
        }

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        // Normalize to 0-1 range
        outX = mousePixelX / static_cast<float>(windowWidth);
        outY = mousePixelY / static_cast<float>(windowHeight);

        // Flip Y coordinate (GLFW has origin at top-left, we want bottom-left)
        outY = 1.0f - outY;
    }
}
