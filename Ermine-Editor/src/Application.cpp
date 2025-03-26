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
#include <EditorGUI.h> // ImGUI wrapper for editor
#include <FrameController.h> // Frame rate controller
#include <Logger.h> // Logging system
#include <Input.h> // Input system

using namespace Ermine;

int main()
{
    Logger::Init();
    EE_CORE_INFO("Logger Initialized");
    GLFWwindow* window = Window::InitWindow(1920,1080, "Ermine Editor 0.1");
    if (window == nullptr)
        return -1;

    if (!engine::Init(window)) // if engine fails to initialize
        return -1;

    editor::EditorGUI::Init(window);
    
    bool running = true;
    while (running && !Window::ShouldCloseWindow(window))
    {
        engine::Update(window);

        editor::EditorGUI::Update();
        
        engine::Render(window);
    
        if (Input::IsKeyPressed(GLFW_KEY_ESCAPE))
            running = false;
    }

    editor::EditorGUI::ShutDown();
    engine::Shutdown();
    Window::ShutDownWindow(window);
    
    return 0;
}