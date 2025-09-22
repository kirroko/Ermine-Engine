#include "PreCompile.h"
#include "InspectorGUI.h"
#include <imgui.h>
#include "MathVector.h"
#include "Components.h"

namespace Ermine
{
    namespace
    {
        // TODO: Move into Quaternion's own file
	    Quaternion Mul(const Quaternion& a, const Quaternion& b)
        {
            return Quaternion(
                a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
                a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
            );
		}

        Quaternion Normalize(const Quaternion& q)
        {
            float len = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
            if (len <= 1e-12f) return {0.f, 0.f, 0.f, 1.f};
            float inv = 1.0f / len;
            return {q.x * inv, q.y * inv, q.z * inv, q.w * inv};
        }

        Quaternion EulerDegToQuaternion(const Vector3D& eulerDeg)
        {
            constexpr float Deg2Rad = 0.017453292519943295769f;

            float rx = eulerDeg.x * Deg2Rad;
            float ry = eulerDeg.y * Deg2Rad;
            float rz = eulerDeg.z * Deg2Rad;

            float cx = std::cos(rx * 0.5f), sx = std::sin(rx * 0.5f);
            float cy = std::cos(ry * 0.5f), sy = std::sin(ry * 0.5f);
            float cz = std::cos(rz * 0.5f), sz = std::sin(rz * 0.5f);

            Quaternion qx(sx, 0.f, 0.f, cx);
            Quaternion qy(0.f, sy, 0.f, cy);
            Quaternion qz(0.f, 0.f, sz, cz);

            Quaternion q = Mul(Mul(qz, qy), qx);
            return Normalize(q);
        }
    }

    InspectorGUI::InspectorGUI()
        : ImGUIWindow("Inspector"), m_entity(EntityID{})
    {
    }

    InspectorGUI::InspectorGUI(EntityID entity, std::string name)
        : ImGUIWindow(name), m_entity(entity)
    {
    }

    void InspectorGUI::SetEntity(EntityID entity)
    {
        m_entity = entity;
    }

    void InspectorGUI::Render()
    {
        //m_Entities (list of entities)

        if (!ImGui::Begin(Name().c_str()))
        {
            ImGui::End();
            return;
        }

        auto& ecs = ECS::GetInstance();
        if (!ecs.IsEntityValid(m_entity))
        {
            ImGui::TextDisabled("No GameObject selected");
            ImGui::End();
            return;
        }

        // Transform Component
        if (ecs.HasComponent<Transform>(m_entity))
        {
            auto& tr = ecs.GetComponent<Transform>(m_entity);

            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::BeginChild("TransformChild", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY);
                //ImGui::BeginGroup();
                // Position
                ImGui::TextUnformatted("Position");
                ImGui::SameLine();
                if (ImGui::SmallButton("Reset##pos")) tr.position = Vector3D(0.f, 0.f, 0.f);
                ImGui::DragFloat3("##pos", &tr.position.x);

                // Rotation
                ImGui::TextUnformatted("Rotation");
                ImGui::SameLine();
                if (ImGui::SmallButton("Reset##rot")) tr.rotation = Quaternion(0.f, 0.f, 0.f, 1.f);

                Vector3D eulerDeg = QuaternionToEuler(tr.rotation);
                
                if (ImGui::DragFloat3("##rot", &eulerDeg.x, 1.0f))
                    tr.rotation = EulerDegToQuaternion(eulerDeg);

                // Scale
                ImGui::TextUnformatted("Scale");
                ImGui::SameLine();


                if (ImGui::SmallButton("Reset##scl")) tr.scale = Vector3D(1.f, 1.f, 1.f);
                if (ImGui::DragFloat3("##scl", &tr.scale.x))
                {
                    constexpr float kMinScale = 0.0001f;
                    tr.scale.x = (tr.scale.x >= 0.f) ? fmaxf(tr.scale.x, kMinScale) : -fmaxf(-tr.scale.x, kMinScale);
                    tr.scale.y = (tr.scale.y >= 0.f) ? fmaxf(tr.scale.y, kMinScale) : -fmaxf(-tr.scale.y, kMinScale);
                    tr.scale.z = (tr.scale.z >= 0.f) ? fmaxf(tr.scale.z, kMinScale) : -fmaxf(-tr.scale.z, kMinScale);
                }

                // Context menu for contents
                if (ImGui::BeginPopupContextWindow("Transform_ContentContext", ImGuiPopupFlags_MouseButtonRight))
                {
                    if (ImGui::MenuItem("Delete", "Del", false))
                    {
                        ecs.RemoveComponent<Transform>(m_entity);
                    }
                    ImGui::EndPopup();
                }

                //ImGui::EndGroup();
                ImGui::EndChild();

            }

            ImGui::Separator();
        }

        if (ecs.HasComponent<Material>(m_entity))
        {
            auto& mt = ecs.GetComponent<Material>(m_entity);

            if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
            {
            }

            ImGui::Separator();
        }

        if (ecs.HasComponent<AudioComponent>(m_entity))
        {
            auto& ac = ecs.GetComponent<AudioComponent>(m_entity);

            if (ImGui::CollapsingHeader("AudioComponent", ImGuiTreeNodeFlags_DefaultOpen))
            {
            }

            ImGui::Separator();
        }

        //ImGui::
        if (ImGui::Button("Add Component"))
        {
			ImGui::OpenPopup("AddComponent");
        }

        ImGui::End();
    }
}
