#include "PreCompile.h"
#include "InspectorGUI.h"
#include <imgui.h>
#include "MathVector.h"
#include "Components.h"

namespace Ermine
{
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
                if (ImGui::SmallButton("Reset##rot")) tr.rotation = Vector3D(0.f, 0.f, 0.f);
                ImGui::DragFloat3("##rot", &tr.rotation.x);

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
