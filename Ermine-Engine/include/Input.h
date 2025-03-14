/* Start Header ************************************************************************/
/*!
\file       Input.h
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       12/03/2025
\brief      This file contains the Input system for handling keyboard and mouse input.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"

struct GLFWwindow;

namespace Ermine
{
    class EE_API Input
    {
    public:
        // Keyboard
        static bool IsKeyPressed(int keyCode);
        static bool IsKeyReleased(int keyCode);
        static bool IsKeyDown(int keyCode);
        
        // Mouse
        static bool IsMouseButtonPressed(int button);
        static bool IsMouseButtonReleased(int button);
        static bool IsMouseButtonDown(int button);
        static std::pair<float, float> GetMousePosition();
        static float GetMouseX();
        static float GetMouseY();
        static std::pair<float, float> GetMouseDelta();
        
        // Initialization
        static void Init(GLFWwindow* window);
        static void Update();
        
    private:
        static GLFWwindow* s_Window;
        static float s_LastMouseX;
        static float s_LastMouseY;
        static float s_MouseDeltaX;
        static float s_MouseDeltaY;
        
        // Track previous frame's key/mouse button states
        static std::unordered_map<int, bool> s_PreviousKeyStates;
        static std::unordered_map<int, bool> s_PreviousMouseButtonStates;
    };
}
