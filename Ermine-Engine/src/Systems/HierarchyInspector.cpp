/* Start Header ************************************************************************/
/*!
\file       HierarchyInspector.cpp
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu (30%)
\co-author  WEE HONG RU Curtis, h.wee, 2301266, h.wee\@digipen.edu (70%)
\date       10/09/2025
\brief      Inspector panel for viewing and editing entity properties

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "HierarchyInspector.h"
#include "Components.h"
#include "ECS.h"
#include "GeometryFactory.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "Physics.h"
#include "FiniteStateMachine.h"
#include "FSMEditor.h"
#include <EditorGUI.h>
#include "Particles.h"


#include "xcore/my_properties.h"
#include "xproperty.h"
#include "sprop/property_sprop.h"
#include "sprop/property_sprop_getset.h"

namespace Ermine::editor {
	// --- small helpers ---
	static std::string PrettyLabelFromPath(const std::string& path) {
		// Take the last segment after '/'
		auto slash = path.find_last_of('/');
		std::string s = (slash == std::string::npos) ? path : path.substr(slash + 1);

		std::string out;
		out.reserve(s.size() + 8);
		bool first = true;

		for (size_t i = 0; i < s.size(); ++i) {
			char c = s[i];
			char prev = (out.empty() ? 0 : out.back());

			// Insert space before uppercase if:
			// - not the first character
			// - previous is NOT uppercase
			// - previous is NOT a digit
			if (!first && std::isupper((unsigned char)c) &&
				!(std::isupper((unsigned char)prev) || std::isdigit((unsigned char)prev))) {
				out.push_back(' ');
			}

			// Uppercase first letter
			out.push_back(first ? (char)std::toupper((unsigned char)c) : c);
			first = false;
		}

		return out;
	}

	static bool FieldAppliesToType(const std::string& key, LightType t) {
		if (key == "innerAngle" || key == "outerAngle") return t == LightType::SPOT;
		if (key == "radius") return t == LightType::SPOT || t == LightType::POINT;
		// color, intensity, castsShadows, type are always shown
		return true;
	}
	template<typename T>
	static bool ComponentHeaderWithRemove(const char* headerLabel, EntityID entity,
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen)
	{
		bool open = ImGui::CollapsingHeader(headerLabel, flags);

		// Open context menu when right-clicking the header row
		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::MenuItem("Remove Component")) {
				auto& ecs = Ermine::ECS::GetInstance();
				ecs.RemoveComponent<T>(entity);
				ImGui::EndPopup();
				return false;
			}
			ImGui::EndPopup();
		}

		if (!open) return false;
		return true;
	}

	// Unity-like XYZ control. Returns true if any component changed.
// - Clicking X/Y/Z button resets that axis to resetValue.
// - speed/min/max behave like regular DragFloat.
	static bool DrawVec3XYZ(const char* label, float v[3], float speed = 0.1f, float resetValue = 0.0f, float minVal = -FLT_MAX, float maxVal = FLT_MAX)
	{
		ImGui::PushID(label);

		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, 140.0f);
		ImGui::TextUnformatted(label);
		ImGui::NextColumn();

		bool changed = false;

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());

		const float lineH = ImGui::GetTextLineHeightWithSpacing();
		const ImVec2 btnSize(lineH, lineH);
		const float inner = ImGui::GetStyle().ItemInnerSpacing.x;

		auto axisWidget = [&](const char* id, const char* axisName, float axisColor[3], float& val)
			{
				ImGui::PushID(id);
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(axisColor[0], axisColor[1], axisColor[2], 1.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(axisColor[0] + 0.1f, axisColor[1] + 0.1f, axisColor[2] + 0.1f, 1.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(axisColor[0] + 0.2f, axisColor[1] + 0.2f, axisColor[2] + 0.2f, 1.f));
				if (ImGui::Button(axisName, btnSize)) { val = resetValue; changed = true; }
				ImGui::PopStyleColor(3);

				ImGui::SameLine(0.0f, inner);
				changed |= ImGui::DragFloat("##v", &val, speed, minVal, maxVal, "%.3f");
				ImGui::PopItemWidth();
				ImGui::PopID();

				ImGui::SameLine();
			};

		float colX[3] = { 0.85f, 0.35f, 0.35f }; // red-ish
		float colY[3] = { 0.40f, 0.80f, 0.40f }; // green-ish
		float colZ[3] = { 0.35f, 0.55f, 0.90f }; // blue-ish

		axisWidget("X", "X", colX, v[0]);
		axisWidget("Y", "Y", colY, v[1]);
		// Last axis on the row: avoid trailing SameLine
		ImGui::PushID("Z");
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(colZ[0], colZ[1], colZ[2], 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(colZ[0] + 0.1f, colZ[1] + 0.1f, colZ[2] + 0.1f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(colZ[0] + 0.2f, colZ[1] + 0.2f, colZ[2] + 0.2f, 1.f));
		if (ImGui::Button("Z", btnSize)) { v[2] = resetValue; changed = true; }
		ImGui::PopStyleColor(3);
		ImGui::SameLine(0.0f, inner);
		changed |= ImGui::DragFloat("##v", &v[2], speed, minVal, maxVal, "%.3f");
		ImGui::PopItemWidth();
		ImGui::PopID();

		ImGui::Columns(1);
		ImGui::PopID();

		return changed;
	}

	void HierarchyInspector::OnImGuiRender() {
		if (!m_IsVisible) return;

		ImGui::Begin("Inspector");

		if (!m_ActiveScene) {
			ImGui::Text("No active scene");
			ImGui::End();
			return;
		}

		EntityID selected = m_ActiveScene->GetSelectedEntity(); // Use scene selection
		if (selected == 0) {
			ImGui::Text("No entity selected");
			ImGui::End();
			return;
		}

		// Entity header
		DrawEntityHeader(selected);

		// Draw components with unique IDs
		ImGui::PushID(static_cast<int>(selected));

		if (ECS::GetInstance().HasComponent<Transform>(selected)) {
			DrawTransformComponent(selected);
		}

		if (ECS::GetInstance().HasComponent<Mesh>(selected)) {
			DrawMeshComponent(selected);
		}

		if (ECS::GetInstance().HasComponent<Material>(selected)) {
			DrawMaterialComponent(selected);
		}

		if (ECS::GetInstance().HasComponent<Light>(selected)) {
			DrawLightComponent(selected);
		}

		if (ECS::GetInstance().HasComponent<HierarchyComponent>(selected)) {
			DrawHierarchyComponent(selected);
		}

		if (ECS::GetInstance().HasComponent<PhysicComponent>(selected)) {
			DrawPhysicsComponent(selected);
		}

		if (ECS::GetInstance().HasComponent<AudioComponent>(selected)) {
			DrawAudioComponent(selected);
		}

		if (ECS::GetInstance().HasComponent<Script>(selected)) {
			DrawScriptComponent(selected);
		}

		if (ECS::GetInstance().HasComponent<ModelComponent>(selected)) {
			DrawModelComponent(selected);
		}

		if (ECS::GetInstance().HasComponent<AnimationComponent>(selected)) {
			DrawAnimationComponent(selected);
		}

		if (ECS::GetInstance().HasComponent<StateMachine>(selected)) {
			DrawStateMachineComponent(selected);
		}

		if (ECS::GetInstance().HasComponent<ParticleEmitter>(selected)) {
			DrawParticleEmitterComponent(selected);
		}

		ImGui::PopID();

		ImGui::Separator();

		// Add Component button
		if (ImGui::Button("Add Component")) {
			ImGui::OpenPopup("AddComponent");
		}

		if (ImGui::BeginPopup("AddComponent")) {
			DrawAddComponentMenu(selected);
			ImGui::EndPopup();
		}

		ImGui::End();
	}

	void HierarchyInspector::DrawEntityHeader(EntityID entity) {
		if (!ECS::GetInstance().HasComponent<ObjectMetaData>(entity)) {
			ImGui::Text("Entity ID: %u", entity);
			ImGui::Text("Missing ObjectMetaData component");
			ImGui::Separator();
			return;
		}

		auto& metadata = ECS::GetInstance().GetComponent<ObjectMetaData>(entity);

		// Collect reflective properties
		xproperty::settings::context ctx{};
		xproperty::sprop::container  bag;
		xproperty::sprop::collector  collect(metadata, bag, ctx, /*forEditors=*/true);

		std::string err;

		for (auto& p : bag.m_Properties) {
			const auto guid = p.m_Value.getTypeGuid();
			const char* id = p.m_Path.c_str();                 // unique ID
			std::string label = PrettyLabelFromPath(p.m_Path);    // pretty label (no "ObjectMetaData/")

			ImGui::PushID(id);

			if (guid == xproperty::settings::var_type<std::string>::guid_v) {
				std::string s = p.m_Value.get<std::string>();
				char buf[256]; std::snprintf(buf, sizeof(buf), "%s", s.c_str());
				if (ImGui::InputText(label.c_str(), buf, IM_ARRAYSIZE(buf))) {
					p.m_Value.set<std::string>(buf);
					xproperty::sprop::setProperty(err, metadata, p, ctx);
				}
			}
			else if (guid == xproperty::settings::var_type<bool>::guid_v) {
				bool v = p.m_Value.get<bool>();
				if (ImGui::Checkbox(label.c_str(), &v)) {
					p.m_Value.set<bool>(v);
					xproperty::sprop::setProperty(err, metadata, p, ctx);
				}
			}

			ImGui::PopID();
		}

		// Entity ID (read-only)
		ImGui::Text("Entity ID: %u", entity);
		ImGui::Separator();

		if (!err.empty())
			ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Error: %s", err.c_str());
	}

	void HierarchyInspector::DrawTransformComponent(EntityID entity) {
		if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
			return;

		auto& t = ECS::GetInstance().GetComponent<Transform>(entity);

		xproperty::settings::context ctx{};
		xproperty::sprop::container bag;
		xproperty::sprop::collector collect(t, bag, ctx, /*forEditors=*/true);

		std::string err;

		for (auto& p : bag.m_Properties)
		{
			const auto guid = p.m_Value.getTypeGuid();
			const char* id = p.m_Path.c_str();                  // unique
			std::string label = PrettyLabelFromPath(p.m_Path);     // pretty, no "Transform/"

			ImGui::PushID(id);

			// Vec3 (position / scale)
			if (guid == xproperty::settings::var_type<Ermine::Vec3>::guid_v) {
				Ermine::Vec3 v = p.m_Value.get<Ermine::Vec3>();
				if (DrawVec3XYZ(label.c_str(), &v.x)) {
					p.m_Value.set<Ermine::Vec3>({ v.x, v.y, v.z });
					xproperty::sprop::setProperty(err, t, p, ctx);
				}
			}
			// Quaternion (rotation) � shown/edited as Euler degrees
			else if (guid == xproperty::settings::var_type<Ermine::Quaternion>::guid_v) {
				Ermine::Quaternion q = p.m_Value.get<Ermine::Quaternion>();

				// Convert to Euler (degrees) for UI
				Ermine::Vec3 eulerDeg = QuaternionToEuler(q, /*degrees*/true);

				// If the field is named "rotation", make the label explicit
				const bool isRotation = (label == "Rotation");
				const char* rotLabel = isRotation ? "Rotation (Degrees)" : label.c_str();

				if (DrawVec3XYZ(rotLabel, &eulerDeg.x, 1.0f, 0.0f, -360.0f, 360.0f)) {
					// Build quaternion back from XYZ degrees (Z * Y * X like before)

					//const float rx = eulerDeg.x * (float)M_PI / 180.0f;
					//const float ry = eulerDeg.y * (float)M_PI / 180.0f;
					//const float rz = eulerDeg.z * (float)M_PI / 180.0f;

					//Matrix4x4 mx, my, mz, m;
					//Mtx44Identity(mx); Mtx44Identity(my); Mtx44Identity(mz);
					//Mtx44RotXRad(mx, rx); Mtx44RotYRad(my, ry); Mtx44RotZRad(mz, rz);
					//m = mz * my * mx;

					//q = Mtx44GetQuaternion(m);
					p.m_Value.set<Ermine::Quaternion>(FromEulerDegrees(eulerDeg));
					xproperty::sprop::setProperty(err, t, p, ctx);
				}
			}

			ImGui::PopID();
		}

		if (!err.empty())
			ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "xprop: %s", err.c_str());
	}

	void HierarchyInspector::DrawMeshComponent(EntityID entity) {
		if (!ComponentHeaderWithRemove<Mesh>("Mesh", entity))
			return;

		auto& mesh = ECS::GetInstance().GetComponent<Mesh>(entity);

		// Combo for mesh kind
		const char* kinds[] = { "None", "Primitive", "Asset" };
		int currentKind = static_cast<int>(mesh.kind);
		if (ImGui::Combo("Kind", &currentKind, kinds, IM_ARRAYSIZE(kinds))) {
			mesh.kind = static_cast<MeshKind>(currentKind);
			if (mesh.kind == MeshKind::Primitive)
				mesh.RebuildPrimitive();
		}

		// Primitive controls
		if (mesh.kind == MeshKind::Primitive) {
			// Shape type dropdown
			const char* types[] = { "Cube", "Sphere", "Quad" };
			int currentType = 0;
			if (mesh.primitive.type == "Sphere") currentType = 1;
			else if (mesh.primitive.type == "Quad") currentType = 2;

			if (ImGui::Combo("Primitive Type", &currentType, types, IM_ARRAYSIZE(types))) {
				mesh.primitive.type = types[currentType];
				mesh.RebuildPrimitive();
			}

			// Size control
			float size[3] = { mesh.primitive.size.x, mesh.primitive.size.y, mesh.primitive.size.z };
			if (ImGui::DragFloat3("Size", size, 0.1f, 0.01f, 100.f)) {
				mesh.primitive.size = { size[0], size[1], size[2] };
				mesh.RebuildPrimitive();
			}
		}

		// Asset controls (basic stub)
		if (mesh.kind == MeshKind::Asset) {
			char buf[256];
			strcpy_s(buf, mesh.asset.meshName.c_str());
			if (ImGui::InputText("Mesh Name", buf, sizeof(buf))) {
				mesh.asset.meshName = buf;
				// TODO: trigger asset reload here
			}
		}
	}

	void HierarchyInspector::DrawMaterialComponent(EntityID entity) {
		if (!ComponentHeaderWithRemove<Material>("Material", entity))
			return;

		// --- Fetch component & underlying material safely ---
		auto& matComp = ECS::GetInstance().GetComponent<Material>(entity);
		graphics::Material* gm = matComp.GetMaterial();

		if (!gm) {
			//ImGui::TextUnformatted("No material bound.");
			//if (ImGui::Button("Create Default PBR")) {
			//	matComp = Material(std::make_shared<graphics::Material>());
			//	gm = matComp.GetMaterial();
			//	if (gm) {
			//		Vec4 alb{ 1.f,1.f,1.f,1.f };
			//		gm->SetVec4("materialAlbedo", alb);
			//		gm->SetVec4("material.albedo", alb);
			//		gm->SetFloat("materialAlpha", 1.0f);
			//		gm->SetFloat("materialTransparency", 0.0f);

			//		gm->SetFloat("materialMetallic", 0.0f);           gm->SetFloat("material.metallic", 0.0f);
			//		gm->SetFloat("materialRoughness", 0.5f);          gm->SetFloat("material.roughness", 0.5f);
			//		gm->SetVec3("materialEmissive", { 0.f,0.f,0.f });   gm->SetVec3("material.emissive", { 0.f,0.f,0.f });
			//		gm->SetFloat("materialEmissiveIntensity", 1.0f);  gm->SetFloat("material.emissiveIntensity", 1.0f);
			//	}
			//}
			//ImGui::Separator();

			matComp = Material(std::make_shared<graphics::Material>());
			gm = matComp.GetMaterial();
			if (gm) {
				Vec4 alb{ 1.f,1.f,1.f,1.f };
				gm->SetVec4("materialAlbedo", alb);
				gm->SetVec4("material.albedo", alb);
				gm->SetFloat("materialAlpha", 1.0f);
				gm->SetFloat("materialTransparency", 0.0f);

				gm->SetFloat("materialMetallic", 0.0f);           gm->SetFloat("material.metallic", 0.0f);
				gm->SetFloat("materialRoughness", 0.5f);          gm->SetFloat("material.roughness", 0.5f);
				gm->SetVec3("materialEmissive", { 0.f,0.f,0.f });   gm->SetVec3("material.emissive", { 0.f,0.f,0.f });
				gm->SetFloat("materialEmissiveIntensity", 1.0f);  gm->SetFloat("material.emissiveIntensity", 1.0f);
			}

			return;
		}

		// --- Safe param accessors (no nullptrs, with fallbacks) ---
		auto getFloat = [&](const char* a, const char* b, float fb) -> float {
			if (auto p = gm->GetParameter(a); p && !p->floatValues.empty()) return p->floatValues[0];
			if (auto q = gm->GetParameter(b); q && !q->floatValues.empty()) return q->floatValues[0];
			return fb;
			};
		auto getVec3 = [&](const char* a, const char* b, const Vec3& fb) -> Vec3 {
			if (auto p = gm->GetParameter(a); p && p->floatValues.size() >= 3)
				return Vec3(p->floatValues[0], p->floatValues[1], p->floatValues[2]);
			if (auto q = gm->GetParameter(b); q && q->floatValues.size() >= 3)
				return Vec3(q->floatValues[0], q->floatValues[1], q->floatValues[2]);
			return fb;
			};
		auto getBool = [&](const char* a, const char* b, bool fb) -> bool {
			if (auto p = gm->GetParameter(a)) return p->boolValue;
			if (b) { if (auto q = gm->GetParameter(b)) return q->boolValue; }
			return fb;
			};

		auto setFloatBoth = [&](const char* a, const char* b, float v) {
			gm->SetFloat(a, v); gm->SetFloat(b, v);
			};
		auto setVec3Both = [&](const char* a, const char* b, const Vec3& v) {
			gm->SetVec3(a, v); gm->SetVec3(b, v);
			};
		auto setBoolBoth = [&](const char* a, const char* b, bool v) {
			gm->SetBool(a, v); if (b) gm->SetBool(b, v);
			};

		auto getAlbedoRGBA = [&]() -> Vec4 {
			// Try main param as vec4
			if (auto p = gm->GetParameter("materialAlbedo"); p && p->floatValues.size() >= 4)
				return Vec4(p->floatValues[0], p->floatValues[1], p->floatValues[2], p->floatValues[3]);

			// Try alias as vec4
			if (auto q = gm->GetParameter("material.albedo"); q && q->floatValues.size() >= 4)
				return Vec4(q->floatValues[0], q->floatValues[1], q->floatValues[2], q->floatValues[3]);

			// Fallback: RGB + (alpha from materialAlpha or 1 - transparency)
			Vec3 rgb = getVec3("materialAlbedo", "material.albedo", matComp.cacheAlbedo);
			float a = 1.0f;
			if (auto aP = gm->GetParameter("materialAlpha"); aP && !aP->floatValues.empty())
				a = aP->floatValues[0];
			else if (auto tP = gm->GetParameter("materialTransparency"); tP && !tP->floatValues.empty())
				a = 1.0f - tP->floatValues[0];

			return Vec4(rgb.x, rgb.y, rgb.z, std::clamp(a, 0.0f, 1.0f));
			};

		auto setAlbedoRGBA = [&](const Vec4& rgba) {
			Vec4 clamped{ std::clamp(rgba.x,0.f,1.f),
						  std::clamp(rgba.y,0.f,1.f),
						  std::clamp(rgba.z,0.f,1.f),
						  std::clamp(rgba.w,0.f,1.f) };
			gm->SetVec4("materialAlbedo", clamped);
			gm->SetVec4("material.albedo", clamped);       // keep alias in lockstep
			gm->SetFloat("materialAlpha", clamped.w);      // mirrors for compat
			gm->SetFloat("materialTransparency", 1.0f - clamped.w);

			// keep component caches updated (Vec3 only)
			matComp.hasAlbedo = true;
			matComp.cacheAlbedo = Vec3(clamped.x, clamped.y, clamped.z);
			};

		// --- Albedo (RGBA) ---
		{
			Vec4 rgba = getAlbedoRGBA();
			float col4[4] = { rgba.x, rgba.y, rgba.z, rgba.w };

			// Edit RGBA (with alpha bar/preview)
			if (ImGui::ColorEdit4("Albedo", col4,
				ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf)) {
				setAlbedoRGBA(Vec4(col4[0], col4[1], col4[2], col4[3]));
			}

			// Optional: separate Transparency (1 - alpha) control for clarity
			float transparency = 1.0f - col4[3];
			if (ImGui::SliderFloat("Transparency", &transparency, 0.0f, 1.0f)) {
				setAlbedoRGBA(Vec4(col4[0], col4[1], col4[2], 1.0f - transparency));
			}
		}

		// --- Metallic ---
		{
			float m = getFloat("materialMetallic", "material.metallic", matComp.cacheMetallic);
			if (ImGui::SliderFloat("Metallic", &m, 0.0f, 1.0f)) {
				matComp.hasMetal = true;
				matComp.cacheMetallic = m;
				setFloatBoth("materialMetallic", "material.metallic", m);
			}
		}

		// --- Roughness ---
		{
			float r = getFloat("materialRoughness", "material.roughness", matComp.cacheRoughness);
			if (ImGui::SliderFloat("Roughness", &r, 0.0f, 1.0f)) {
				matComp.hasRough = true;
				matComp.cacheRoughness = r;
				setFloatBoth("materialRoughness", "material.roughness", r);
			}
		}

		ImGui::SeparatorText("Emissive");

		// --- Emissive color & intensity ---
		{
			Vec3 e = getVec3("materialEmissive", "material.emissive", matComp.cacheEmissive);
			float col[3] = { e.x, e.y, e.z };
			float I = getFloat("materialEmissiveIntensity", "material.emissiveIntensity", matComp.cacheEmissiveIntensity);

			bool chC = ImGui::ColorEdit3("Emissive Color", col);
			bool chI = ImGui::SliderFloat("Emissive Intensity", &I, 0.0f, 10.0f);

			if (chC || chI) {
				e = { col[0], col[1], col[2] };
				matComp.hasEmiss = true;
				matComp.cacheEmissive = e;
				matComp.cacheEmissiveIntensity = I;
				setVec3Both("materialEmissive", "material.emissive", e);
				setFloatBoth("materialEmissiveIntensity", "material.emissiveIntensity", I);
			}
		}

		ImGui::SeparatorText("Maps");

		// --- Presence flags (use same keys as (de)serialize) ---
		bool hasAlbMap = getBool("materialHasAlbedoMap", nullptr, false);
		bool hasNorm = getBool("materialHasNormalMap", "material.hasNormalMap", false);
		bool hasRghMap = getBool("materialHasRoughnessMap", nullptr, false);
		bool hasMetMap = getBool("materialHasMetallicMap", nullptr, false);
		bool hasAoMap = getBool("materialHasAoMap", nullptr, false);
		bool hasEmiMap = getBool("materialHasEmissiveMap", nullptr, false);

		if (ImGui::Checkbox("Albedo Map", &hasAlbMap)) {
			gm->SetBool("materialHasAlbedoMap", hasAlbMap);
		}
		if (ImGui::Checkbox("Normal Map", &hasNorm)) {
			setBoolBoth("materialHasNormalMap", "material.hasNormalMap", hasNorm);
		}
		if (ImGui::Checkbox("Roughness Map", &hasRghMap)) {
			gm->SetBool("materialHasRoughnessMap", hasRghMap);
		}
		if (ImGui::Checkbox("Metallic Map", &hasMetMap)) {
			gm->SetBool("materialHasMetallicMap", hasMetMap);
		}
		if (ImGui::Checkbox("AO Map", &hasAoMap)) {
			gm->SetBool("materialHasAoMap", hasAoMap);
		}
		if (ImGui::Checkbox("Emissive Map", &hasEmiMap)) {
			gm->SetBool("materialHasEmissiveMap", hasEmiMap);
		}

		ImGui::Separator();

		// --- Safe reset button ---
		if (ImGui::SmallButton("Reset to Defaults")) {
			Vec4 alb{ 1.f,1.f,1.f,1.f };
			setAlbedoRGBA(alb);

			setFloatBoth("materialMetallic", "material.metallic", 0.0f);
			setFloatBoth("materialRoughness", "material.roughness", 0.5f);
			Vec3 emi{ 0.f,0.f,0.f }; setVec3Both("materialEmissive", "material.emissive", emi);
			setFloatBoth("materialEmissiveIntensity", "material.emissiveIntensity", 1.0f);

			gm->SetBool("materialHasAlbedoMap", false);
			setBoolBoth("materialHasNormalMap", "material.hasNormalMap", false);
			gm->SetBool("materialHasRoughnessMap", false);
			gm->SetBool("materialHasMetallicMap", false);
			gm->SetBool("materialHasAoMap", false);
			gm->SetBool("materialHasEmissiveMap", false);

			matComp.hasAlbedo = matComp.hasMetal = matComp.hasRough = matComp.hasEmiss = true;
			matComp.cacheMetallic = 0.0f;
			matComp.cacheRoughness = 0.5f;
			matComp.cacheEmissive = emi;
			matComp.cacheEmissiveIntensity = 1.0f;
		}

		ImGui::SeparatorText("Textures");

		struct SlotRow {
			const char* label;          // UI label
			const char* slot;           // primary slot name used by your material
			const char* altSlot;        // optional alias slot (only albedo needs this)
			const char* hasFlag;        // presence flag (primary)
			const char* hasFlagAlias;   // presence flag alias (only normal uses this)
		};
		SlotRow rows[] = {
			{ "Albedo",    "materialAlbedoMap", "material.albedoMap", "materialHasAlbedoMap", nullptr },
			{ "Normal",    "material.normalMap", nullptr,              "materialHasNormalMap", "material.hasNormalMap" },
			{ "Roughness", "materialRoughnessMap", nullptr,            "materialHasRoughnessMap", nullptr },
			{ "Metallic",  "material.metallicMap", nullptr,            "materialHasMetallicMap", nullptr },
			{ "AO",        "materialAoMap", nullptr,                   "materialHasAoMap", nullptr },
			{ "Emissive",  "materialEmissiveMap", nullptr,             "materialHasEmissiveMap", nullptr },
		};

		// Build a stable list of choices: <None> + all loaded texture paths
		std::vector<std::string> choices;
		choices.emplace_back("<None>");
		std::vector<std::shared_ptr<graphics::Texture>> choicePtrs;
		choicePtrs.emplace_back(nullptr);

		const auto& loaded = AssetManager::GetInstance().GetLoadedTextures(); // map<path, texture>
		choices.reserve(choices.size() + loaded.size());
		choicePtrs.reserve(choicePtrs.size() + loaded.size());
		for (const auto& kv : loaded) {
			choices.emplace_back(kv.first);
			choicePtrs.emplace_back(kv.second);
		}

		// Utility to show a combo for one slot
		auto showTextureCombo = [&](const SlotRow& r) {
			// Resolve current texture for this slot
			std::shared_ptr<graphics::Texture> curTex = gm->GetTexture(r.slot);
			// Find current index
			int currentIdx = 0; // <None>
			if (curTex) {
				for (int i = 1; i < (int)choicePtrs.size(); ++i) {
					if (choicePtrs[i].get() == curTex.get()) { currentIdx = i; break; }
				}
			}

			// Combo UI
			ImGui::PushID(r.slot);
			if (ImGui::BeginCombo(r.label, choices[currentIdx].c_str())) {
				for (int i = 0; i < (int)choices.size(); ++i) {
					bool selected = (i == currentIdx);
					if (ImGui::Selectable(choices[i].c_str(), selected)) {
						currentIdx = i;

						// Apply selection
						if (currentIdx == 0) {
							// None -> clear slot
							gm->SetTexture(r.slot, nullptr);
							if (r.altSlot) gm->SetTexture(r.altSlot, nullptr);
							if (r.hasFlag) gm->SetBool(r.hasFlag, false);
							if (r.hasFlagAlias) gm->SetBool(r.hasFlagAlias, false);
						}
						else {
							auto newTex = choicePtrs[currentIdx];
							if (newTex && newTex->IsValid()) {
								gm->SetTexture(r.slot, newTex);
								if (r.altSlot) gm->SetTexture(r.altSlot, newTex); // albedo alias
								if (r.hasFlag) gm->SetBool(r.hasFlag, true);
								if (r.hasFlagAlias) gm->SetBool(r.hasFlagAlias, true); // normal alias
							}
						}
					}
					if (selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::PopID();
			};

		// Rows
		for (const auto& row : rows) {
			showTextureCombo(row);
		}
	}

	void HierarchyInspector::DrawLightComponent(EntityID entity)
	{
		if (!ComponentHeaderWithRemove<Light>("Light", entity))
			return;

		auto& light = ECS::GetInstance().GetComponent<Light>(entity);

		xproperty::settings::context ctx{};
		xproperty::sprop::container  bag;
		xproperty::sprop::collector  collect(light, bag, ctx, /*forEditors=*/true);

		std::string err;

		// Optional: section headers
		ImGui::TextDisabled("Common");
		ImGui::Separator();

		for (auto& p : bag.m_Properties)
		{
			const auto guid = p.m_Value.getTypeGuid();
			const std::string key = [&] {
				const auto slash = p.m_Path.find_last_of('/');
				return (slash == std::string::npos) ? p.m_Path : p.m_Path.substr(slash + 1);
				}();

			if (!FieldAppliesToType(key, light.type))
				continue; // hide irrelevant fields

			const std::string labelStr = PrettyLabelFromPath(p.m_Path);
			const char* label = labelStr.c_str();

			ImGui::PushID(p.m_Path.c_str());

			// --- strings (none in Light, but generic kept for future fields) ---
			if (guid == xproperty::settings::var_type<std::string>::guid_v) {
				std::string s = p.m_Value.get<std::string>();
				char buf[256]; std::snprintf(buf, sizeof(buf), "%s", s.c_str());
				if (ImGui::InputText(label, buf, IM_ARRAYSIZE(buf))) {
					p.m_Value.set<std::string>(buf);
					xproperty::sprop::setProperty(err, light, p, ctx);
				}
			}
			// --- booleans ---
			else if (guid == xproperty::settings::var_type<bool>::guid_v) {
				bool b = p.m_Value.get<bool>();
				if (ImGui::Checkbox(label, &b)) {
					p.m_Value.set<bool>(b);
					xproperty::sprop::setProperty(err, light, p, ctx);
				}
			}
			// --- floats ---
			else if (guid == xproperty::settings::var_type<float>::guid_v) {
				float v = p.m_Value.get<float>();
				if (key == "intensity") {
					if (ImGui::SliderFloat("Intensity", &v, 0.0f, 10.0f)) {
						p.m_Value.set<float>(v);
						xproperty::sprop::setProperty(err, light, p, ctx);
					}
				}
				else if (key == "innerAngle") {
					if (ImGui::DragFloat("Inner Angle (deg)", &v, 0.1f, 0.0f, 180.0f)) {
						p.m_Value.set<float>(v);
						xproperty::sprop::setProperty(err, light, p, ctx);
					}
				}
				else if (key == "outerAngle") {
					if (ImGui::DragFloat("Outer Angle (deg)", &v, 0.1f, 0.0f, 180.0f)) {
						p.m_Value.set<float>(v);
						xproperty::sprop::setProperty(err, light, p, ctx);
					}
				}
				else if (key == "radius") {
					if (ImGui::DragFloat("Radius", &v, 0.01f, 0.0f)) {
						p.m_Value.set<float>(v);
						xproperty::sprop::setProperty(err, light, p, ctx);
					}
				}
				else {
					if (ImGui::DragFloat(label, &v, 0.01f)) {
						p.m_Value.set<float>(v);
						xproperty::sprop::setProperty(err, light, p, ctx);
					}
				}
			}
			// --- Vec3 (color) ---
			else if (guid == xproperty::settings::var_type<Ermine::Vec3>::guid_v
				|| guid == xproperty::settings::var_type<Vec3>::guid_v) {
				auto v = (guid == xproperty::settings::var_type<Ermine::Vec3>::guid_v)
					? p.m_Value.get<Ermine::Vec3>()
					: Ermine::Vec3{ p.m_Value.get<Vec3>().x, p.m_Value.get<Vec3>().y, p.m_Value.get<Vec3>().z };

				float a[3] = { v.x, v.y, v.z };
				if (key == "color" ? ImGui::ColorEdit3("Color", a) : ImGui::DragFloat3(label, a, 0.01f)) {
					Ermine::Vec3 nv{ a[0], a[1], a[2] };
					p.m_Value.set<Ermine::Vec3>(nv); // set canonical type
					xproperty::sprop::setProperty(err, light, p, ctx);
				}
			}
			// --- enum: LightType ---
			else if (guid == xproperty::settings::var_type<LightType>::guid_v) {
				int idx = static_cast<int>(p.m_Value.get<LightType>());
				const char* names[] = { "Point", "Directional", "Spot" };
				if (ImGui::Combo("Type", &idx, names, IM_ARRAYSIZE(names))) {
					p.m_Value.set<LightType>(static_cast<LightType>(idx));
					xproperty::sprop::setProperty(err, light, p, ctx);
				}
			}
			else {
				// Unknown type: show a read-only stub so you see it's there
				ImGui::TextDisabled("%s (unhandled type)", label);
			}

			ImGui::PopID();
		}

		if (!err.empty())
			ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "xprop: %s", err.c_str());
	}

	void HierarchyInspector::DrawHierarchyComponent(EntityID entity) {
		if (ImGui::CollapsingHeader("Hierarchy")) {
			auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);

			if (hierarchy.parent != 0) {
				if (ECS::GetInstance().HasComponent<ObjectMetaData>(hierarchy.parent)) {
					auto& metadata = ECS::GetInstance().GetComponent<ObjectMetaData>(hierarchy.parent);
					ImGui::Text("Parent: %s", metadata.name.c_str());
				}
			}
			else {
				ImGui::Text("Parent: None");
			}

			if (!hierarchy.children.empty()) {
				if (ImGui::TreeNode("Children")) {
					for (auto child : hierarchy.children) {
						if (ECS::GetInstance().HasComponent<ObjectMetaData>(child)) {
							auto& metadata = ECS::GetInstance().GetComponent<ObjectMetaData>(child);
							ImGui::BulletText("%s", metadata.name.c_str());
						}
					}
					ImGui::TreePop();
				}
			}
		}
	}

	void HierarchyInspector::DrawPhysicsComponent(EntityID entity)
	{
		bool open = ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen);
		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::MenuItem("Remove Component")) {
				auto& ecs = ECS::GetInstance();
				ecs.RemoveComponent<PhysicComponent>(entity);
				ecs.GetSystem<Physics>()->UpdatePhysicList();  // keep physics in sync
				ImGui::EndPopup();
				return;
			}
			ImGui::EndPopup();
		}
		if (!open) return;

		auto& pc = ECS::GetInstance().GetComponent<PhysicComponent>(entity);

		xproperty::settings::context ctx{};
		xproperty::sprop::container  bag;
		xproperty::sprop::collector  collect(pc, bag, ctx, /*forEditors=*/true);

		std::string err;

		// Draw reflected scalars/enums only
		for (auto& p : bag.m_Properties)
		{
			const auto guid = p.m_Value.getTypeGuid();
			const char* id = p.m_Path.c_str();
			std::string label = PrettyLabelFromPath(p.m_Path);

			ImGui::PushID(id);

			// Mass (float, clamp >= 0)
			if (guid == xproperty::settings::var_type<float>::guid_v && label == "Mass") {
				float m = p.m_Value.get<float>();
				if (ImGui::DragFloat("Mass", &m, 0.01f, 0.0f)) {
					if (m < 0.0f) m = 0.0f;
					p.m_Value.set<float>(m);
					xproperty::sprop::setProperty(err, pc, p, ctx);
					ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();
				}
			}
			// PhysicsBodyType
			else if (guid == xproperty::settings::var_type<PhysicsBodyType>::guid_v) {
				int idx = static_cast<int>(p.m_Value.get<PhysicsBodyType>());
				const char* names[] = { "Rigidbody", "Trigger" };
				if (ImGui::Combo("Physics Body Type", &idx, names, IM_ARRAYSIZE(names))) {
					p.m_Value.set<PhysicsBodyType>(static_cast<PhysicsBodyType>(idx));
					xproperty::sprop::setProperty(err, pc, p, ctx);
					ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();
				}
			}
			// JPH::EMotionType
			else if (guid == xproperty::settings::var_type<JPH::EMotionType>::guid_v) {
				int idx = static_cast<int>(p.m_Value.get<JPH::EMotionType>());
				const char* names[] = { "Static", "Kinematic", "Dynamic" };
				if (ImGui::Combo("Motion Type", &idx, names, IM_ARRAYSIZE(names))) {
					p.m_Value.set<JPH::EMotionType>(static_cast<JPH::EMotionType>(idx));
					xproperty::sprop::setProperty(err, pc, p, ctx);
					ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();
				}
			}
			// ShapeType
			else if (guid == xproperty::settings::var_type<ShapeType>::guid_v) {
				int idx = static_cast<int>(p.m_Value.get<ShapeType>());
				const char* names[] = { "Box", "Sphere", "Capsule", "CustomMesh" };
				if (ImGui::Combo("Shape Type", &idx, names, (int)ShapeType::Total)) {
					p.m_Value.set<ShapeType>(static_cast<ShapeType>(idx));
					xproperty::sprop::setProperty(err, pc, p, ctx);
					ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();
				}
			}

			ImGui::PopID();
		}

		if (!err.empty())
			ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "xprop: %s", err.c_str());

		ImGui::Separator();
	}

	void HierarchyInspector::DrawAudioComponent(EntityID entity)
	{
		if (!ComponentHeaderWithRemove<AudioComponent>("Audio", entity))
			return;

		if (!ImGui::CollapsingHeader("Audio", ImGuiTreeNodeFlags_DefaultOpen))
			return;

		auto& audio = ECS::GetInstance().GetComponent<AudioComponent>(entity);

		// Collect reflective properties
		xproperty::settings::context ctx{};
		xproperty::sprop::container  bag;
		xproperty::sprop::collector  collect(audio, bag, ctx, true);

		std::string err;

		for (auto& p : bag.m_Properties)
		{
			const auto guid = p.m_Value.getTypeGuid();
			const char* id = p.m_Path.c_str();
			std::string label = PrettyLabelFromPath(p.m_Path);

			ImGui::PushID(id);

			// string fields
			if (guid == xproperty::settings::var_type<std::string>::guid_v) {
				std::string s = p.m_Value.get<std::string>();
				char buf[256]; std::snprintf(buf, sizeof(buf), "%s", s.c_str());
				if (ImGui::InputText(label.c_str(), buf, IM_ARRAYSIZE(buf))) {
					p.m_Value.set<std::string>(buf);
					xproperty::sprop::setProperty(err, audio, p, ctx);
				}
			}
			// bool fields
			else if (guid == xproperty::settings::var_type<bool>::guid_v) {
				bool v = p.m_Value.get<bool>();
				if (ImGui::Checkbox(label.c_str(), &v)) {
					p.m_Value.set<bool>(v);
					xproperty::sprop::setProperty(err, audio, p, ctx);
				}
			}
			// float fields
			else if (guid == xproperty::settings::var_type<float>::guid_v) {
				float v = p.m_Value.get<float>();
				if (ImGui::DragFloat(label.c_str(), &v, 0.01f, 0.0f, 1.0f)) {
					p.m_Value.set<float>(v);
					xproperty::sprop::setProperty(err, audio, p, ctx);
				}
			}
			// int fields
			else if (guid == xproperty::settings::var_type<int>::guid_v) {
				int v = p.m_Value.get<int>();
				if (ImGui::DragInt(label.c_str(), &v)) {
					p.m_Value.set<int>(v);
					xproperty::sprop::setProperty(err, audio, p, ctx);
				}
			}

			ImGui::PopID();
		}

		ImGui::Separator();

		// Optional quick preview buttons
		if (ImGui::Button("Play")) {
			// TODO: AudioSystem::Get().Play(audio.soundName, entity);
		}
		ImGui::SameLine();
		if (ImGui::Button("Stop")) {
			// TODO: AudioSystem::Get().Stop(entity);
		}

		if (!err.empty())
			ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Error: %s", err.c_str());
	}

	void HierarchyInspector::DrawScriptComponent(EntityID entity)
	{
		if (!ComponentHeaderWithRemove<Script>("Script", entity))
			return;

		auto& script = ECS::GetInstance().GetComponent<Script>(entity);

		// Helper: property row with label on the left and widget on the right
		auto PropertyRow = [&](const char* label, auto&& widgetFn)
			{
				ImGui::Columns(2, nullptr, false);
				ImGui::SetColumnWidth(0, 140.0f);
				ImGui::TextUnformatted(label);
				ImGui::NextColumn();
				widgetFn();
				ImGui::Columns(1);
			};

		// Editable class name (label left)
		{
			char buf[256];
			strcpy_s(buf, script.m_className.c_str());
			PropertyRow("Class", [&] {
				if (ImGui::InputText("##Class", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue)) { // TODO: validate classname?
					script = Script{ std::string(buf), entity }; // Reconstruct a new state to initialize the Script
				}
				});
		}

		// Display fields
		std::unordered_map<std::string, ScriptFieldValue> fields;
		if (script.m_instance)
			scripting::ScriptEngine::PullManagedFieldsToCache(script.m_instance->object, fields);

		for (auto& [name, val] : fields)
		{
			ImGui::PushID(name.c_str());

			switch (val.kind)
			{
			case ScriptFieldValue::Kind::Float:
			{
				PropertyRow(name.c_str(), [&] {
					ImGui::InputFloat("##v", &std::get<float>(val.value));
					});
				break;
			}
			case ScriptFieldValue::Kind::Int:
			{
				PropertyRow(name.c_str(), [&] {
					ImGui::InputInt("##v", &std::get<int>(val.value));
					});
				break;
			}
			case ScriptFieldValue::Kind::Bool:
			{
				PropertyRow(name.c_str(), [&] {
					ImGui::Checkbox("##v", &std::get<bool>(val.value));
					});
				break;
			}
			case ScriptFieldValue::Kind::Vector3:
			{
				// Uses internal two-column layout with label left
				if (DrawVec3XYZ(name.c_str(), &std::get<Vec3>(val.value).x))
				{
				}
				break;
			}
			case ScriptFieldValue::Kind::Quaternion:
			{
				// Uses internal two-column layout with label left
				Vec3 euler = QuaternionToEuler(std::get<Quaternion>(val.value), true);
				if (DrawVec3XYZ(name.c_str(), &euler.x))
				{
				}
				val.value = FromEulerDegrees(euler);
				break;
			}
			case ScriptFieldValue::Kind::String:
			{
				char innerBuff[256];
				strcpy_s(innerBuff, std::get<std::string>(val.value).c_str());
				PropertyRow(name.c_str(), [&] {
					if (ImGui::InputText("##v", innerBuff, sizeof(innerBuff), ImGuiInputTextFlags_EnterReturnsTrue))
					{
						// nothing else to do here; value is set when pushing back to managed fields
					}
					});
				val.value = innerBuff;
				break;
			}
			default: break;
			}

			ImGui::PopID();
		}

		// Push change to managed object
		if (script.m_instance)
			scripting::ScriptEngine::PushCacheToManagedFields(script.m_instance->object, fields);
	}

	void HierarchyInspector::DrawModelComponent(EntityID entity)
	{
		if (!ComponentHeaderWithRemove<ModelComponent>("Model", entity))
			return;

		auto& modelComp = ECS::GetInstance().GetComponent<ModelComponent>(entity);

		// --- Scan available models from ../Resources/Models/ ---
		static std::vector<std::string> availableModels;
		static bool initialized = false;
		static const std::string modelsDir = "../Resources/Models/";

		if (!initialized) {
			availableModels.clear();
			for (auto& entry : std::filesystem::directory_iterator(modelsDir)) {
				if (entry.is_regular_file()) {
					std::string name = entry.path().filename().string();
					std::string ext = entry.path().extension().string();
					// Added .skin and .mesh to the filter
					if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" ||
						ext == ".skin" || ext == ".mesh") {
						availableModels.push_back(name);
					}
				}
			}
			std::sort(availableModels.begin(), availableModels.end());
			initialized = true;
		}

		// --- Current model name ---
		std::string currentName = (modelComp.m_model ? modelComp.m_model->GetName() : "<None>");
		if (currentName.empty()) currentName = "<None>";

		// --- Dropdown menu ---
		static int selectedModel = -1;
		if (modelComp.m_model) {
			auto it = std::find(availableModels.begin(), availableModels.end(), modelComp.m_model->GetName());
			if (it != availableModels.end())
				selectedModel = (int)std::distance(availableModels.begin(), it);
		}

		if (ImGui::BeginCombo("Model File", currentName.c_str())) {
			for (int i = 0; i < (int)availableModels.size(); ++i) {
				bool isSelected = (i == selectedModel);
				if (ImGui::Selectable(availableModels[i].c_str(), isSelected)) {
					selectedModel = i;
					std::string fullPath = modelsDir + availableModels[i];

					// Use LoadModel (loads if missing, returns cached if present)
					auto model = AssetManager::GetInstance().LoadModel(fullPath);
					if (model) {
						modelComp.m_model = model;

						// Auto-attach animator if entity has AnimationComponent
						if (ECS::GetInstance().HasComponent<AnimationComponent>(entity)) {
							auto& animComp = ECS::GetInstance().GetComponent<AnimationComponent>(entity);
							const aiScene* scene = model->GetAssimpScene();
							if (scene && scene->mNumAnimations > 0)
								animComp.m_animator = std::make_shared<graphics::Animator>(model);
							else
								animComp.m_animator.reset();
						}
					}
				}
				if (isSelected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		// --- Info about the model ---
		if (modelComp.m_model) {
			auto& model = modelComp.m_model;
			ImGui::Text("Name: %s", model->GetName().c_str());
			ImGui::Text("Meshes: %d", (int)model->GetMeshes().size());
			ImGui::Text("Bones: %d", model->GetBoneCount());
		}

		// --- Reload Button ---
		if (modelComp.m_model) {
			if (ImGui::Button("Reload Model")) {
				std::string fullPath = modelsDir + modelComp.m_model->GetName();

				// Remove cached version (forces reload from disk)
				auto& manager = AssetManager::GetInstance();
				manager.UnloadModel(fullPath); // remove from cache

				// Load fresh copy
				auto reloaded = manager.LoadModel(fullPath);
				if (reloaded) {
					modelComp.m_model = reloaded;

					// Refresh animator
					if (ECS::GetInstance().HasComponent<AnimationComponent>(entity)) {
						auto& animComp = ECS::GetInstance().GetComponent<AnimationComponent>(entity);
						const aiScene* scene = reloaded->GetAssimpScene();
						if (scene && scene->mNumAnimations > 0)
							animComp.m_animator = std::make_shared<graphics::Animator>(reloaded);
						else
							animComp.m_animator.reset();
					}
				}
			}
		}
	}

	void HierarchyInspector::DrawAnimationComponent(EntityID entity)
	{
		if (!ComponentHeaderWithRemove<AnimationComponent>("Animation", entity))
			return;

		auto& animComp = ECS::GetInstance().GetComponent<AnimationComponent>(entity);

		if (!animComp.m_animator) {
			ImGui::TextUnformatted("No animations available (model has no animations).");
			return;
		}

		auto& animator = animComp.m_animator;
		const auto& clips = animator->GetClips();

		if (!clips.empty()) {
			static int selectedClip = 0;
			std::vector<const char*> names;
			names.reserve(clips.size());
			for (auto& c : clips) names.push_back(c.name.c_str());

			if (ImGui::Combo("Active Clip", &selectedClip, names.data(), (int)names.size())) {
				animator->PlayAnimation(selectedClip, animator->IsLooping());
			}

			if (ImGui::Button("Play")) animator->PlayAnimation(selectedClip, animator->IsLooping());
			ImGui::SameLine();
			if (ImGui::Button("Pause")) animator->PauseAnimation();
			ImGui::SameLine();
			if (ImGui::Button("Resume")) animator->ResumeAnimation();
			ImGui::SameLine();
			if (ImGui::Button("Stop")) animator->StopAnimation();

			if (auto current = animator->GetCurrentClip()) {
				ImGui::Separator();
				ImGui::Text("Current: %s", current->name.c_str());
				ImGui::Text("Duration: %.2fs", current->duration / current->ticksPerSecond);
				ImGui::Text("Ticks: %.2f, TPS: %.2f", current->duration, current->ticksPerSecond);
			}
		}
		else
			ImGui::TextUnformatted("No animation clips found in this model.");

		// Looping toggle (persisted)
		bool looping = animator->IsLooping();
		if (ImGui::Checkbox("Looping", &looping))
			animator->IsLooping() = looping;

		// Reload Animator Button
		if (ImGui::Button("Reload Animation")) {
			auto model = animator->GetModel();
			if (model) {
				const aiScene* scene = model->GetAssimpScene();
				if (scene && scene->mNumAnimations > 0) {
					animComp.m_animator = std::make_shared<graphics::Animator>(model);
					ImGui::TextUnformatted("Animation reloaded successfully.");
				}
				else {
					animComp.m_animator.reset();
					ImGui::TextUnformatted("No animations found in this model.");
				}
			}
		}
	}

	void HierarchyInspector::DrawStateMachineComponent(EntityID entity)
	{
		//if (!ImGui::CollapsingHeader("State Machine", ImGuiTreeNodeFlags_DefaultOpen))
		//	return;

		if (!ComponentHeaderWithRemove<StateMachine>("State Machine", entity))
			return;

		auto& fsmComp = ECS::GetInstance().GetComponent<StateMachine>(entity);

		ImGui::Text("Current Node: %s",
			(fsmComp.m_CurrentScript ? fsmComp.m_CurrentScript->name.c_str() : "(none)"));

		ImGui::Text("Attached Script: %s",
			(fsmComp.m_CurrentScript && fsmComp.m_CurrentScript->isAttached && !fsmComp.m_CurrentScript->scriptClassName.empty())
			? fsmComp.m_CurrentScript->scriptClassName.c_str()
			: "(none)");

		ImGui::Separator();
		if (ImGui::Button("Edit State Machine"))
		{
			auto fsmWindow = editor::EditorGUI::GetWindow<FSMEditorImGUI>();
			if (fsmWindow)
			{
				fsmWindow->SetSelectedEntity(entity);
				editor::EditorGUI::FocusWindow("FSM Editor");
			}
		}
	}

	void HierarchyInspector::DrawParticleEmitterComponent(EntityID entity)
	{
		//if (!ImGui::CollapsingHeader("Particle Emitter", ImGuiTreeNodeFlags_DefaultOpen))
		//	return;

		if (!ComponentHeaderWithRemove<ParticleEmitter>("Particle Emitter", entity))
			return;

		auto& emitter = ECS::GetInstance().GetComponent<ParticleEmitter>(entity);

		auto particleSystem = ECS::GetInstance().GetSystem<ParticleSystem>();
		if (!particleSystem)
		{
			ImGui::Text("Particle System not found.");
			return;
		}

		// count alive particles for this emitter
		int alive = particleSystem->GetParticleCount();

		// Display info
		ImGui::Text("Emitter Active: %s", emitter.active ? "Yes" : "No");
		ImGui::Text("Particles Alive: %d", alive);
	}

	void HierarchyInspector::DrawAddComponentMenu(EntityID entity) {
		if (ImGui::MenuItem("Transform") && !ECS::GetInstance().HasComponent<Transform>(entity)) {
			ECS::GetInstance().AddComponent(entity, Transform());
		}
		if (ImGui::MenuItem("Mesh") && !ECS::GetInstance().HasComponent<Mesh>(entity)) {
			ECS::GetInstance().AddComponent(entity, graphics::GeometryFactory::CreateCube());
			ECS::GetInstance().AddComponent(entity, Material());
		}
		if (ImGui::MenuItem("Material") && !ECS::GetInstance().HasComponent<Material>(entity)) {
			ECS::GetInstance().AddComponent(entity, Material());
		}
		if (ImGui::MenuItem("Light") && !ECS::GetInstance().HasComponent<Light>(entity)) {
			ECS::GetInstance().AddComponent(entity, Light());
		}
		if (ImGui::MenuItem("Physics") && !ECS::GetInstance().HasComponent<PhysicComponent>(entity)) {
			ECS::GetInstance().AddComponent(entity, PhysicComponent());
			ECS::GetInstance().ResyncAllSignaturesFromStorage();
			ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();
		}
		if (ImGui::MenuItem("Audio") && !ECS::GetInstance().HasComponent<AudioComponent>(entity)) {
			ECS::GetInstance().AddComponent(entity, AudioComponent());
		}
		//if (ImGui::MenuItem("Particle") && !ECS::GetInstance().HasComponent<Particle>(entity)) {
		//    ECS::GetInstance().AddComponent(entity, Particle());
		//}
		if (ImGui::MenuItem("Script") && !ECS::GetInstance().HasComponent<Script>(entity)) {
			ECS::GetInstance().AddComponent(entity, Script());
		}
		if (ImGui::MenuItem("Model") && !ECS::GetInstance().HasComponent<ModelComponent>(entity)) {
			ECS::GetInstance().AddComponent(entity, ModelComponent());
		}
		if (ImGui::MenuItem("Animation") && !ECS::GetInstance().HasComponent<AnimationComponent>(entity)) {
			ECS::GetInstance().AddComponent(entity, AnimationComponent());
		}
		if (ImGui::MenuItem("State Machine") && !ECS::GetInstance().HasComponent<StateMachine>(entity)) {
			ECS::GetInstance().AddComponent(entity, StateMachine());

			auto& fsmComp = ECS::GetInstance().GetComponent<StateMachine>(entity);

			if (fsmComp.m_Nodes.empty())
			{
				auto defaultNode = std::make_shared<ScriptNode>();
				defaultNode->id = 0;
				defaultNode->name = "Start";
				defaultNode->isAttached = false;
				defaultNode->scriptClassName = "";
				defaultNode->isStartNode = true;

				fsmComp.m_Nodes.push_back(defaultNode);

				//EE_CORE_INFO("Created default FSM start node for entity {0}", entity);
			}

			// Initialize FSM
			fsmComp.Init(entity);

			//EE_CORE_INFO("StateMachine component added and initialized for entity {0}", entity);
		}
		if (ImGui::MenuItem("ParticleEmitter") && !ECS::GetInstance().HasComponent<ParticleEmitter>(entity)) {
			ECS::GetInstance().AddComponent(entity, ParticleEmitter());
		}
		// Add more component types as needed
	}
} // namespace Ermine::editor