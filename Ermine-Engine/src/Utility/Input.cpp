/* Start Header ************************************************************************/
/*!
\file       Input.cpp
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       12/03/2025
\brief      This file contains the implementation of the Input system.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "Input.h"
#include <GLFW/glfw3.h>
#include "Logger.h"

namespace Ermine
{
    GLFWwindow* Input::s_Window = nullptr;
    float Input::s_LastMouseX = 0.0f;
    float Input::s_LastMouseY = 0.0f;
    float Input::s_MouseDeltaX = 0.0f;
    float Input::s_MouseDeltaY = 0.0f;
    float Input::s_MouseScrollOffset = 0.0f;
    std::unordered_map<int, bool> Input::s_PreviousKeyStates;
    std::unordered_map<int, bool> Input::s_PreviousMouseButtonStates;

    void Input::Init(GLFWwindow* window)
    {
        s_Window = window;
        if (!s_Window)
        {
            EE_CORE_ERROR("Input system initialized with null window!");
            return;
        }

        // Scroll callback
        glfwSetScrollCallback(window, [](GLFWwindow* window, double offsetX, double offsetY)
        {
           s_MouseScrollOffset += static_cast<float>(offsetY); 
        });
        
        double mouseX, mouseY;
        glfwGetCursorPos(s_Window, &mouseX, &mouseY);
        s_LastMouseX = static_cast<float>(mouseX);
        s_LastMouseY = static_cast<float>(mouseY);
        
        EE_CORE_INFO("Input system initialized");
    }

    void Input::Update()
    {
        // Update mouse delta
        double mouseX, mouseY;
        glfwGetCursorPos(s_Window, &mouseX, &mouseY);
        
        s_MouseDeltaX = static_cast<float>(mouseX) - s_LastMouseX;
        s_MouseDeltaY = static_cast<float>(mouseY) - s_LastMouseY;
        
        s_LastMouseX = static_cast<float>(mouseX);
        s_LastMouseY = static_cast<float>(mouseY);
        
        // Update previous frame key states
        for (auto& [key, state] : s_PreviousKeyStates)
        {
            state = IsKeyDown(key);
        }
        
        // Update previous frame mouse button states
        for (auto& [button, state] : s_PreviousMouseButtonStates)
        {
            state = IsMouseButtonDown(button);
        }
    }

    bool Input::IsKeyPressed(int keyCode)
    {
        if (!s_Window)
            return false;
            
        // Check if key exists in previous states map
        auto it = s_PreviousKeyStates.find(keyCode);
        if (it == s_PreviousKeyStates.end())
        {
            s_PreviousKeyStates[keyCode] = false;
        }
        
        bool previous = s_PreviousKeyStates[keyCode];
        bool current = IsKeyDown(keyCode);
        
        return current && !previous;
    }

    bool Input::IsKeyReleased(int keyCode)
    {
        if (!s_Window)
            return false;
            
        // Check if key exists in previous states map
        auto it = s_PreviousKeyStates.find(keyCode);
        if (it == s_PreviousKeyStates.end())
        {
            s_PreviousKeyStates[keyCode] = false;
        }
        
        bool previous = s_PreviousKeyStates[keyCode];
        bool current = IsKeyDown(keyCode);
        
        return !current && previous;
    }

    bool Input::IsKeyDown(int keyCode)
    {
        if (!s_Window)
            return false;
            
        auto state = glfwGetKey(s_Window, keyCode);
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool Input::IsMouseButtonPressed(int button)
    {
        if (!s_Window)
            return false;
            
        // Check if button exists in previous states map
        auto it = s_PreviousMouseButtonStates.find(button);
        if (it == s_PreviousMouseButtonStates.end())
        {
            s_PreviousMouseButtonStates[button] = false;
        }
        
        bool previous = s_PreviousMouseButtonStates[button];
        bool current = IsMouseButtonDown(button);
        
        return current && !previous;
    }

    bool Input::IsMouseButtonReleased(int button)
    {
        if (!s_Window)
            return false;
            
        // Check if button exists in previous states map
        auto it = s_PreviousMouseButtonStates.find(button);
        if (it == s_PreviousMouseButtonStates.end())
        {
            s_PreviousMouseButtonStates[button] = false;
        }
        
        bool previous = s_PreviousMouseButtonStates[button];
        bool current = IsMouseButtonDown(button);
        
        return !current && previous;
    }

    bool Input::IsMouseButtonDown(int button)
    {
        if (!s_Window)
            return false;
            
        auto state = glfwGetMouseButton(s_Window, button);
        return state == GLFW_PRESS;
    }

    std::pair<float, float> Input::GetMousePosition()
    {
        if (!s_Window)
            return {0.0f, 0.0f};
            
        double mouseX, mouseY;
        glfwGetCursorPos(s_Window, &mouseX, &mouseY);
        
        return {static_cast<float>(mouseX), static_cast<float>(mouseY)};
    }

    float Input::GetMouseX()
    {
        return GetMousePosition().first;
    }

    float Input::GetMouseY()
    {
        return GetMousePosition().second;
    }

    float Input::GetMouseScrollOffset()
    {
        return s_MouseScrollOffset;
    }

    void Input::ResetMouseScrollOffset()
    {
        s_MouseScrollOffset = 0.0f;
    }

    std::pair<float, float> Input::GetMouseDelta()
    {
        return {s_MouseDeltaX, s_MouseDeltaY};
    }
}