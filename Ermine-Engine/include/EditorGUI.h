/* Start Header ************************************************************************/
/*!
\file       EditorGUI.h
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       27/03/2025
\brief      This file contains the declaration of the EditorGUI class.
            Function just like a wrapper for the ImGUI library.
            Each window for teh editor should be encapsulated into a function in this class.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/

#pragma once
#include "GLFW/glfw3.h"

namespace Ermine::editor
{
    /**
 * @brief The EditorGUI class, function just like a wrapper for the ImGUI library
 */
    class EE_API EditorGUI
    {
    public:
        /**
         * @brief Initialize the ImGUI context
         * @param window The window to initialize the ImGUI context
         */
        static void Init(GLFWwindow* window);

        /**
        * @brief Update the ImGUI context (Render)
        */
        static void Update();

        /**
         * @brief Render the ImGUI context
         */
        static void Render();

        /**
         * @brief Shut down the ImGUI context
         */
        static void ShutDown();
    };
}
