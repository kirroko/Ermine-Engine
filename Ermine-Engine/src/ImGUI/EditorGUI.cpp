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
#include "PreCompile.h"
#include "EditorGUI.h"
#include "Logger.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "ECS.h"
#include "EditorCamera.h"
#include "FrameController.h"
#include "Input.h"
#include "Renderer.h"

using namespace Ermine::editor;

void EditorGUI::TopMenuBar(GLFWwindow* windowContext)
{
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Open", "Ctrl+O"))
        {
			EE_CORE_INFO("Open file clicked");
            // Code to open a file, the scene?
        }
        if (ImGui::MenuItem("Save", "Ctrl+S"))
        {
            EE_CORE_INFO("Save file clicked");
            // Code to save a file, maybe the scene
        }
        if (ImGui::MenuItem("Exit", "Alt+F4"))
        {
			EE_CORE_INFO("Exit clicked");
			// Code to exit the application
			glfwSetWindowShouldClose(windowContext, GLFW_TRUE);
        }
		ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
		if (ImGui::MenuItem("Undo", "Ctrl+Z"))
		{
			EE_CORE_INFO("Undo clicked");
			// Code to undo
		}
		if (ImGui::MenuItem("Redo", "Ctrl+Y"))
		{
			EE_CORE_INFO("Redo clicked");
			// Code to redo
		}
		ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

// This function is called before rendering the scene
void EditorGUI::ViewPortWindow(bool &show)
{
    ImGui::Begin("Scene Viewer", &show);

    // Obtain available context region in the window (viewport size)
	ImVec2 viewport_size = ImGui::GetContentRegionAvail();

    // Ensure the viewport size is within an acceptable range
    constexpr int minSize = 1;
    int max_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);

    viewport_size.x = std::clamp(viewport_size.x, static_cast<float>(minSize), static_cast<float>(max_size));
    viewport_size.y = std::clamp(viewport_size.y, static_cast<float>(minSize), static_cast<float>(max_size));

    static bool first_time = true;
    if (first_time)
    {
		ECS::GetInstance().GetSystem<graphics::Renderer>()->Create(static_cast<int>(viewport_size.x), static_cast<int>(viewport_size.y));
        first_time = false;
    }
	const auto offscreen_buffer = ECS::GetInstance().GetSystem<graphics::Renderer>()->GetOffscreenBuffer();
	offscreen_buffer->width = static_cast<int>(viewport_size.x);
	offscreen_buffer->height = static_cast<int>(viewport_size.y);

	EditorCamera::GetInstance().SetViewportSize(viewport_size.x, viewport_size.y);

	ImGui::Image(offscreen_buffer->ColorTexture, viewport_size, ImVec2(0, 1), ImVec2(1, 0));

    if (ImGui::IsWindowHovered())
    {
	    EditorCamera::GetInstance().ProcessMouseMovement();
		EditorCamera::GetInstance().ProcessKeyboardInput(FrameController::GetDeltaTime());
		EditorCamera::GetInstance().ProcessScrollWheel(Input::GetMouseScrollOffset());
    }
	ImGui::End();
}

/**
 * @brief Initialize the ImGUI context
 * @param window The window to initialize the ImGUI context
 */
void EditorGUI::Init(GLFWwindow* window)
{
    // Setup Dear ImGUI context
    EE_CORE_TRACE("Setting up ImGUI...");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // // set custom allocator functions to handle DLL boundaries
    // ImGui::SetAllocatorFunctions(ImGUiMemAlloc, ImGuiMemFree);
    //
    // // Make the ImGui context current
    // ImGui::SetCurrentContext(ImGui::GetCurrentContext());
    
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;     // Enable Multi-Viewport / Platform Windows
    
    // Setup Dear ImGui style
    ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
}

/**
 * @brief Check if the ImGUI context is initialized
 */
bool EditorGUI::IsInit()
{
	return ImGui::GetCurrentContext() != nullptr;
}

void EditorGUI::DockingWindow()
{
	// Create a dock space window inside the main viewport (i.e. the entire window)
	ImGuiViewport* Viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(Viewport->WorkPos);
	ImGui::SetNextWindowSize(Viewport->WorkSize);
	ImGui::SetNextWindowViewport(Viewport->ID);

	// 2. Create a main dock space in your main render loop
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoBringToFrontOnFocus;

	// optionally disable background if you want a clean area
	ImGui::SetNextWindowBgAlpha(0.0f);

	// Remove docking flag from the dockspace window so it behaves as a container
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::Begin("MainDockSpace", nullptr, windowFlags);

	// Pass docking ID to create the dock space
	ImGuiID dockSpaceId = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f));

	ImGui::End();
	ImGui::PopStyleVar(2);
}

void EditorGUI::Update(GLFWwindow* windowContext)
{
    // start a new ImGui Frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

	DockingWindow();

    // Windows that imgui has to render
    TopMenuBar(windowContext);
    static bool showSceneViewer = true;
    if (showSceneViewer)
		ViewPortWindow(showSceneViewer);

    static bool show_demo_window = true;
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    static bool show_another_window = true;
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
}

void EditorGUI::Render()
{
    ImGui::Render();
    
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform Windows are windows embedded within the host window)
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

void EditorGUI::ShutDown()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
