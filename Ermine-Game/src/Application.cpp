#include "Engine.h"
#include <Window.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	GLFWwindow* window = Ermine::Window::InitWindow(1920, 1080, "Shadow Splitter");
    if (!window) return -1;

    if (!Ermine::engine::Init(window)) return -1;

    while (Ermine::Window::ShouldCloseWindow(window))
    {
        Ermine::engine::Update(window);
        Ermine::engine::Render(window);
    }

    Ermine::engine::Shutdown();
    Ermine::Window::ShutDownWindow(window);
    return 0;
}
