/* Start Header ************************************************************************/
/*!
\file       Window.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\co-author  WEE HUNG RU, Curtis, h.wee, 230xxx, h.wee\@digipen.edu (25%)
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

#include "Serialisation.h"
#include "glad/glad.h"
#include "AssetBrowser.h" // For forwarding dropped files to the asset browser

/**
 * @brief GLFW callback function for handling file drops.
 * This function is registered with GLFW to receive notifications
 * when files are dropped onto the application window. It collects
 * the file paths and forwards them to the asset browser for processing.
 * @param window Pointer to the GLFW window where files were dropped.
 * @param count Number of files dropped.
 * @param paths Array of C-style strings representing the dropped file paths.
 */
static void GLFW_DropCallback(GLFWwindow* window, int count, const char** paths)
{
    // Collect dropped file paths into a vector of strings
    std::vector<std::string> droppedFiles;
    droppedFiles.reserve(count);

    // Copy paths to the vector
    for (int i = 0; i < count; ++i)
        droppedFiles.emplace_back(paths[i]);

    // Log the dropped files and forward them to the asset browser
    EE_CORE_INFO("Dropped {} files into the editor window.", count);
    Ermine::ImguiUI::AssetBrowser::OnExternalFilesDropped(droppedFiles);
}

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

    const std::filesystem::path cfgPath = "Ermine-Engine.config";

    Config cfg{};
    try {
        cfg = LoadConfigFromFile(cfgPath);
        EE_CORE_INFO("Loaded config: {0}x{1}, fullscreen={2}, maximised={3}, title={4}",
            cfg.windowWidth, cfg.windowHeight, cfg.fullscreen, cfg.maximized, cfg.title);
    }
    catch (const std::exception& e) {
        EE_CORE_WARN("Config not found/invalid ({}). Using defaults.", e.what());
        cfg = { .windowWidth = width, .windowHeight = height, .fullscreen = false, .maximized = false, .title = title };

#ifdef EE_RELEASE
        cfg.fullscreen = true; // Force fullscreen on release build
        cfg.maximized = false; // Prevent maximize instead of fullscreen
#endif

        try { SaveConfigToFile(cfg, cfgPath, /*pretty=*/true); }
        catch (const std::exception& w) { EE_CORE_WARN("Could not write default config: {}", w.what()); }
    }

    window_width = cfg.windowWidth;
    window_height = cfg.windowHeight;

    glfwSetErrorCallback([]([[maybe_unused]] int error , const char* description) { EE_CORE_ERROR("GLFW Error: {0}", description); });
    
    if (!glfwInit())
    {
        EE_CORE_ERROR("Failed to initialize GLFW");
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

    GLFWwindow* window = glfwCreateWindow(cfg.windowWidth, cfg.windowHeight, cfg.title.c_str(), nullptr, nullptr);
    if (!window)
    {
        EE_CORE_ERROR("Failed to create window");
        glfwTerminate();
        return nullptr;
    }

    if (cfg.fullscreen)
    {
	    GLFWmonitor* mon = glfwGetPrimaryMonitor();
	    const GLFWvidmode* mode = glfwGetVideoMode(mon);
	    glfwSetWindowMonitor(window, mon, 0, 0,
	        mode->width, mode->height,
	        mode->refreshRate);
    }
    else 
    {
        glfwSetWindowSize(window, cfg.windowWidth, cfg.windowHeight);

        if (cfg.maximized) {
            glfwMaximizeWindow(window);
        }
        else {
            glfwRestoreWindow(window);
        }
    }

    glfwMakeContextCurrent(window);

    // Set the drop callback to handle file drops
    glfwSetDropCallback(window, GLFW_DropCallback);

    // We'll like to initialize GLAD as well...
    if (!gladLoadGL())
    {
        EE_CORE_ERROR("Failed to initialize GLAD!");
        glfwTerminate(); // GLAD failed, shouldn't continue to run
        return nullptr;
    }

    glViewport(0,0,cfg.windowWidth,cfg.windowHeight);
    
    glfwSwapInterval(1); // Enable V-Sync

    glEnable(GL_DEPTH_TEST);

    std::string glRenderer = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    std::string glVersion = std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    EE_CORE_TRACE("Renderer: {0}", glRenderer);
    EE_CORE_TRACE("OpenGL version supported {0}", glVersion);

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

/**
 * @brief Toggle fullscreen mode for the window
 * @param window The window to toggle fullscreen mode
 */
void Ermine::Window::ToggleFullscreenWindow(GLFWwindow* window)
{
    static bool isFullscreen =
#if defined(EE_DEBUG)
        false; // Debug starts in window mode
#else
        true;  // Release starts in fullscreen mode
#endif

    if (!isFullscreen)
    {
        GLFWmonitor* mon = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(mon);

        glfwSetWindowMonitor(window, mon, 0, 0,
            mode->width, mode->height, mode->refreshRate);
    }
    else
    {
        // Restore to windowed mode
        glfwSetWindowMonitor(window, nullptr,
            100, 100, window_width, window_height, 0);
    }

    isFullscreen = !isFullscreen;
}
