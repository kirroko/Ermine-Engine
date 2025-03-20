/* Start Header ************************************************************************/
/*!
\file       Application.cpp
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       15/03/2025
\brief      Main application file for the Ermine editor.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/

#include <Window.h> // Window management for editor
#include <Engine.h> // Core engine systems
#include <FrameController.h> // Frame rate controller
#include <Logger.h> // Logging system
#include <Input.h> // Input system

using namespace Ermine;

int main()
{
    // TODO: Implement ImGUI 
    Logger::Init();
    EE_CORE_INFO("Logger Initialized");
    GLFWwindow* window = Window::InitWindow(1920,1080, "Ermine Editor 0.1");
    if (window == nullptr)
        return -1;

    if (!Engine::Init(window)) // if engine fails to initialize
        return -1;
    
    bool running = true;
    while (running && !Window::ShouldCloseWindow(window))
    {
        Engine::Update(window);
        Engine::Render(window);
    
        if (Input::IsKeyDown(GLFW_KEY_ESCAPE))
            running = false;
    }

    Engine::Shutdown();
    Window::ShutDownWindow(window);
    
    return 0;
}