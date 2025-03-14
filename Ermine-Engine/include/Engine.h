/* Start Header ************************************************************************/
/*!
\file       Engine.h
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This file contains the declaration of the Engine system.
            This file is used to initialize the engine and run the game loop.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/
#pragma once

#include "EngineAPI.h"
#include "GLFW/glfw3.h"

namespace Ermine::Engine
{
    /**
     * @brief Initialize the engine, load all resources
     * @return true if initialization is successful, false otherwise
     */
    EE_API bool Init(GLFWwindow* windowContext);

    /**
     * @brief Shutdown the engine, release all resources
     */
    EE_API void Shutdown();
    
    /** 
     * @brief Run the game loop
     */
    EE_API void Update(float deltaTime, GLFWwindow* windowContext);

    /**
     * @brief Render the game
     */
    EE_API void Render(GLFWwindow* window);

    // Free to use for testing purposes
    EE_API void Dummy(GLFWwindow* wwindow);
}
