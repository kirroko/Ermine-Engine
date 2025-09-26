/* Start Header ************************************************************************/
/*!
\file       ViewPortGUI.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       21/09/2025
\brief      This file contains the responsibility for rendering the viewport window

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "InspectorGUI.h"
#include "ViewPortGUI.h"

#include "ECS.h"
#include "FrameController.h"
#include "Input.h"
#include "Renderer.h"
#include "GLFW/glfw3.h"
#include "EditorGUI.h"

#include <ImGuizmo.h>
#include <glm/gtx/matrix_decompose.hpp>

#include "AssetManager.h"

using namespace Ermine::editor;

EditorGUI::SimState EditorGUI::s_state = SimState::stopped;

namespace
{
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

Ermine::ViewPortGUI::ViewPortGUI() : ImGUIWindow("Viewport"), show(true), ref_Inspector(nullptr)
{
}

Ermine::ViewPortGUI::ViewPortGUI(InspectorGUI* ref) : ImGUIWindow("Viewport"), show(true), ref_Inspector(ref)
{
}

void Ermine::ViewPortGUI::Render()
{
}

void Ermine::ViewPortGUI::Update()
{
	ImGui::Begin("Scene Viewer", &show);

	LoadToolbarIcons();

	const ImVec2 iconSize = ImVec2(28.f, 28.f);
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const int buttonCount = 3;
	const float totalWidth = buttonCount * iconSize.x + (buttonCount - 1) * spacing;

	// Center horizontally
	float availWidth = ImGui::GetContentRegionAvail().x;
	float startOffsetX = (availWidth > totalWidth) ? (availWidth - totalWidth) * 0.5f : 0.0f;
	float oldX = ImGui::GetCursorPosX();
	ImGui::SetCursorPosX(oldX + startOffsetX);

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, 6.f));
	ImGui::BeginGroup();
	{
		const bool playing = (EditorGUI::s_state == EditorGUI::SimState::playing);
		const bool paused = (EditorGUI::s_state == EditorGUI::SimState::paused);
		const bool stopped = (EditorGUI::s_state == EditorGUI::SimState::stopped);

		// Play
		ImGui::BeginDisabled(playing);
		if (DrawIconOrTextButton(gIconPlay, "Play", iconSize))
		{
			EditorGUI::s_state = EditorGUI::SimState::playing;
			EE_CORE_INFO("Simulation: Play");
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Play (Ctrl+P)");
		ImGui::EndDisabled();

		ImGui::SameLine();

		// Pause/Resume
		//ImGui::BeginDisabled(stopped);
		//const char* pauseText = paused ? "Resume" : "Pause";
		//ImTextureID pauseIcon = paused ? gIconPlay : gIconPause; // show Play icon for Resume
		//if (DrawIconOrTextButton(pauseIcon, pauseText, iconSize))
		//{
		//	EditorGUI::s_state = paused ? EditorGUI::SimState::playing : EditorGUI::SimState::paused;
		//	EE_CORE_INFO("Simulation: {0}", paused ? "Resume" : "Pause");
		//}
		//if (ImGui::IsItemHovered())
		//	ImGui::SetTooltip("Pause/Resume (Ctrl+P)");
		//ImGui::EndDisabled();

		//ImGui::SameLine();

		// Stop
		ImGui::BeginDisabled(stopped);
		if (DrawIconOrTextButton(gIconStop, "Stop", iconSize))
		{
			EditorGUI::s_state = EditorGUI::SimState::stopped;
			EE_CORE_INFO("Simulation: Stop");
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Stop (Ctrl+Shift+P)");
		ImGui::EndDisabled();
	}
	ImGui::EndGroup();
	ImGui::PopStyleVar();

	ImGui::Separator();

	EditorGUI::isPlaying = EditorGUI::s_state == EditorGUI::SimState::playing;

	// Begin ImGuizmo frame
	ImGuizmo::BeginFrame();
	ImGuizmo::Enable(true);

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

	ImGui::BeginChild("SceneViewportRegion", ImVec2(0, 0), false, vpChildFlags);

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

	// View cube (top-right corner)
	const float pad = 10.f;
	const ImVec2 vmSize = ImVec2(100.f, 100.f);
	const ImVec2 vmPos = ImVec2(imgMax.x - vmSize.x - pad, imgMin.y + pad);
	const ImVec2 vmPosBR = ImVec2(vmPos.x + vmSize.x, vmPos.y + vmSize.y);

	const bool overViewCube = ImGui::IsMouseHoveringRect(vmPos, vmPosBR, false);

	EntityID selectedEntity{};
	selectedEntity = ref_Inspector->GetEntity();

	// Keyboard shortcuts for gizmo
	static ImGuizmo::OPERATION gOperation = ImGuizmo::TRANSLATE;
	static ImGuizmo::MODE gMode = ImGuizmo::LOCAL;

	const ImGuiHoveredFlags hovFlags =
		ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
		ImGuiHoveredFlags_AllowWhenOverlappedByWindow |
		ImGuiHoveredFlags_AllowWhenOverlappedByItem;

	const bool viewportHovered = ImGui::IsItemHovered(hovFlags);
	const bool viewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_None);

	// Set manipulation mode based on keyboard shortcuts
	if (viewportFocused && viewportHovered && !EditorGUI::isPlaying && !Input::IsMouseButtonDownEditor(GLFW_MOUSE_BUTTON_RIGHT))
	{
		if (Input::IsKeyPressedEditor(GLFW_KEY_W)) gOperation = ImGuizmo::TRANSLATE;
		if (Input::IsKeyPressedEditor(GLFW_KEY_E)) gOperation = ImGuizmo::ROTATE;
		if (Input::IsKeyPressedEditor(GLFW_KEY_R)) gOperation = ImGuizmo::SCALE;
		if (Input::IsKeyPressedEditor(GLFW_KEY_Q)) gMode = gMode == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
	}

	// Overlay current gizmo operation/mode
	{
		auto OpToString = [](ImGuizmo::OPERATION op) -> const char*
			{
				switch (op)
				{
				case ImGuizmo::TRANSLATE: return "Translate";
				case ImGuizmo::ROTATE: return "Rotate";
				case ImGuizmo::SCALE: return "Scale";
				default: return "Unknown";
				}
			};

		auto ModeToString = [](ImGuizmo::MODE m) -> const char*
			{
				return (m == ImGuizmo::LOCAL) ? "Local" : "World";
			};

		const char* opText = OpToString(gOperation);
		const char* modeText = ModeToString(gMode);

		char label[128];
		(void)snprintf(label, sizeof(label), "Op: %s | Mode: %s", opText, modeText);

		ImDrawList* dl = ImGui::GetForegroundDrawList();
		const ImVec2 padPx(6.f, 4.f);
		const ImVec2 textSize = ImGui::CalcTextSize(label);
		const ImVec2 boxPos = ImVec2(imgMin.x + 8.f, imgMin.y + 8.f);
		const ImVec2 boxMax = ImVec2(boxPos.x + textSize.x + padPx.x * 2.f,
			boxPos.y + textSize.y * 2.f + padPx.y * 2.f);

		dl->AddRectFilled(boxPos, boxMax, IM_COL32(0, 0, 0, 160), 4.0f);
		dl->AddText(ImVec2(boxPos.x + padPx.x, boxPos.y + padPx.y), IM_COL32(255, 255, 255, 255), label);
	}

	// Focus camera on selected entity (F key)
	if (viewportFocused && viewportHovered && !EditorGUI::isPlaying && Input::IsKeyPressedEditor(GLFW_KEY_F))
	{
		if (ECS::GetInstance().IsEntityValid(selectedEntity) && ECS::GetInstance().HasComponent<Transform>(selectedEntity))
		{
			auto& ecs = ECS::GetInstance();
			auto& tr = ecs.GetComponent<Transform>(selectedEntity);
			EditorCamera::GetInstance().Focus(tr.position, 2.5f); // TODO: Lerp for smoother transition
		}
	}

	// Orbit around pivot
	static bool s_orbiting = false;
	static ImVec2 s_lastMouse = ImVec2(0, 0);
	static Vector3D s_pivot = Vector3D(0.0f, 0.0f, 0.0f);
	static float s_distance = 5.0f;

	const bool altDown = Input::IsKeyDownEditor(GLFW_KEY_LEFT_ALT);
	const bool rightMouseDown = Input::IsMouseButtonDownEditor(GLFW_MOUSE_BUTTON_RIGHT);

	// Orbit controls
	if (viewportHovered && !EditorGUI::isPlaying && altDown && rightMouseDown && !overViewCube)
	{
		ImGuiIO& io = ImGui::GetIO();
		if (!s_orbiting)
		{
			// Initialize orbit
			if (ECS::GetInstance().IsEntityValid(selectedEntity) && ECS::GetInstance().HasComponent<Transform>(selectedEntity))
			{
				auto& ecs = ECS::GetInstance();
				auto& tr = ecs.GetComponent<Transform>(selectedEntity);
				s_pivot = tr.position;
			}
			else
			{
				//s_pivot = EditorCamera::GetInstance().GetPosition() + EditorCamera::GetInstance().() * s_distance;
			}

			const Vector3D camPos = EditorCamera::GetInstance().GetPosition();
			Vector3D diff = camPos - s_pivot;
			s_distance = Vec3Length(diff);
			if (s_distance < 0.001f) s_distance = 5.0f;

			s_lastMouse = io.MousePos;
			s_orbiting = true;
		}
		else
		{
			const float dx = io.MousePos.x - s_lastMouse.x;
			const float dy = io.MousePos.y - s_lastMouse.y;
			s_lastMouse = io.MousePos;

			EditorCamera::GetInstance().OrbitAround(s_pivot, dx, dy, s_distance);
		}
	}
	else
	{
		s_orbiting = false;
	}

	// Camera controls
	if (viewportHovered && !EditorGUI::isPlaying && !s_orbiting)
	{
		if (!ImGuizmo::IsUsing())
		{
			EditorCamera::GetInstance().ProcessMouseMovement();
			EditorCamera::GetInstance().ProcessKeyboardInput(FrameController::GetDeltaTime());
			EditorCamera::GetInstance().ProcessScrollWheel(Input::GetMouseScrollOffsetEditor());
		}
	}

	// Left-click within the image, perform picking
	if (!EditorGUI::isPlaying && ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		if (!s_orbiting && !overViewCube && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
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
				auto [hit, entity] = renderer->PickEntityAt(std::clamp(px, 0, offscreen_buffer->width - 1),
					std::clamp(py, 0, offscreen_buffer->height - 1),
					EditorCamera::GetInstance().GetViewMatrix(),
					EditorCamera::GetInstance().GetProjectionMatrix());

				if (hit && ref_Inspector)
					ref_Inspector->SetEntity(entity);
			}
		}
	}

	const Mtx44& v = EditorCamera::GetInstance().GetViewMatrix();
	const Mtx44& p = EditorCamera::GetInstance().GetProjectionMatrix();

	glm::mat4 view = glm::mat4(
		v.m00, v.m01, v.m02, v.m03,
		v.m10, v.m11, v.m12, v.m13,
		v.m20, v.m21, v.m22, v.m23,
		v.m30, v.m31, v.m32, v.m33
	);
	glm::mat4 proj = glm::mat4(
		p.m00, p.m01, p.m02, p.m03,
		p.m10, p.m11, p.m12, p.m13,
		p.m20, p.m21, p.m22, p.m23,
		p.m30, p.m31, p.m32, p.m33
	);

	// ImGuizmo setup for this image rect
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
	ImGuizmo::SetRect(imgMin.x, imgMin.y, imgSize.x, imgSize.y);

	// OBJECT Gizmo overlay
	if (!EditorGUI::isPlaying && ECS::GetInstance().IsEntityValid(selectedEntity) && ECS::GetInstance().HasComponent<Transform>(selectedEntity))
	{
		auto& ecs = ECS::GetInstance();
		auto& tr = ecs.GetComponent<Transform>(selectedEntity);

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(tr.position.x, tr.position.y, tr.position.z));
		glm::quat rotQuat(tr.rotation.w, tr.rotation.x, tr.rotation.y, tr.rotation.z);
		rotQuat = glm::normalize(rotQuat);
		model *= glm::mat4_cast(rotQuat);
		model = glm::scale(model, glm::vec3(tr.scale.x, tr.scale.y, tr.scale.z));

		// Snapping
		const bool useSnap = Input::IsKeyDownEditor(GLFW_KEY_LEFT_CONTROL) || Input::IsKeyDownEditor(GLFW_KEY_RIGHT_CONTROL);
		float snap[3] = { 0.5f, 0.5f, 0.5f };
		if (gOperation == ImGuizmo::ROTATE) { snap[0] = snap[1] = snap[2] = 5.0f; }
		if (gOperation == ImGuizmo::SCALE) { snap[0] = snap[1] = snap[2] = 0.1f; }

		// Manipulate
		ImGuizmo::Manipulate(
			glm::value_ptr(view),
			glm::value_ptr(proj),
			gOperation,
			gMode,
			glm::value_ptr(model),
			nullptr,
			useSnap ? snap : nullptr
		);

		// Apply result back into Transform
		if (ImGuizmo::IsUsing())
		{
			glm::vec3 skew, translation, scale;
			glm::vec4 perspective;
			glm::quat rotation;
			if (glm::decompose(model, scale, rotation, translation, skew, perspective))
			{
				rotation = glm::normalize(rotation);
				tr.position = Vector3D(translation.x, translation.y, translation.z);
				tr.scale = Vector3D(scale.x, scale.y, scale.z);
				tr.rotation = Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);
			}
		}
	}

	// VIEW gizmo (camera nav cube)
	{
		glm::mat4 viewBefore = view;
		glm::mat4 viewEdit = view;

		const Vector3D camPos = EditorCamera::GetInstance().GetPosition();
		float viewLength = Vec3Length(camPos);
		if (viewLength < 0.001f) viewLength = 5.0f;

		ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

		ImGuizmo::ViewManipulate(
			glm::value_ptr(viewEdit),
			viewLength,
			vmPos,
			vmSize,
			0x10101010
		);

		auto matChanged = [](const glm::mat4& a, const glm::mat4& b) {
			const float eps = 1e-5f;
			for (int c = 0; c < 4; ++c)
				for (int r = 0; r < 4; ++r)
					if (!glm::epsilonEqual(a[c][r], b[c][r], eps))
						return true;
			return false;
			};

		if (matChanged(viewBefore, viewEdit))
		{
			glm::mat4 inv = glm::inverse(viewEdit);
			glm::vec3 pos = glm::vec3(inv[3]);
			glm::vec3 fwd = glm::normalize(-glm::vec3(inv[2])); // -Z axis

			auto rad2deg = [](float r) { return r * 57.29577951308232f; };
			float yaw = rad2deg(std::atan2(fwd.z, fwd.x));
			float pitch = rad2deg(std::asin(std::clamp(fwd.y, -1.0f, 1.0f)));

			EditorCamera::GetInstance().SetPosition(Vector3D(pos.x, pos.y, pos.z));
			EditorCamera::GetInstance().SetYawPitch(yaw, pitch);
		}
	}

	ImGui::EndChild();

	Input::SetEditorInputActive(viewportFocused && viewportHovered);

	// Hotkeys
	if (Input::IsKeyDownEditor(GLFW_KEY_LEFT_CONTROL) && Input::IsKeyPressedEditor(GLFW_KEY_P))
		EditorGUI::s_state = EditorGUI::SimState::playing;
	else if (Input::IsKeyDownEditor(GLFW_KEY_LEFT_CONTROL) && Input::IsKeyDownEditor(GLFW_KEY_LEFT_SHIFT)
		&& Input::IsKeyPressedEditor(GLFW_KEY_P))
		EditorGUI::s_state = EditorGUI::SimState::stopped;

	EditorGUI::isPlaying = EditorGUI::s_state == EditorGUI::SimState::playing;

	Input::SetGameInputActive(EditorGUI::isPlaying && viewportFocused && viewportHovered);

	ImGui::End();
}