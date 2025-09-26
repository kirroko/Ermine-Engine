/* Start Header ************************************************************************/
/*!
\file       EditorGUI.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu (98%)
\co-authors LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu (2%)
\date       27/03/2025
\brief      This file contains the declaration of the EditorGUI class.
            Function just like a wrapper for the ImGUI library.
            Each window for teh editor should be encapsulated into a function in this class.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "EditorGUI.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "ECS.h"
#include "EditorCamera.h"
#include "FrameController.h"
#include "Input.h"
#include "InspectorGUI.h"
#include "Renderer.h"

#include <ImGuizmo.h>
#include "Serialisation.h"
#include <optional>
#include "SceneManager.h"

namespace Ermine
{
	class InspectorGUI;
}

class Ermine::InspectorGUI;

using namespace Ermine::editor;

// Definition for static member m_Windows, for ImGUI Windows
std::vector<std::unique_ptr<Ermine::ImGUIWindow>>EditorGUI::m_Windows;
bool Ermine::editor::EditorGUI::isPlaying = false; // TODO: tied to Play/Stop toolbar state.

namespace
{
    std::string FormatNumber(uint64_t value)
    {
        struct Unit { uint64_t base; const char* suffix; };
        static constexpr Unit units[] = {
			{.base= 1'000'000'000'000ULL, .suffix= "T"},
	        {.base= 1'000'000'000ULL, .suffix= "B"},
	        {.base= 1'000'000ULL, .suffix= "M"},
	        {.base= 1'000ULL, .suffix= "K"},
	        {.base= 1, .suffix= ""}
        };

        for (const auto& u : units)
        {
            if (value >= u.base)
            {
                char buffer[32];
                const double scaled = static_cast<double>(value) / static_cast<double>(u.base);
                const int written = snprintf(buffer, sizeof(buffer), "%.1f%s", scaled, u.suffix);
                if (written < 0)
                {
                    EE_CORE_WARN("FormatNumber error occurred");
                    return std::to_string(value);
                }
                return std::string(buffer);
            }
        }

        char buffer[32];
		const int written = snprintf(buffer, sizeof(buffer), "%llu", value);
        if (written < 0)
        {
            EE_CORE_WARN("FormatNumber error occurred");
            return std::to_string(value);
        }
		return std::string(buffer);
	}
}

void EditorGUI::TopMenuBar(GLFWwindow* windowContext)
{
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New"))
            SceneManager::GetInstance().NewScene();

        if (ImGui::MenuItem("Open...", "Ctrl+O"))
            if (auto path = SceneManager::ShowOpenDialog(GetActiveWindow()))
                SceneManager::GetInstance().OpenScene(*path);

        if (ImGui::MenuItem("Save", "Ctrl+S"))
            SceneManager::GetInstance().SaveScene();

        if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
            if (auto path = SceneManager::ShowSaveDialog(L"untitled.scene", GetActiveWindow()))
                SceneManager::GetInstance().SaveSceneTo(*path);

        if (ImGui::MenuItem("Exit", "Alt+F4"))
            glfwSetWindowShouldClose(windowContext, GLFW_TRUE);

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

void EditorGUI::ProfilingWindow()
{
    ImGui::Begin("GPU Profiler");

    const auto& metrics = graphics::GPUProfiler::GetMetrics();

    float avgFps = metrics.averageFrameTimeMs > 0.0f ? 1000.0f / metrics.averageFrameTimeMs : 0.0f;

    //ImGui::Text("FPS: %.1f (avg: %.1f)", metrics.fps, avgFps);
    ImGui::Text("FPS: %.1f", avgFps);
    ImGui::Text("Frame Time: %.2f ms", metrics.frameTimeMs);
    ImGui::Text("CPU Time: %.2f ms", metrics.cpuFrameTimeMs);
    ImGui::Text("GPU Time: %.2f ms", metrics.gpuFrameTimeMs);

    ImGui::Separator();

    ImGui::Text("Min Frame Time: %.2f ms", metrics.minFrameTimeMs);
    ImGui::Text("Max Frame Time: %.2f ms", metrics.maxFrameTimeMs);
    ImGui::Text("Avg Frame Time: %.2f ms", metrics.averageFrameTimeMs);

    ImGui::Separator();

    ImGui::Text("Draw Calls: %u", metrics.drawCallCount);
    ImGui::Text("Tris: %s", FormatNumber(metrics.triangleCount).c_str());
    ImGui::Text("Verts: %s", FormatNumber(metrics.vertexCount).c_str());

    ImGui::Separator();

    ImGui::Text("Total VRAM: %llu MB", metrics.totalVRAMUsageMB);
    ImGui::Text("Texture Memory: %llu MB", metrics.textureMemoryMB);
    ImGui::Text("Buffer Memory: %llu MB", metrics.bufferMemoryMB);

    // Display frame time history graph
    const auto& history = graphics::GPUProfiler::GetFrameTimeHistory();
    if (!history.empty())
    {
        std::vector values(history.begin(), history.end());
        ImGui::PlotLines("Frame Times", values.data(), static_cast<int>(values.size()),
            0, nullptr, 0.0f, metrics.maxFrameTimeMs * 1.2f, ImVec2(0, 80));
    }
    ImGui::End();
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
	auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
    if (first_time)
    {
        renderer->CreateOffscreenBuffer(static_cast<int>(viewport_size.x), static_cast<int>(viewport_size.y));
        renderer->ResizeGBuffer(static_cast<int>(viewport_size.x), static_cast<int>(viewport_size.y));
        first_time = false;
    }

	const auto offscreen_buffer = renderer->GetOffscreenBuffer(); // released at the end of the scope
    if (offscreen_buffer)
    {
        // Resize the offscreen buffer when viewport size changes
	    if (offscreen_buffer->width != static_cast<int>(viewport_size.x) ||
            offscreen_buffer->height != static_cast<int>(viewport_size.y))
	    {
		    renderer->ResizeOffscreenBuffer(static_cast<int>(viewport_size.x), static_cast<int>(viewport_size.y));
			renderer->ResizeGBuffer(static_cast<int>(viewport_size.x), static_cast<int>(viewport_size.y));
	    }
    }

    // Set editor's camera viewport size
	EditorCamera::GetInstance().SetViewportSize(viewport_size.x, viewport_size.y);

    // Child region that ignores all ImGui inputs
    ImGuiWindowFlags vpChildFlags =
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::BeginChild("SceneViewportRegion", ImVec2(0,0), false, vpChildFlags);

    // Draw the rendered scene
    if (offscreen_buffer)
    {
        ImGui::Image(
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W) || defined(IMGUI_IMPL_OPENGL_ES2) || defined(IMGUI_IMPL_OPENGL_ES3) || defined(IMGUI_IMPL_OPENGL_LOADER_GLEW) || defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
            (ImTextureID)(intptr_t)offscreen_buffer->ColorTexture,
#else
            offscreen_buffer->ColorTexture,
#endif
            ImGui::GetContentRegionAvail(),
            ImVec2(0, 1), ImVec2(1, 0)
        );
    }

	// Capture the image rect for mouse->pixel conversion
    const ImVec2 imgMin = ImGui::GetItemRectMin();
    const ImVec2 imgMax = ImGui::GetItemRectMax();
    const ImVec2 imgSize = ImGui::GetItemRectSize();

    // Left-click within the image, perform picking
    if (!isPlaying && ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        ImGuiIO io = ImGui::GetIO();
        const float localX = io.MousePos.x - imgMin.x;
		const float localY = io.MousePos.y - imgMin.y;

        if (localX >= 0.0f && localY >= 0.0f && localX <= imgSize.x && localY <= imgSize.y)
        {
            // Convert to framebuffer coordinates (y is flipped)
			const float u = imgSize.x > 0.0f ? localX / imgSize.x : 0.0f;
			const float v = imgSize.y > 0.0f ? localY / imgSize.y : 0.0f;

			const int px = static_cast<int>(u * offscreen_buffer->width);
            const int py = static_cast<int>((1.0f - v) * offscreen_buffer->height);
            auto [hit, entity] = renderer->PickEntityAt(std::clamp(px,0,offscreen_buffer->width - 1),
                std::clamp(py,0,offscreen_buffer->height - 1),
                EditorCamera::GetInstance().GetViewMatrix(),
                EditorCamera::GetInstance().GetProjectionMatrix());

            if (hit)
            {
				// Set selection in Inspector?
                for (auto& w : m_Windows)
                {
                    if (auto* inspector = dynamic_cast<InspectorGUI*>(w.get()))
                        inspector->SetEntity(entity);
                }
			}
        }
    }

    const ImGuiHoveredFlags hovFlags =
        ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
        ImGuiHoveredFlags_AllowWhenOverlappedByWindow |
        ImGuiHoveredFlags_AllowWhenOverlappedByItem;

    const bool viewportHovered = ImGui::IsItemHovered(hovFlags);
	const bool viewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_None);

    ImGui::EndChild();

    Input::SetEditorInputActive(viewportFocused && viewportHovered);
    if (Input::IsKeyDownEditor(GLFW_KEY_LEFT_CONTROL) && Input::IsKeyPressedEditor(GLFW_KEY_P))
    {
		isPlaying = !isPlaying;
        EE_CORE_INFO("Play {0}", isPlaying);
    }
    Input::SetGameInputActive(isPlaying && viewportFocused && viewportHovered);

    if (viewportHovered && !isPlaying)
    {
	    EditorCamera::GetInstance().ProcessMouseMovement();
		EditorCamera::GetInstance().ProcessKeyboardInput(FrameController::GetDeltaTime());
		EditorCamera::GetInstance().ProcessScrollWheel(Input::GetMouseScrollOffsetEditor());
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

    // --- DPI scaling for UI ---
    float xScale, yScale;
    glfwGetWindowContentScale(window, &xScale, &yScale);
	io.FontGlobalScale = xScale; // Apply the xScale to the global font scale
    //ImGui::GetStyle().ScaleAllSizes(xScale);

    // Setup Dear ImGui style
    //ImGui::StyleColorsLight();
    ImGui::StyleColorsDark();

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
  //  static bool show_scene_viewer = true;
  //  if (show_scene_viewer)
		//ViewPortWindow(show_scene_viewer);

    static bool show_demo_window = true;
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    //static bool show_another_window = true;
    //if (show_another_window)
    //{
    //    ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    //    ImGui::Text("Hello from another window!");
    //    if (ImGui::Button("Close Me"))
    //        show_another_window = false;
    //    ImGui::End();
    //}

    // Call Update() for all registered ImGui windows
    for (auto& window : m_Windows) {
        window->Update();
    }
}

void EditorGUI::Render()
{
    static bool show_profiler = true;
    if (show_profiler)
        ProfilingWindow();

    // Render additional ImGUI windows
    for (auto& window : m_Windows) {
        window->Render();
    }

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
    m_Windows.clear(); // Clean up additional ImGUI windows
}
