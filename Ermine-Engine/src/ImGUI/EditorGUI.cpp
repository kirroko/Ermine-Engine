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
#include "Scene.h"
#include "HierarchyPanel.h"
#include "HierarchyInspector.h"


#include "AssetManager.h"
#include "imgui_internal.h"
#include "Physics.h"
#include "Serialisation.h"
#include <optional>
#include "SceneManager.h"
#include <imnodes.h>

namespace Ermine
{
	class InspectorGUI;
    class Physics;
}

class Ermine::InspectorGUI;
class Ermine::Physics;

using namespace Ermine::editor;

// Definition for static member m_Windows, for ImGUI Windows
std::vector<std::unique_ptr<Ermine::ImGUIWindow>>EditorGUI::m_Windows;
bool EditorGUI::isPlaying = false; // tied to Play/Stop toolbar state.

std::shared_ptr<Ermine::Scene> EditorGUI::s_ActiveScene = nullptr; 
std::unique_ptr<Ermine::HierarchyPanel> Ermine::editor::EditorGUI::s_HierarchyPanel = nullptr;
std::unique_ptr<Ermine::editor::HierarchyInspector> Ermine::editor::EditorGUI::s_Inspector = nullptr;
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

    ImTextureID gIconPlay = 0;
    ImTextureID gIconStop = 0;
    bool gIconsLoaded = false;

    void LoadToolbarIcons()
    {
        if (gIconsLoaded) return;

        auto loadTex = [](const char* path) -> ImTextureID
            {
                auto tex = Ermine::AssetManager::GetInstance().LoadTexture(path);
                if (tex && tex->IsValid())
                    return static_cast<ImTextureID>(static_cast<intptr_t>(tex->GetRendererID()));
                return 0;
            };

        gIconPlay = loadTex("../Resources/Textures/Icons/play.png");
        if (!gIconPlay) EE_CORE_WARN("Cannot find play button!");

        gIconStop = loadTex("../Resources/Textures/Icons/stop.png");
        if (!gIconStop) EE_CORE_WARN("Cannot find stop button!");

        gIconsLoaded = true;
    }

    bool DrawIconOrTextButton(ImTextureID icon, const char* text, const ImVec2& size)
    {
        if (icon)
            return ImGui::ImageButton(text, icon, size, ImVec2(0, 1), ImVec2(1, 0));
        return ImGui::Button(text, size);
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
        {
            if (auto path = SceneManager::ShowOpenDialog(GetActiveWindow()))
            {
                SceneManager::GetInstance().ClearScene();
                SceneManager::GetInstance().OpenScene(*path);
            }
        }

        if (ImGui::MenuItem("Save", "Ctrl+S"))
            SceneManager::GetInstance().SaveScene();

        if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
            if (auto path = SceneManager::ShowSaveDialog(L"untitled.scene", GetActiveWindow()))
                SceneManager::GetInstance().SaveSceneTo(*path);

        if (ImGui::MenuItem("Exit", "Alt+F4"))
            glfwSetWindowShouldClose(windowContext, GLFW_TRUE);

        ImGui::EndMenu();
    }

    ImGuiIO& io = ImGui::GetIO();

    if (!io.WantTextInput)  // don't trigger if user is typing in text fields
    {
        bool ctrl = io.KeyCtrl;
        bool shift = io.KeyShift;

        if (ImGui::IsKeyPressed(ImGuiKey_S, false))
        {
            if (ctrl && shift)
            {
                //EE_CORE_INFO("Ctrl + Shift + S = SAVE AS");
                if (auto path = SceneManager::ShowSaveDialog(L"untitled.scene", GetActiveWindow()))
                    SceneManager::GetInstance().SaveSceneTo(*path);
            }
            else if (ctrl)
            {
                //EE_CORE_INFO("Ctrl + S = SAVE");
                SceneManager::GetInstance().SaveScene();
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_O, false))
        {
            if (ctrl)
            {
                //EE_CORE_INFO("Ctrl + O = OPEN");
                if (auto path = SceneManager::ShowOpenDialog(GetActiveWindow()))
                {
                    SceneManager::GetInstance().ClearScene();
                    SceneManager::GetInstance().OpenScene(*path);
                }
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
        {
            if (ctrl)
            {
                EE_CORE_INFO("Ctrl + Z = UNDO");
                // code to undo
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
        {
            if (ctrl)
            {
                EE_CORE_INFO("Ctrl + Y = REDO");
                // code to redo
            }
        }
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

    if (ImGui::BeginMenu("Settings"))
    {
        if (ImGui::MenuItem("Light Mode"))
        {
            EE_CORE_INFO("Light clicked");
        }
        if (ImGui::MenuItem("Dark Mode"))
        {
            EE_CORE_INFO("Dark clicked");
        }
        if (ImGui::MenuItem("Pink Mode"))
        {
            EE_CORE_INFO("Pink clicked");
        }
        if (ImGui::MenuItem("Cyberpunk Mode"))
        {
            EE_CORE_INFO("Cyberpunk clicked");
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Windows"))
    {
        if (ImGui::MenuItem("Console"))
        {
            EE_CORE_INFO("Console open");
        }

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void EditorGUI::Toolbar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

    auto& colors = ImGui::GetStyle().Colors;
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors[ImGuiCol_ButtonHovered]);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors[ImGuiCol_ButtonActive]);

    ImGui::Begin("Toolbar", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);

    const float size = ImGui::GetWindowHeight() - 4.0f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float total = size * 2.0f + spacing;
    const float content_w = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
    const float start_x = (content_w - total) * 0.5f;
    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x + ImMax(0.0f, start_x));

    auto RenderToggledButton = [&](bool toggled, ImTextureID icon, const char* label)
        {
            if (toggled)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, colors[ImGuiCol_ButtonActive]);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors[ImGuiCol_ButtonActive]);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors[ImGuiCol_ButtonActive]);
                ImGui::BeginDisabled(true);
            }

            bool clicked = DrawIconOrTextButton(icon, label, ImVec2(size, size));

            if (toggled)
            {
                ImGui::EndDisabled();
                ImGui::PopStyleColor(3);
            }
            return clicked;
        };

	if (RenderToggledButton(isPlaying,gIconPlay,"Play"))
	{
		if (!isPlaying)
            isPlaying = true;
	}

    ImGui::SameLine();

    if (RenderToggledButton(!isPlaying,gIconStop,"Stop"))
    {
	    if (isPlaying)
			isPlaying = false;
    }

    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
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

void Ermine::editor::EditorGUI::FocusWindow(const std::string& windowName)
{
    // Loop through all registered windows and match by name
    for (auto& window : m_Windows)
    {
        if (window->Name() == windowName)
        {
            ImGui::SetWindowFocus(windowName.c_str());
            return;
        }
    }
}

void EditorGUI::SetActiveScene(std::shared_ptr<Ermine::Scene> scene) {
    s_ActiveScene = scene;

    // Debug logging to see what's happening
    EE_CORE_INFO("Setting active scene: {}", scene ? scene->GetName() : "null");

    // Update the hierarchy panel with the new scene
    if (s_HierarchyPanel) {
        s_HierarchyPanel->SetScene(scene.get());
        EE_CORE_INFO("Scene set to hierarchy panel");

        // Verify it was set correctly
        auto retrievedScene = s_HierarchyPanel->GetScene();
        EE_CORE_INFO("Retrieved scene from hierarchy panel: {}", retrievedScene ? "exists" : "null");
    }
    else {
        EE_CORE_WARN("s_HierarchyPanel is null!");
    }

    // Update the inspector panel with the new scene  
    if (s_Inspector) {
        s_Inspector->SetScene(scene.get());
    }
}

void SetCutesyPinkTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(0.15f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.50f, 0.55f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(1.00f, 0.93f, 0.96f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 0.92f, 0.95f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.90f, 0.70f, 0.80f, 0.60f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 0.88f, 0.93f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00f, 0.82f, 0.90f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(1.00f, 0.75f, 0.85f, 1.00f);

    colors[ImGuiCol_TitleBg] = ImVec4(1.00f, 0.85f, 0.90f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(1.00f, 0.80f, 0.88f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.85f, 0.88f, 0.60f);

    colors[ImGuiCol_MenuBarBg] = ImVec4(1.00f, 0.90f, 0.93f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(1.00f, 0.94f, 0.96f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(1.00f, 0.80f, 0.88f, 0.8f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.00f, 0.72f, 0.84f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 0.65f, 0.78f, 1.0f);

    colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.30f, 0.55f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.70f, 0.82f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.55f, 0.75f, 1.00f);

    colors[ImGuiCol_Button] = ImVec4(1.00f, 0.82f, 0.90f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 0.75f, 0.87f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 0.68f, 0.82f, 1.00f);

    colors[ImGuiCol_Header] = ImVec4(1.00f, 0.80f, 0.88f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.70f, 0.85f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(1.00f, 0.65f, 0.80f, 1.00f);

    colors[ImGuiCol_Separator] = ImVec4(0.95f, 0.70f, 0.80f, 0.6f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 0.82f, 0.90f, 0.7f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 0.75f, 0.87f, 0.9f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.68f, 0.82f, 1.0f);

    colors[ImGuiCol_Tab] = ImVec4(1.00f, 0.84f, 0.90f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(1.00f, 0.75f, 0.87f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(1.00f, 0.70f, 0.84f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(1.00f, 0.87f, 0.93f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(1.00f, 0.80f, 0.88f, 1.00f);

    colors[ImGuiCol_DockingPreview] = ImVec4(1.00f, 0.60f, 0.80f, 0.4f);
    colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.65f, 0.80f, 1.00f);

    // Style tweaks (rounded and soft)
    style.FrameRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.PopupRounding = 8.0f;
    style.WindowRounding = 10.0f;
    style.ScrollbarRounding = 10.0f;

    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(6, 6);
}

void SetCyberpunk2077Theme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // 🔤 Text
    colors[ImGuiCol_Text] = ImVec4(1.00f, 0.93f, 0.00f, 1.00f); // neon yellow
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.55f, 0.20f, 1.00f);

    // 🪟 Backgrounds
    colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.03f, 0.06f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.05f, 0.05f, 0.10f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.08f, 0.12f, 0.98f);

    // 🟦 Borders / Lines
    colors[ImGuiCol_Border] = ImVec4(0.0f, 0.9f, 1.0f, 0.60f); // neon cyan
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // 🧱 Frames (buttons, inputs, etc.)
    colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.05f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.00f, 0.85f, 1.00f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(1.00f, 0.93f, 0.00f, 0.45f);

    // 🎛️ Buttons
    colors[ImGuiCol_Button] = ImVec4(0.10f, 0.05f, 0.18f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 0.93f, 0.00f, 0.65f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.85f, 1.00f, 0.90f);

    // 🎚️ Sliders / Grab handles
    colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.93f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.85f, 1.00f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.93f, 0.00f, 1.00f);

    // 📁 Tabs / Headers
    colors[ImGuiCol_Header] = ImVec4(0.00f, 0.85f, 1.00f, 0.45f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.93f, 0.00f, 0.55f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.85f, 1.00f, 0.70f);

    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.00f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.00f, 0.35f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.02f, 0.18f, 1.00f);

    // 🌀 Tabs
    colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.05f, 0.25f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(1.00f, 0.93f, 0.00f, 0.75f);
    colors[ImGuiCol_TabActive] = ImVec4(0.00f, 0.85f, 1.00f, 1.00f);

    // ✨ Accent
    colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.93f, 0.00f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.00f, 0.85f, 1.00f, 0.8f);
    colors[ImGuiCol_DockingPreview] = ImVec4(1.00f, 0.93f, 0.00f, 0.4f);

    // ⚙️ Rounding & padding for that sleek neon look
    style.FrameRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.WindowRounding = 6.0f;
    style.ScrollbarRounding = 9.0f;
    style.TabRounding = 4.0f;
    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(6, 6);
}

// Call: SetOverwatchTheme(true);  // true = dark, false = light
// --------------------------------------
void SetOverwatchTheme(bool dark_variant = true)
{
    auto OW_Orange = [](float a = 1.0f) { return ImVec4(0.98f, 0.62f, 0.10f, a); };
    auto OW_Blue = [](float a = 1.0f) { return ImVec4(0.10f, 0.70f, 1.00f, a); };

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    if (dark_variant)
    {
        // DARK MODE
        colors[ImGuiCol_Text] = ImVec4(0.96f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.58f, 0.63f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.08f, 0.10f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.11f, 0.14f, 0.98f);
        colors[ImGuiCol_Border] = ImVec4(0.23f, 0.25f, 0.30f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

        colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = OW_Blue(0.35f);
        colors[ImGuiCol_FrameBgActive] = OW_Blue(0.60f);

        colors[ImGuiCol_Button] = ImVec4(0.15f, 0.16f, 0.19f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = OW_Orange(0.85f);
        colors[ImGuiCol_ButtonActive] = OW_Orange(1.00f);

        colors[ImGuiCol_Header] = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = OW_Orange(0.55f);
        colors[ImGuiCol_HeaderActive] = OW_Orange(0.80f);

        colors[ImGuiCol_CheckMark] = OW_Orange(1.00f);
        colors[ImGuiCol_SliderGrab] = OW_Blue(0.80f);
        colors[ImGuiCol_SliderGrabActive] = OW_Blue(1.00f);

        colors[ImGuiCol_Tab] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_TabHovered] = OW_Orange(0.95f);
        colors[ImGuiCol_TabActive] = ImVec4(0.18f, 0.19f, 0.22f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);

        colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.13f, 0.16f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);

        colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.30f, 0.34f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = OW_Blue(0.55f);
        colors[ImGuiCol_ResizeGripHovered] = OW_Blue(0.80f);
        colors[ImGuiCol_ResizeGripActive] = OW_Blue(1.00f);

        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.07f, 0.08f, 0.10f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.26f, 0.30f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = OW_Orange(0.75f);
        colors[ImGuiCol_ScrollbarGrabActive] = OW_Orange(1.00f);

        colors[ImGuiCol_DockingPreview] = OW_Orange(0.40f);
        colors[ImGuiCol_NavHighlight] = OW_Blue(1.00f);
    }
    else
    {
        // LIGHT MODE
        colors[ImGuiCol_Text] = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.62f, 0.66f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.97f, 0.98f, 0.99f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.98f, 0.99f, 1.00f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
        colors[ImGuiCol_Border] = ImVec4(0.85f, 0.87f, 0.90f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

        colors[ImGuiCol_FrameBg] = ImVec4(0.94f, 0.95f, 0.97f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = OW_Blue(0.25f);
        colors[ImGuiCol_FrameBgActive] = OW_Blue(0.45f);
        colors[ImGuiCol_Button] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = OW_Orange(0.85f);
        colors[ImGuiCol_ButtonActive] = OW_Orange(1.00f);

        colors[ImGuiCol_Header] = ImVec4(0.94f, 0.95f, 0.97f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = OW_Orange(0.45f);
        colors[ImGuiCol_HeaderActive] = OW_Orange(0.70f);

        colors[ImGuiCol_CheckMark] = OW_Orange(1.00f);
        colors[ImGuiCol_SliderGrab] = OW_Blue(0.80f);
        colors[ImGuiCol_SliderGrabActive] = OW_Blue(1.00f);

        colors[ImGuiCol_Tab] = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);
        colors[ImGuiCol_TabHovered] = OW_Orange(0.90f);
        colors[ImGuiCol_TabActive] = ImVec4(0.88f, 0.91f, 0.95f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);

        colors[ImGuiCol_TitleBg] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.96f, 0.97f, 0.99f, 1.00f);

        colors[ImGuiCol_Separator] = ImVec4(0.82f, 0.84f, 0.88f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = OW_Blue(0.45f);
        colors[ImGuiCol_ResizeGripHovered] = OW_Blue(0.75f);
        colors[ImGuiCol_ResizeGripActive] = OW_Blue(1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.96f, 0.97f, 0.99f, 0.70f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.86f, 0.88f, 0.92f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = OW_Orange(0.70f);
        colors[ImGuiCol_ScrollbarGrabActive] = OW_Orange(0.95f);
        colors[ImGuiCol_DockingPreview] = OW_Orange(0.35f);
        colors[ImGuiCol_NavHighlight] = OW_Blue(1.00f);
    }

    // 🎨 Shared Overwatch style DNA
    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 5.0f;
    style.ScrollbarRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(10, 6);
    style.ItemSpacing = ImVec2(8, 8);
    style.ItemInnerSpacing = ImVec2(6, 6);
    style.IndentSpacing = 18.0f;
    style.GrabMinSize = 14.0f;
    style.ScrollbarSize = 14.0f;
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
    /*ImGui::StyleColorsLight();
    SetCutesyPinkTheme();*/
    ImGui::StyleColorsDark();
    SetCyberpunk2077Theme();
    //SetOverwatchTheme(false);

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

    ImNodes::CreateContext();
    ImNodes::StyleColorsDark();

    // Create Scene first
    //s_ActiveScene = std::make_unique<Scene>("Default Scene"); // Give it a name

    // Then create HierarchyPanel with the scene
    s_HierarchyPanel = std::make_unique<HierarchyPanel>();
    s_HierarchyPanel->SetScene(s_ActiveScene.get());

    // Create Inspector Panel
    s_Inspector = std::make_unique<HierarchyInspector>();
    s_Inspector->SetScene(s_ActiveScene.get());
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


    // Hierarchy Panel
    static bool show_hierarchy = true;
    if (s_HierarchyPanel && show_hierarchy) {
        s_HierarchyPanel->SetVisible(show_hierarchy);
        s_HierarchyPanel->OnImGuiRender();
    }

    // Inspector Panel
    static bool show_inspector = true;
    if (s_Inspector && show_inspector) {
        s_Inspector->SetVisible(show_inspector);
        s_Inspector->OnImGuiRender();
    }

    static bool show_demo_window = true;
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // Call Update() for all registered ImGui windows
    for (auto& window : m_Windows) {
        window->Update();
    }
}

void EditorGUI::Render()
{
    //static bool show_profiler = true;
    //if (show_profiler)
    //    ProfilingWindow();

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
    // Clean up panels first
    s_Inspector.reset();
    s_HierarchyPanel.reset();
    s_ActiveScene.reset();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImNodes::DestroyContext();
    ImGui::DestroyContext();
    m_Windows.clear(); // Clean up additional ImGUI windows
}
