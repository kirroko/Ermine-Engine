/* Start Header ************************************************************************/
/*!
\file       Window.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       09/03/2025
\brief      This file contains the definition of the Window system.
            This file is used to create a window using GLFW.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "Window.h"

#include "glad/glad.h"

/**
 * @brief Initialize the window, You can find openGL (MSAA, V-Sync) settings here
 * 
 * @param width The width of the window
 * @param height The height of the window
 * @param title The title of the window
 * @return GLFWwindow* The window pointer
 */
GLFWwindow* Ermine::Window::InitWindow(int width, int height, const char* title)
{
    EE_CORE_TRACE("Initializing Window...");
    window_width = width;
    window_height = height;

    glfwSetErrorCallback([]([[maybe_unused]] int error , const char* description) { EE_CORE_ERROR("GLFW Error: {0}", description); });
    
    if (!glfwInit())
    {
        EE_CORE_INFO("Failed to initialize GLFW");
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // MSAA
    glfwWindowHint(GLFW_SAMPLES, 4);

    // borderless full screen
    // GLFWmonitor *mon = NULL;
    // mon = glfwGetPrimaryMonitor();
    //
    // const GLFWvidmode* mode = glfwGetVideoMode( mon );
    // // Hinting these properties lets us use "borderless full screen" mode.
    // glfwWindowHint( GLFW_RED_BITS, mode->redBits );
    // glfwWindowHint( GLFW_GREEN_BITS, mode->greenBits );
    // glfwWindowHint( GLFW_BLUE_BITS, mode->blueBits );
    // glfwWindowHint( GLFW_REFRESH_RATE, mode->refreshRate );
    // GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, title, nullptr, nullptr);

    GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window)
    {
        EE_CORE_ERROR("Failed to create window");
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    // We'll like to initialize GLAD as well...
    if (!gladLoadGL())
    {
        EE_CORE_ERROR("Failed to initialize GLAD!");
        glfwTerminate(); // GLAD failed, shouldn't continue to run
        return nullptr;
    }

    glViewport(0,0,width,height);
    
    glfwSwapInterval(1); // Enable V-Sync

    glEnable(GL_DEPTH_TEST);

    // Input class handle this part already
    //glfwSetMouseButtonCallback(window,nullptr);
    //glfwSetScrollCallback(window,nullptr);
    //glfwSetKeyCallback(window,nullptr);
    //glfwSetCharCallback(window,nullptr);

    std::string glRenderer = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    std::string glVersion = std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    EE_CORE_TRACE("Renderer: {0}", glRenderer);
    EE_CORE_TRACE("OpenGL version supported {0}", glVersion);
    
    EE_CORE_INFO("Window creation with width: {0}, height: {1}, title: {2}", width, height, title);
    return window;
}

/**
 * @brief Check if the window should close
 * 
 * @param window The window to check
 * @return true if the window should close, false otherwise
 */
bool Ermine::Window::ShouldCloseWindow(GLFWwindow* window)
{
    return glfwWindowShouldClose(window);
}

/**
 * @brief Shut down the window
 * 
 * @param window The window to shut down
 */
void Ermine::Window::ShutDownWindow(GLFWwindow* window)
{
    EE_CORE_TRACE("Shutting down window...");
    glfwDestroyWindow(window);
    glfwTerminate();
    EE_CORE_INFO("Window terminated successfully!");
}


