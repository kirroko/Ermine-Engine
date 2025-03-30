/* Start Header ************************************************************************/
/*!
\file       Input.h
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       12/03/2025
\brief      This file contains the Input system for handling keyboard, mouse and gamepad input.
            This file is used to manage the input system for the engine.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"

#include "imgui.h"

struct GLFWwindow;

namespace Ermine
{
    class EE_API Input
    {
    public:
        // Gamepad
        /*!
        \brief Checks if a gamepad button is currently pressed.
        \param JoystickID The ID of the joystick (GLFW_JOYSTICK_1 through GLFW_JOYSTICK_16).
        \param ButtonID The button ID to check.
        \return True if the button is pressed, false otherwise.
        */
        static bool IsGamepadButtonPressed(int JoystickID, int ButtonID);

        /*!
        \brief Checks if a gamepad button is triggered (pressed for the first time).
        \param JoystickID The ID of the joystick (GLFW_JOYSTICK_1 through GLFW_JOYSTICK_16).
        \param ButtonID The button ID to check.
        \return True if the button is triggered, false otherwise.
        */
        static bool IsGamepadButtonTriggered(int JoystickID, int ButtonID);

        /*!
        \brief Gets the joystick axes values.
        \param JoystickID The ID of the joystick (GLFW_JOYSTICK_1 through GLFW_JOYSTICK_16).
        \param deadzone The deadzone value for the joystick axes.
        \return A vector of float values representing joystick axis positions, or empty if joystick is not present.
        */
        static std::vector<float> GetJoystickAxes(int JoystickID, float deadzone);
        
        // Keyboard
        /**
         * @brief Check if the key is pressed
         * @param keyCode The key to check
         * @return true if the key is pressed
         */
        static bool IsKeyPressed(int keyCode);
        /**
         * @brief Check if the key is released
         * @param keyCode The key to check
         * @return true if the key is released
         */
        static bool IsKeyReleased(int keyCode);
        /**
         * @brief Check if the key is down
         * @param keyCode The key to check
         * @return true if the key is down
         */
        static bool IsKeyDown(int keyCode);
        
        // Mouse
        /**
         * @brief Check if the mouse button is pressed
         * @param button The button to check
         * @return true if the button is pressed
         */
        static bool IsMouseButtonPressed(int button);
        /**
         * @brief Check if the mouse button is released
         * @param button The button to check
         * @return true if the button is released
         */
        static bool IsMouseButtonReleased(int button);
        /**
         * @brief Check if the mouse button is down
         * @param button The button to check
         * @return true if the button is down
         */
        static bool IsMouseButtonDown(int button);
        /**
         * @brief Get the mouse position
         * @return The mouse position
         */
        static std::pair<float, float> GetMousePosition();
        /**
         * @brief Get the mouse x position
         * @return The mouse x position
         */
        static float GetMouseX();
        /**
         * @brief Get the mouse y position
         * @return The mouse y position
         */
        static float GetMouseY();
        /**
         * @brief Get the mouse scroll offset
         * @return The mouse scroll offset
         */
        static float GetMouseScrollOffset();
        /**
         * @brief Reset the mouse scroll accumulation
         */
        static void ResetMouseScrollOffset();
        /**
         * @brief Get the mouse delta
         * @return The mouse delta
         */
        static std::pair<float, float> GetMouseDelta();
        
        // Initialization
        /**
         * @brief Initialize the input system
         * @param window The window to initialize the input system with
         */
        static void Init(GLFWwindow* window);
        /**
         * @brief Update the input system
         */
        static void Update();
        
    private:
        static GLFWwindow* s_Window;
        static float s_LastMouseX;
        static float s_LastMouseY;
        static float s_MouseDeltaX;
        static float s_MouseDeltaY;
        static float s_MouseScrollOffset;
        
        // Track previous frame's key/mouse button states
        static std::unordered_map<int, bool> s_PreviousKeyStates;
        static std::unordered_map<int, bool> s_PreviousMouseButtonStates;

		// Helper function to convert GLFW key to ImGui key
        static ImGuiKey GlfwKeyToImguiKey(int key);
    };
}
