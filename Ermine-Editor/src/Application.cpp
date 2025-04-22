/* Start Header ************************************************************************/
/*!
\file       Application.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       15/03/2025
\brief      Main application file for the Ermine editor.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
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

#ifdef _DEBUG
    editor::EditorGUI::Init(window);
#endif

    EE_CORE_INFO("Begin running program...");
    bool running = true;
    while (running && !Window::ShouldCloseWindow(window))
    {
        engine::Update(window);

        if (editor::EditorGUI::IsInit())
            editor::EditorGUI::Update(window); // Update the ImGUI context

        engine::Render(window);
    }

#ifdef _DEBUG
    editor::EditorGUI::ShutDown();
#endif

    engine::Shutdown();
    Window::ShutDownWindow(window);
    
    return 0;
}