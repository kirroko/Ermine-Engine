/* Start Header ************************************************************************/
/*!
\file       Window.h
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This reflects the brief of the Window.h file.
            This file is used to create a window using GLFW.
            This file is included in all the source files
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"
#include "GLFW/glfw3.h"

namespace Ermine
{
    static int window_width, window_height;

    /**
     * @brief The Window class
     */
    class EE_API Window
    {
    public:
        /** 
         * @brief Initialize the window context using GLFW
         * @param width The width of the window
         * @param height The height of the window
         * @param title The title of the window
         * @return The window
         */
        static GLFWwindow* InitWindow(int width, int height, const char* title);
        /** 
         * @brief Check if the window should close
         * @param window The window to check
         * @return True if the window should close, false otherwise
         */
        static bool ShouldCloseWindow(GLFWwindow* window);
        /** 
         * @brief Shut down the window, destroy the window and terminate GLFW, basically closes the application
         * @param window The window to swap the buffers
         */
        static void ShutDownWindow(GLFWwindow* window);
    };
}
