/* Start Header ************************************************************************/
/*!
\file       HierarchyInspector.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       27/03/2025
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

namespace Ermine::editor {

void HierarchyInspector::OnImGuiRender() {
    if (!m_IsVisible) return;

    ImGui::Begin("Inspector");

    if (!m_ActiveScene) {
        ImGui::Text("No active scene");
        ImGui::End();
        return;
    }

    EntityID selected = m_ActiveScene->GetSelectedEntity();
    if (selected == 0) {
        ImGui::Text("No entity selected");
        ImGui::End();
        return;
    }

    // Entity header
    if (ECS::GetInstance().HasComponent<ObjectMetaData>(selected)) {
        auto& metadata = ECS::GetInstance().GetComponent<ObjectMetaData>(selected);
        char nameBuf[256];
        strcpy_s(nameBuf, metadata.name.c_str());
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
            metadata.name = nameBuf;
        }
        char tagBuf[256];
        strcpy_s(tagBuf, metadata.tag.c_str());
        if (ImGui::InputText("Tag", tagBuf, sizeof(tagBuf))) {
            metadata.tag = tagBuf;
        }
        ImGui::Checkbox("Active", &metadata.selfActive);
        ImGui::Separator();
    }

    // Draw components
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

void HierarchyInspector::DrawTransformComponent(EntityID entity) {
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
        
        float position[3] = { transform.position.x, transform.position.y, transform.position.z };
        if (ImGui::DragFloat3("Position", position, 0.1f)) {
            transform.position = Vec3(position[0], position[1], position[2]);
        }

        float rotation[3] = { transform.rotation.x, transform.rotation.y, transform.rotation.z };
        if (ImGui::DragFloat3("Rotation", rotation, 1.0f)) {
            transform.rotation = Vec3(rotation[0], rotation[1], rotation[2]);
        }

        float scale[3] = { transform.scale.x, transform.scale.y, transform.scale.z };
        if (ImGui::DragFloat3("Scale", scale, 0.1f, 0.1f, 10.0f)) {
            transform.scale = Vec3(scale[0], scale[1], scale[2]);
        }
    }
}

void HierarchyInspector::DrawMeshComponent(EntityID entity) {
    if (ImGui::CollapsingHeader("Mesh")) {
        auto& mesh = ECS::GetInstance().GetComponent<Mesh>(entity);
        ImGui::Text("Vertex Count: %d", mesh.vertex_array ? mesh.vertex_array->GetVertexCount() : 0);
        ImGui::Text("Index Count: %d", mesh.index_buffer ? mesh.index_buffer->GetCount() : 0);
    }
}

void HierarchyInspector::DrawMaterialComponent(EntityID entity) {
    if (ImGui::CollapsingHeader("Material")) {
        auto& material = ECS::GetInstance().GetComponent<Material>(entity);
        if (material.GetMaterial()) {
            auto& materialData = material.GetMaterial()->GetUBOData();
            float albedo[3] = { materialData.albedo.x, materialData.albedo.y, materialData.albedo.z };
            if (ImGui::ColorEdit3("Albedo", albedo)) {
                material.SetAlbedo(Vec3(albedo[0], albedo[1], albedo[2]));
            }
            
            float metallic = materialData.metallic;
            if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f)) {
                material.GetMaterial()->SetFloat("material.metallic", metallic);
            }

            float roughness = materialData.roughness;
            if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f)) {
                material.GetMaterial()->SetFloat("material.roughness", roughness);
            }
        }
    }
}

void HierarchyInspector::DrawLightComponent(EntityID entity) {
    if (ImGui::CollapsingHeader("Light")) {
        auto& light = ECS::GetInstance().GetComponent<Light>(entity);
        
        float color[3] = { light.color.x, light.color.y, light.color.z };
        if (ImGui::ColorEdit3("Color", color)) {
            light.color = Vec3(color[0], color[1], color[2]);
        }

        ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 10.0f);
        
        const char* lightTypes[] = { "Point", "Directional", "Spot" };
        int currentType = static_cast<int>(light.type);
        if (ImGui::Combo("Type", &currentType, lightTypes, IM_ARRAYSIZE(lightTypes))) {
            light.type = static_cast<LightType>(currentType);
        }
    }
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

void HierarchyInspector::DrawAddComponentMenu(EntityID entity) {
    if (ImGui::MenuItem("Transform") && !ECS::GetInstance().HasComponent<Transform>(entity)) {
        ECS::GetInstance().AddComponent(entity, Transform());
    }
    if (ImGui::MenuItem("Mesh") && !ECS::GetInstance().HasComponent<Mesh>(entity)) {
        ECS::GetInstance().AddComponent(entity, graphics::GeometryFactory::CreateCube());
    }
    if (ImGui::MenuItem("Light") && !ECS::GetInstance().HasComponent<Light>(entity)) {
        ECS::GetInstance().AddComponent(entity, Light());
    }
    // Add more component types as needed
}

} // namespace Ermine::editor