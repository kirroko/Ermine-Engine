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

    /**
     * @brief Load the contents of a file into a buffer.
     * @param filepath The path to the file to load.
     * @return The contents of the file as a buffer.
     */
    const char* load_file_contents(const char* filepath) { // TODO: Move this to a utility file
        FILE* file = fopen(filepath, "rb"); // TODO: Replace deprecated fopen
        if (!file) {
            EE_CORE_ERROR("Failed to open file: {0}", filepath);
            return nullptr;
        }
            
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        rewind(file);
            
        char* buffer = new char[size + 1];
        size_t read = fread(buffer, 1, size, file);
        buffer[read] = '\0';
            
        fclose(file);
        return buffer;
    }

    /**
     * @brief Initialize the input system
     * @param window The window to initialize the input system with
     */
    void Input::Init(GLFWwindow* window)
    {
        s_Window = window;
        if (!s_Window)
        {
            EE_CORE_ERROR("Input system initialized with null window!");
            return;
        }

        // Scroll callback
        glfwSetScrollCallback(window, []([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] double offsetX, double offsetY)
        {
           s_MouseScrollOffset += static_cast<float>(offsetY); 
        });
        
        double mouseX, mouseY;
        glfwGetCursorPos(s_Window, &mouseX, &mouseY);
        s_LastMouseX = static_cast<float>(mouseX);
        s_LastMouseY = static_cast<float>(mouseY);

        if (const char* keyMap = load_file_contents("../Resources/gamecontrollerdb.txt"))
        {
            EE_CORE_TRACE("Loading game controller database...");
            glfwUpdateGamepadMappings(keyMap);
        }
        
        EE_CORE_INFO("Input system initialized");
    }

    /**
     * @brief Update the input system
     */
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

    /*!
    \brief Checks if a gamepad button is currently pressed.
    \param JoystickID The ID of the joystick (GLFW_JOYSTICK_1 through GLFW_JOYSTICK_16).
    \param ButtonID The button ID to check.
    \return True if the button is pressed, false otherwise.
    */
    bool Input::IsGamepadButtonPressed(int JoystickID, int ButtonID)
    {
        if (!glfwJoystickIsGamepad(JoystickID))
            return false;

        GLFWgamepadstate state;
        if (glfwGetGamepadState(JoystickID,&state) && ButtonID >= 0 && ButtonID <= GLFW_GAMEPAD_BUTTON_LAST)
        {
            return state.buttons[ButtonID] == GLFW_PRESS;
        }

        return false;
    }

    /*!
    \brief Checks if a gamepad button is triggered (pressed for the first time).
    \param JoystickID The ID of the joystick (GLFW_JOYSTICK_1 through GLFW_JOYSTICK_16).
    \param ButtonID The button ID to check.
    \return True if the button is triggered, false otherwise.
    */
    bool Input::IsGamepadButtonTriggered(int JoystickID, int ButtonID)
    {
        if (!glfwJoystickIsGamepad(JoystickID))
            return false;

        GLFWgamepadstate state;
        int uniqueKey = (JoystickID << 16) | ButtonID;
        if (glfwGetGamepadState(JoystickID,&state) && ButtonID >=0 && ButtonID <= GLFW_GAMEPAD_BUTTON_LAST)
        {
            if (state.buttons[ButtonID] == GLFW_PRESS && !s_PreviousKeyStates[uniqueKey])
            {
                s_PreviousKeyStates[uniqueKey] = true;
                return true;
            }
        }

        if (state.buttons[ButtonID] == GLFW_RELEASE)
        {
            s_PreviousKeyStates[uniqueKey] = false;
        }
		
        return false;
    }
    
    /*!
    \brief Gets the joystick axes values.
    \param JoystickID The ID of the joystick (GLFW_JOYSTICK_1 through GLFW_JOYSTICK_16).
    \param deadzone The deadzone value for the joystick axes.
    \return A vector of float values representing joystick axis positions, or empty if joystick is not present.
    */
    std::vector<float> Input::GetJoystickAxes(int JoystickID, float deadzone)
    {
        // Clamp deadzone to valid range [0.0, 1.0]
        deadzone = std::max(0.0f,std::min(deadzone,1.0f));
		
        int axesCount;
        const float* axes = glfwGetJoystickAxes(JoystickID, &axesCount);
    
        if (axes == nullptr)
            return {};

        // Convert the raw pointer to a vector with deadzone applied
        std::vector<float> processedAxes;
        processedAxes.reserve(axesCount);

        for (int i = 0; i < axesCount; ++i)
        {
            float value = axes[i];

            // Apply deadzone
            if (std::abs(value) < deadzone)
            {
                value = 0.0f;
            }
            else
            {
                // Rescale the values outside deadzone to full range
                // This creates a smooth transition from deadzone to max value
                float sign = (value > 0.0f) ? 1.0f : -1.0f;
                value = sign * (std::abs(value) - deadzone) / (1.0f - deadzone);
            }

            processedAxes.push_back(value);
        }
		
        return processedAxes;
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