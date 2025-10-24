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
#include "ViewPortGUI.h"

#include "ECS.h"
#include "FrameController.h"
#include "Input.h"
#include "Renderer.h"
#include "GLFW/glfw3.h"
#include "EditorGUI.h"
#include "HierarchySystem.h"

#include <ImGuizmo.h>
#include <glm/gtx/matrix_decompose.hpp>

#include "AssetManager.h"

#include "Scene.h"
#include "SceneManager.h"
#include "PrefabManager.h"
#include "Physics.h"

#include "TransformMode.h"
#include "Matrix4x4.h"

using namespace Ermine::editor;

// Initialize static variables
glm::mat4 Ermine::ViewPortGUI::s_previousModel = glm::mat4(1.0f);
bool Ermine::ViewPortGUI::s_wasManipulating = false;
TransformMode Ermine::ViewPortGUI::s_activeTransformMode = TransformMode::Pivot;
Ermine::Vec3 Ermine::ViewPortGUI::s_manipulationPoint(0.0f, 0.0f, 0.0f);
Ermine::Vec3 Ermine::ViewPortGUI::s_originalPosition(0.0f, 0.0f, 0.0f);
float Ermine::ViewPortGUI::s_modeMessageTimer = 0.0f;
const char* Ermine::ViewPortGUI::s_modeMessage = nullptr;

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
    
    // Helper function to rotate a vector by a quaternion
    Ermine::Vec3 QuaternionRotateVector(const Ermine::Quaternion& q, const Ermine::Vec3& v)
    {
        // Quaternion rotation formula: q * v * q^-1 where v is treated as a quaternion with w=0
        // We'll use a more efficient implementation
        
        // Extract components
        float qx = q.x, qy = q.y, qz = q.z, qw = q.w;
        float vx = v.x, vy = v.y, vz = v.z;
        
        // Calculate qvq^-1
        float tx = 2.0f * (qy * vz - qz * vy);
        float ty = 2.0f * (qz * vx - qx * vz);
        float tz = 2.0f * (qx * vy - qy * vx);
        
        return Ermine::Vec3(
            vx + qw * tx + qy * tz - qz * ty,
            vy + qw * ty + qz * tx - qx * tz,
            vz + qw * tz + qx * ty - qy * tx
        );
    }
}

Ermine::ViewPortGUI::ViewPortGUI() : ImGUIWindow("Viewport"), show(true)
{
}

void Ermine::ViewPortGUI::Render()
{
}

void Ermine::ViewPortGUI::TopBarSimulationControl(const ImVec2 iconSize)
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, 6.f));
	ImGui::BeginGroup();
	{
		const bool playing = (EditorGUI::s_state == EditorGUI::SimState::playing);
		//const bool paused = (EditorGUI::s_state == EditorGUI::SimState::paused);
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

		// Add Transform Mode Dropdown
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f); // Add some spacing
		
		TransformMode currentMode = TransformModeManager::GetMode();
		const char* currentModeStr = (currentMode == TransformMode::Pivot) ? "Pivot" : "Center";

		ImGui::SetNextItemWidth(90.0f);
		if (ImGui::BeginCombo("##TransformMode", currentModeStr, ImGuiComboFlags_NoArrowButton))
		{
			// Pivot option
			bool isPivot = (currentMode == TransformMode::Pivot);
			if (ImGui::Selectable("Pivot", isPivot))
			{
				TransformModeManager::SetMode(TransformMode::Pivot);
				s_activeTransformMode = TransformMode::Pivot;
				EntityID selectedEntity = SceneManager::GetInstance().GetActiveScene()->GetSelectedEntity();
				if (ECS::GetInstance().IsEntityValid(selectedEntity)) {
					UpdateManipulationPoint(selectedEntity);
				}
				EE_CORE_INFO("Transform mode: Pivot");
			}
			if (isPivot)
				ImGui::SetItemDefaultFocus();

			// Center option
			bool isCenter = (currentMode == TransformMode::Center);
			if (ImGui::Selectable("Center", isCenter))
			{
				TransformModeManager::SetMode(TransformMode::Center);
				s_activeTransformMode = TransformMode::Center;
				EntityID selectedEntity = SceneManager::GetInstance().GetActiveScene()->GetSelectedEntity();
				if (ECS::GetInstance().IsEntityValid(selectedEntity)) {
					UpdateManipulationPoint(selectedEntity);
				}
				EE_CORE_INFO("Transform mode: Center");
			}
			if (isCenter)
				ImGui::SetItemDefaultFocus();

			ImGui::EndCombo();
		}

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Toggle Pivot/Center Mode (Z)");
	}
	ImGui::EndGroup();

	float rightOffset = ImGui::GetContentRegionAvail().x - 150.0f;
	ImGui::SameLine(ImGui::GetCursorPosX() + rightOffset);

	ImGui::Checkbox("Physics Wireframe", &ECS::GetInstance().GetSystem<Physics>()->wireframe);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Requires Physics Component");

	ImGui::PopStyleVar();
}

void Ermine::ViewPortGUI::FrameBufferHandler(const ImVec2& viewport_size)
{
	static bool first_time = true;
	auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
	if (first_time)
	{
		renderer->CreateOffscreenBuffer(static_cast<int>(viewport_size.x), static_cast<int>(viewport_size.y));
		renderer->ResizeGBuffer(static_cast<int>(viewport_size.x), static_cast<int>(viewport_size.y));
		first_time = false;
	}

	if (const auto offscreen_buffer = renderer->GetOffscreenBuffer())
	{
		// Resize the offscreen buffer when viewport size changes
		if (offscreen_buffer->width != static_cast<int>(viewport_size.x) ||
			offscreen_buffer->height != static_cast<int>(viewport_size.y))
		{
			renderer->ResizeOffscreenBuffer(static_cast<int>(viewport_size.x), static_cast<int>(viewport_size.y));
			renderer->ResizeGBuffer(static_cast<int>(viewport_size.x), static_cast<int>(viewport_size.y));
		}
	}
}

void Ermine::ViewPortGUI::OverlayGizmoOperation(const ImVec2& imgMin, const ImGuizmo::OPERATION& gOperation, const ImGuizmo::MODE& gMode)
{
	// Overlay current gizmo operation/mode

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
		
		auto TransformModeToString = []() -> const char*
		{
			return (TransformModeManager::GetMode() == TransformMode::Pivot) ? "Pivot" : "Center";
		};

		const char* opText = OpToString(gOperation);
		const char* modeText = ModeToString(gMode);
		const char* transformModeText = TransformModeToString();

		char label[128];
		(void)snprintf(label, sizeof(label), "Op: %s | Mode: %s | Transform: %s (Z)", opText, modeText, transformModeText);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	const ImVec2 padPx(6.f, 4.f);
	const ImVec2 textSize = ImGui::CalcTextSize(label);
	const ImVec2 boxPos = ImVec2(imgMin.x + 8.f, imgMin.y + 8.f);
	const ImVec2 boxMax = ImVec2(boxPos.x + textSize.x + padPx.x * 2.f,
		boxPos.y + textSize.y * 2.f + padPx.y * 2.f);

	dl->AddRectFilled(boxPos, boxMax, IM_COL32(0, 0, 0, 160), 4.0f);
	dl->AddText(ImVec2(boxPos.x + padPx.x, boxPos.y + padPx.y), IM_COL32(255, 255, 255, 255), label);
}

void Ermine::ViewPortGUI::FocusOnSelected(const Ermine::EntityID& selectedEntity, const bool& viewportHovered, const bool& viewportFocused)
{
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
}

void Ermine::ViewPortGUI::CameraControls(const bool& overViewCube, const Ermine::EntityID& selectedEntity, const bool& viewportHovered, bool& s_orbiting)
{
	static ImVec2 s_lastMouse = ImVec2(0, 0);
	static Vector3D s_pivot = Vector3D(0.0f, 0.0f, 0.0f);
	static float s_distance = 5.0f;

	const bool altDown = Input::IsKeyDownEditor(GLFW_KEY_LEFT_ALT);
	const bool MouseDown = Input::IsMouseButtonDownEditor(GLFW_MOUSE_BUTTON_LEFT);

	// Orbit controls
	if (viewportHovered && !EditorGUI::isPlaying && altDown && MouseDown && !overViewCube)
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
}

void Ermine::ViewPortGUI::ObjectPicking(const std::shared_ptr<Ermine::graphics::Renderer::OffscreenBuffer>& offscreen_buffer, const ImVec2& imgMin, const ImVec2
	& imgSize, const bool& overViewCube, bool s_orbiting)
{
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
				auto [hit, entity] = ECS::GetInstance().GetSystem<graphics::Renderer>()->PickEntityAt(std::clamp(px, 0, offscreen_buffer->width - 1),
					std::clamp(py, 0, offscreen_buffer->height - 1),
					EditorCamera::GetInstance().GetViewMatrix(),
					EditorCamera::GetInstance().GetProjectionMatrix());

				if (hit)
				{
					auto previousEntity = SceneManager::GetInstance().GetActiveScene()->GetSelectedEntity();
					SceneManager::GetInstance().GetActiveScene()->SetSelectedEntity(entity);
					
					// If entity changed, update manipulation point for new entity
					if (previousEntity != entity) {
						UpdateManipulationPoint(entity);
					}
				}
			}
		}
	}
}

void Ermine::ViewPortGUI::GizmoOverlay(const ImVec2& imgMin, const ImVec2& imgSize,
	const ImVec2& vmSize, const ImVec2& vmPos,
	const Ermine::EntityID& selectedEntity,
	ImGuizmo::OPERATION& gOperation,
	ImGuizmo::MODE& gMode)
{
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
	if (!EditorGUI::isPlaying && ECS::GetInstance().IsEntityValid(selectedEntity)
		&& ECS::GetInstance().HasComponent<Transform>(selectedEntity))
	{
		auto& ecs = ECS::GetInstance();
		auto& tr = ecs.GetComponent<Transform>(selectedEntity);
		auto hierarchySystem = ecs.GetSystem<HierarchySystem>();
		
		// Get entity's actual world position (pivot)
		Vec3 entityWorldPos;
		if (hierarchySystem) {
			entityWorldPos = hierarchySystem->GetWorldPosition(selectedEntity);
		} else {
			entityWorldPos = tr.position;
		}
		
		// FIXED: Get the correct manipulation position based on current mode
		Vec3 operationPos = entityWorldPos; // Default to pivot
		
		TransformMode currentMode = TransformModeManager::GetMode();
		if (currentMode == TransformMode::Center) {
			// Use center point calculation from TransformModeManager
			operationPos = TransformModeManager::GetManipulationPosition(selectedEntity);
		}
		
		// Visual indicators to help understand the difference between pivot and center
		if (Vec3Length(entityWorldPos - operationPos) > 0.01f) {
			// Project 3D points to screen space
			auto worldToScreen = [&view, &proj, &imgMin, &imgSize](const Vec3& worldPos) -> ImVec2 {
				// Create position vector with w=1
				glm::vec4 pos(worldPos.x, worldPos.y, worldPos.z, 1.0f);
				
				// Transform to clip space
				glm::vec4 clipPos = proj * view * pos;
				
				// Perspective divide
				if (std::abs(clipPos.w) > 0.0001f) {
					clipPos.x /= clipPos.w;
					clipPos.y /= clipPos.w;
				}
				
				// NDC to screen space
				ImVec2 screenPos;
				screenPos.x = imgMin.x + (clipPos.x + 1.0f) * 0.5f * imgSize.x;
				screenPos.y = imgMin.y + (1.0f - (clipPos.y + 1.0f) * 0.5f) * imgSize.y;
				
				return screenPos;
			};
			
			ImVec2 pivotScreenPos = worldToScreen(entityWorldPos);
			ImVec2 centerScreenPos = worldToScreen(operationPos);
			
			// Check if points are in front of the camera (simple check)
			glm::vec4 pivotViewPos = view * glm::vec4(entityWorldPos.x, entityWorldPos.y, entityWorldPos.z, 1.0f);
			glm::vec4 centerViewPos = view * glm::vec4(operationPos.x, operationPos.y, operationPos.z, 1.0f);
			
			if (pivotViewPos.z < 0 && centerViewPos.z < 0) {
				ImDrawList* drawList = ImGui::GetForegroundDrawList();
				
				// Draw pivot indicator (red)
				drawList->AddCircleFilled(pivotScreenPos, 5.0f, IM_COL32(255, 0, 0, 180));
				drawList->AddText(ImVec2(pivotScreenPos.x + 8, pivotScreenPos.y - 8), IM_COL32(255, 0, 0, 255), "Pivot");
				
				// Draw center indicator (green)
				drawList->AddCircleFilled(centerScreenPos, 5.0f, IM_COL32(0, 255, 0, 180));
				drawList->AddText(ImVec2(centerScreenPos.x + 8, centerScreenPos.y - 8), IM_COL32(0, 255, 0, 255), "Center");
				
				// Draw connecting line
				drawList->AddLine(pivotScreenPos, centerScreenPos, IM_COL32(255, 255, 0, 150), 1.0f);
				
				// Add note about transform mode
				const char* modeText = currentMode == TransformMode::Pivot ? 
					"Current Mode: Pivot (press Z to toggle)" : 
					"Current Mode: Center (press Z to toggle)";
				drawList->AddText(ImVec2(imgMin.x + 10, imgMin.y + 40), IM_COL32(255, 255, 255, 200), modeText);
			}
		}

        // FIXED: When starting manipulation, lock the manipulation point based on CURRENT mode
        if (ImGuizmo::IsUsing() && !s_wasManipulating) {
            s_activeTransformMode = TransformModeManager::GetMode();
            
            // Lock to whatever point we're using NOW
            s_manipulationPoint = operationPos; // This is already correct based on current mode
            s_originalPosition = entityWorldPos;
            
            EE_CORE_INFO("Starting manipulation in {} mode at point ({:.3f}, {:.3f}, {:.3f})",
                        (s_activeTransformMode == TransformMode::Center) ? "CENTER" : "PIVOT",
                        s_manipulationPoint.x, s_manipulationPoint.y, s_manipulationPoint.z);
        }

		// FIXED: Build model matrix at the correct position
		glm::mat4 model = glm::mat4(1.0f);
		
		// CRITICAL FIX: Use operationPos consistently throughout the gizmo operation
        // When manipulating, keep using the locked point; otherwise use current mode's point
        Vec3 gizmoPos = ImGuizmo::IsUsing() ? s_manipulationPoint : operationPos;
        
        model = glm::translate(model, glm::vec3(gizmoPos.x, gizmoPos.y, gizmoPos.z));

		// For rotation and scale, we still use the entity's own rotation/scale
		glm::quat rotQuat(tr.rotation.w, tr.rotation.x, tr.rotation.y, tr.rotation.z);
		rotQuat = glm::normalize(rotQuat);
		model *= glm::mat4_cast(rotQuat);
		model = glm::scale(model, glm::vec3(tr.scale.x, tr.scale.y, tr.scale.z));

		// Snapping
		const bool useSnap = Input::IsKeyDownEditor(GLFW_KEY_LEFT_CONTROL)
			|| Input::IsKeyDownEditor(GLFW_KEY_LEFT_SHIFT);
		
		float snapValue = 0.5f;
		if (gOperation == ImGuizmo::ROTATE) snapValue = 15.0f;
		else if (gOperation == ImGuizmo::SCALE) snapValue = 0.1f;

		float snapArr[3] = { snapValue, snapValue, snapValue };

		// Store previous model for delta calculation
		glm::mat4 previousModel = model;
		
		// Manipulate the gizmo
		ImGuizmo::Manipulate(
			&view[0][0], &proj[0][0],
			gOperation, gMode,
			&model[0][0],
			nullptr, useSnap ? snapArr : nullptr
		);

		// FIXED: Handle transformation changes properly
		if (ImGuizmo::IsUsing()) {
            s_wasManipulating = true;
            
            // Calculate the delta transformation
            glm::mat4 delta = model * glm::inverse(previousModel);
            
            if (gOperation == ImGuizmo::TRANSLATE) {
                // Translation: Extract translation from delta and apply to entity
                glm::vec3 deltaPos(delta[3][0], delta[3][1], delta[3][2]);
                Vec3 newWorldPos = hierarchySystem->GetWorldPosition(selectedEntity) + 
                                  Vec3(deltaPos.x, deltaPos.y, deltaPos.z);
                hierarchySystem->SetWorldPosition(selectedEntity, newWorldPos);
                
            } else if (gOperation == ImGuizmo::ROTATE) {
                // Rotation around manipulation point
                // Extract rotation from delta matrix
                glm::mat3 rotMat(delta);
                glm::quat deltaQuat = glm::normalize(glm::quat_cast(rotMat));
                Quaternion deltaRot(deltaQuat.x, deltaQuat.y, deltaQuat.z, deltaQuat.w);
                
                // Rotate around the manipulation point
                hierarchySystem->RotateAroundPointQuat(selectedEntity, s_manipulationPoint, deltaRot);
                
            } else if (gOperation == ImGuizmo::SCALE) {
                // Scale from manipulation point
                glm::vec3 deltaScale = glm::vec3(
                    glm::length(glm::vec3(delta[0])),
                    glm::length(glm::vec3(delta[1])),
                    glm::length(glm::vec3(delta[2]))
                );
                
                Vec3 currentScale = hierarchySystem->GetWorldScale(selectedEntity);
                Vec3 newScale(
                    currentScale.x * deltaScale.x,
                    currentScale.y * deltaScale.y,
                    currentScale.z * deltaScale.z
                );
                hierarchySystem->SetWorldScale(selectedEntity, newScale);
                
                // If scaling from center, adjust position to maintain center point
                if (s_activeTransformMode == TransformMode::Center) {
                    Vec3 currentPos = hierarchySystem->GetWorldPosition(selectedEntity);
                    Vec3 toEntity = currentPos - s_manipulationPoint;
                    
                    // Scale the offset vector
                    Vec3 scaledOffset(
                        toEntity.x * deltaScale.x,
                        toEntity.y * deltaScale.y,
                        toEntity.z * deltaScale.z
                    );
                    
                    Vec3 newPos = s_manipulationPoint + scaledOffset;
                    hierarchySystem->SetWorldPosition(selectedEntity, newPos);
                }
            }
		}
		else if (s_wasManipulating) {
            // Just finished manipulating
            s_wasManipulating = false;
            EE_CORE_INFO("Finished manipulation");
            
            // Update manipulation point if mode is still Center
            if (TransformModeManager::GetMode() == TransformMode::Center) {
                UpdateManipulationPoint(selectedEntity);
            }
        }
	}
}

void Ermine::ViewPortGUI::UpdateManipulationPoint(EntityID entity)
{
    if (!ECS::GetInstance().IsEntityValid(entity))
        return;

    TransformMode currentMode = TransformModeManager::GetMode();
    s_activeTransformMode = currentMode;
    
    // Calculate the manipulation point based on mode
    s_manipulationPoint = TransformModeManager::GetManipulationPosition(entity);
    
    // Get original position for reference
    auto& ecs = ECS::GetInstance();
    auto hierarchySystem = ecs.GetSystem<HierarchySystem>();
    if (hierarchySystem) {
        s_originalPosition = hierarchySystem->GetWorldPosition(entity);
    } else if (ecs.HasComponent<Transform>(entity)) {
        s_originalPosition = ecs.GetComponent<Transform>(entity).position;
    }
    
    EE_CORE_INFO("Updated manipulation point: mode={}, point=({:.3f}, {:.3f}, {:.3f})",
                (currentMode == TransformMode::Pivot) ? "PIVOT" : "CENTER",
                s_manipulationPoint.x, s_manipulationPoint.y, s_manipulationPoint.z);
                
    // Set temporary UI feedback
    s_modeMessage = (currentMode == TransformMode::Pivot) ? "Pivot Mode" : "Center Mode";
    s_modeMessageTimer = 2.0f; // Show for 2 seconds
}

void Ermine::ViewPortGUI::Update()
{
	ImGui::Begin("Scene Viewer", &show);

	LoadToolbarIcons();

	constexpr ImVec2 iconSize = ImVec2(28.f, 28.f);
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const int buttonCount = 4; // Increased from 3 to include transform mode dropdown
	const float totalWidth = buttonCount * iconSize.x + (buttonCount - 1) * spacing + 80.0f; // Added width for dropdown

	// Center horizontally
	float availWidth = ImGui::GetContentRegionAvail().x;
	float startOffsetX = (availWidth > totalWidth) ? (availWidth - totalWidth) * 0.5f : 0.0f;
	float oldX = ImGui::GetCursorPosX();
	ImGui::SetCursorPosX(oldX + startOffsetX);

	TopBarSimulationControl(iconSize);

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

	FrameBufferHandler(viewport_size);
	const auto offscreen_buffer = ECS::GetInstance().GetSystem<graphics::Renderer>()->GetOffscreenBuffer();

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
	selectedEntity = SceneManager::GetInstance().GetActiveScene()->GetSelectedEntity();

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

		// Toggle transform mode with Z key
		if (Input::IsKeyPressedEditor(GLFW_KEY_Z)) {
			// Toggle the mode
			TransformMode newMode = TransformModeManager::ToggleMode();
			s_activeTransformMode = newMode;

			// ALWAYS update the manipulation point when toggling modes
			if (ECS::GetInstance().IsEntityValid(selectedEntity)) {
				UpdateManipulationPoint(selectedEntity);
			}
		}
	}

	OverlayGizmoOperation(imgMin, gOperation, gMode);

	FocusOnSelected(selectedEntity, viewportHovered, viewportFocused);

	static bool s_orbiting = false;
	CameraControls(overViewCube, selectedEntity, viewportHovered, s_orbiting);

	ObjectPicking(offscreen_buffer, imgMin, imgSize, overViewCube, s_orbiting);

	GizmoOverlay(imgMin, imgSize, vmSize, vmPos, selectedEntity, gOperation, gMode);
	
	// Display mode change message if active
	if (s_modeMessageTimer > 0.0f) {
		s_modeMessageTimer -= FrameController::GetDeltaTime();
		
		if (s_modeMessage) {
			ImDrawList* dl = ImGui::GetForegroundDrawList();
			ImVec2 screenCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, 
									   ImGui::GetIO().DisplaySize.y * 0.2f);
			
			float alpha = std::min(1.0f, s_modeMessageTimer);
			
			dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.5f, 
					   ImVec2(screenCenter.x - ImGui::CalcTextSize(s_modeMessage).x * 0.75f, screenCenter.y),
					   ImColor(1.0f, 1.0f, 1.0f, alpha), s_modeMessage);
		}
	}

	ImGui::EndChild();

	if (ImGui::BeginDragDropTarget()) { // Begin drag & drop target
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_BROWSER_FILE")) {
			const char* cpath = static_cast<const char*>(payload->Data);

			if (cpath && payload->DataSize > 0 && cpath[payload->DataSize - 1] == '\0')
			{
				std::string path = cpath;
				PrefabManager::GetInstance().LoadPrefab(path);
			}
			//EE_CORE_INFO("Dropped prefab file: {}", payload->Data);
		}
		ImGui::EndDragDropTarget(); // End drag & drop target
	}

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