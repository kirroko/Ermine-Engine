/* Start Header ************************************************************************/
/*!
\file       HierarchyInspector.cpp
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu
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
#include "HierarchySystem.h"
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
        if (ECS::GetInstance().HasComponent<ObjectMetaData>(entity)) {
            auto& metadata = ECS::GetInstance().GetComponent<ObjectMetaData>(entity);

            // Entity name
            char nameBuf[256];
            strcpy_s(nameBuf, metadata.name.c_str());
            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
                metadata.name = nameBuf;
            }

            // Entity tag
            char tagBuf[256];
            strcpy_s(tagBuf, metadata.tag.c_str());
            if (ImGui::InputText("Tag", tagBuf, sizeof(tagBuf))) {
                metadata.tag = tagBuf;
            }

            // Active checkbox
            ImGui::Checkbox("Active", &metadata.selfActive);

            // Entity ID (read-only)
            ImGui::Text("Entity ID: %u", entity);

            ImGui::Separator();
        }
        else {
            ImGui::Text("Entity ID: %u", entity);
            ImGui::Text("Missing ObjectMetaData component");
            ImGui::Separator();
        }
    }

    void HierarchyInspector::DrawTransformComponent(EntityID entity) {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
            bool transformChanged = false;

            // Position
            float position[3] = { transform.position.x, transform.position.y, transform.position.z };
            if (ImGui::DragFloat3("Position", position, 0.1f)) {
                transform.position = Vec3(position[0], position[1], position[2]);
                transformChanged = true;
            }

            // Convert quaternion to Euler angles for display (in degrees)
            Vec3 eulerAngles = QuaternionToEuler(transform.rotation, true); // true = degrees
            float rotation[3] = { eulerAngles.x, eulerAngles.y, eulerAngles.z };
            if (ImGui::DragFloat3("Rotation (Degrees)", rotation, 1.0f)) {
                // Convert degrees to radians and create quaternion from Euler angles
                float radX = rotation[0] * M_PI / 180.0f;
                float radY = rotation[1] * M_PI / 180.0f;
                float radZ = rotation[2] * M_PI / 180.0f;

                // Create rotation matrices for each axis
                Matrix4x4 rotX, rotY, rotZ, combined;
                Mtx44Identity(rotX);
                Mtx44Identity(rotY);
                Mtx44Identity(rotZ);

                Mtx44RotXRad(rotX, radX);
                Mtx44RotYRad(rotY, radY);
                Mtx44RotZRad(rotZ, radZ);

                // Combine rotations (order: Z * Y * X)
                combined = rotZ * rotY * rotX;

                // Convert back to quaternion
                transform.rotation = Mtx44GetQuaternion(combined);
                transformChanged = true;
            }

            // Scale
            float scale[3] = { transform.scale.x, transform.scale.y, transform.scale.z };
            if (ImGui::DragFloat3("Scale", scale, 0.1f, 0.1f, 10.0f)) {
                transform.scale = Vec3(scale[0], scale[1], scale[2]);
                transformChanged = true;
            }

            // Mark transform as dirty if any changes occurred
            if (transformChanged) {
                auto hierarchySystem = ECS::GetInstance().GetSystem<HierarchySystem>();
                if (hierarchySystem) {
                    hierarchySystem->OnTransformChanged(entity);
                }
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