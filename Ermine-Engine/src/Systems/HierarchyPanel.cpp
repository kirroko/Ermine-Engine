/* Start Header ************************************************************************/
/*!
\file       HierarchyPanel.cpp
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee\@digipen.edu
\date       Sep 05, 2025
\brief      Hierarchy panel UI for scene entity tree management with drag-and-drop
            parenting support.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "HierarchyPanel.h"
#include "Components.h"
#include "HierarchySystem.h"
#include "ECS.h"
#include "GeometryFactory.h"
#include "AssetManager.h"
#include "Physics.h"

namespace Ermine {
    void HierarchyPanel::SetScene(Scene* scene) {
        m_ActiveScene = scene;
        EE_CORE_INFO("HierarchyPanel::SetScene called with scene: {}", scene ? scene->GetName() : "null");
    }

    Scene* HierarchyPanel::GetScene() const {
        return m_ActiveScene;
    }

    void HierarchyPanel::OnImGuiRender() {
        if (!m_IsVisible) return;
        ImGui::Begin("Scene Hierarchy", &m_IsVisible);

        if (!m_ActiveScene) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "No active scene");
            ImGui::End();
            return;
        }

        // Scene header
        ImGui::Text("Scene: %s", m_ActiveScene->GetName().c_str());
        ImGui::Text("Entities: %zu", m_ActiveScene->GetEntityCount());
        ImGui::Separator();

        // Toolbar
        if (ImGui::Button("Create Entity")) {
            EntityID newEntity = m_ActiveScene->CreateEntity("New Entity");
            m_ActiveScene->SetSelectedEntity(newEntity);
            ImGui::SetWindowFocus("Inspector");
        }
        ImGui::SameLine();

        EntityID selected = m_ActiveScene->GetSelectedEntity();
        if (selected != 0) {
            if (ImGui::Button("Delete Selected")) {
                m_ActiveScene->DestroyEntity(selected);
            }
        }

        ImGui::Separator();

        // Entity hierarchy
        auto rootEntities = m_ActiveScene->GetRootEntities();
        for (auto entity : rootEntities) {
            DrawEntityNode(entity, 0);
        }

        // Add invisible button to catch drops on empty space
        ImVec2 space = ImGui::GetContentRegionAvail();
		if (space.x != 0.0f && space.y != 0.0f)
			ImGui::InvisibleButton("UnparentDropZone", ImGui::GetContentRegionAvail());
        HandleUnparentDrop();

        // Right-click context menu
        DrawContextMenu();

        // Handle delayed inspector focus - wait for mouse release
            if (m_PendingFocusEntity != 0) {
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    // Mouse released - check if it was a drag or just a click
                    if (!ImGui::GetDragDropPayload()) {
                        ImGui::SetWindowFocus("Inspector");
                    }
                    m_PendingFocusEntity = 0; // Reset
                }
            }

        ImGui::End();
    }

    void HierarchyPanel::DrawEntityNode(EntityID entity, int depth) {
        if (!ECS::GetInstance().IsEntityValid(entity)) return;

        auto& metadata = ECS::GetInstance().GetComponent<ObjectMetaData>(entity);
        bool isSelected = m_ActiveScene->IsEntitySelected(entity);

        ImGui::PushID(static_cast<int>(entity));

        // Get children
        std::vector<EntityID> children;
        bool hasChildren = false;
        if (ECS::GetInstance().HasComponent<HierarchyComponent>(entity)) {
            const auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
            children = hierarchy.children;
            hasChildren = !children.empty();
        }

        // Add indentation for depth (16 pixels per level)
        float indent = depth * 16.0f;
        if (indent > 0) ImGui::Indent(indent);

        // Simple display name without symbols
        std::string displayName = metadata.name;
        if (hasChildren) {
            displayName += " (" + std::to_string(children.size()) + " children)";
        }

        // Tree node flags
        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_FramePadding;

        if (isSelected) {
            nodeFlags |= ImGuiTreeNodeFlags_Selected;
        }
        if (!hasChildren) {
            nodeFlags |= ImGuiTreeNodeFlags_Leaf;
        }

        bool nodeOpen = ImGui::TreeNodeEx(displayName.c_str(), nodeFlags);

        // Handle interaction
        HandleDragDrop(entity);
        if (ImGui::IsItemClicked()) {
            m_ActiveScene->SetSelectedEntity(entity);
            m_PendingFocusEntity = entity;
        }

        // Context menu
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete")) {
                m_ActiveScene->DestroyEntity(entity);
                ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();
                ImGui::CloseCurrentPopup();
            }
            //if (ImGui::MenuItem("Duplicate Selected")) {
            //    EntityID selectedEntity = m_ActiveScene->GetSelectedEntity();
            //    if (selectedEntity != 0) {
            //        // Clone the entity and add it to the scene
            //        EntityID newEntity = ECS::GetInstance().CloneEntity(selectedEntity);

            //        // Optionally, set a new name for the duplicated entity
            //        auto& meta = ECS::GetInstance().GetComponent<ObjectMetaData>(newEntity);
            //        meta.name += " (Copy)";

            //        // Add the new entity to the scene and select it
            //        m_ActiveScene->SetSelectedEntity(newEntity);
            //        ImGui::SetWindowFocus("Inspector");
            //    }
            //}
            ImGui::EndPopup();
        }

        // Draw children
        if (nodeOpen) {
            for (auto child : children) {
                DrawEntityNode(child, depth + 1);
            }
            ImGui::TreePop();
        }

        if (indent > 0) ImGui::Unindent(indent);
        ImGui::PopID();
    }

    void HierarchyPanel::HandleDragDrop(EntityID entity) {
        // Existing drag source code
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("HIERARCHY_ENTITY", &entity, sizeof(EntityID));
            auto& metadata = ECS::GetInstance().GetComponent<ObjectMetaData>(entity);
            ImGui::Text("Moving: %s", metadata.name.c_str());
            ImGui::EndDragDropSource();
        }

        // Existing drop target code for parenting
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY")) {
                EntityID droppedEntity = *(EntityID*)payload->Data;

                if (droppedEntity != entity) {
                    auto hierarchySystem = ECS::GetInstance().GetSystem<HierarchySystem>();

                    if (!hierarchySystem->WouldCreateCycle(droppedEntity, entity)) {
                        hierarchySystem->SetParent(droppedEntity, entity);
                        EE_CORE_INFO("Reparented entity {} to {}", droppedEntity, entity);
                    }
                    else {
                        EE_CORE_WARN("Cannot parent entity {} to {} - would create cycle", droppedEntity, entity);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    void HierarchyPanel::HandleUnparentDrop() {
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY")) {
                EntityID droppedEntity = *(EntityID*)payload->Data;

                auto hierarchySystem = ECS::GetInstance().GetSystem<HierarchySystem>();

                // Check if entity currently has a parent
                EntityID currentParent = hierarchySystem->GetParent(droppedEntity);
                if (currentParent != 0) {
                    hierarchySystem->UnsetParent(droppedEntity);
                    EE_CORE_INFO("Unparented entity {} - now a root entity", droppedEntity);
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    void HierarchyPanel::DrawContextMenu() {
        if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
            if (ImGui::MenuItem("Create Empty Entity")) {
                EntityID newEntity = m_ActiveScene->CreateEntity("Empty Entity");
                m_ActiveScene->SetSelectedEntity(newEntity); // Auto-select the new entity
                ImGui::SetWindowFocus("Inspector"); // ADD THIS LINE
            }

            if (ImGui::BeginMenu("Create Primitive")) {
                if (ImGui::MenuItem("Cube")) {
                    EntityID entity = m_ActiveScene->CreateEntity("Cube");
                    ECS::GetInstance().AddComponent(entity, graphics::GeometryFactory::CreateCube(1, 1, 1));

                    auto shader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/vertex.glsl", "../Resources/Shaders/fragment.glsl");
                    auto materialPtr = std::make_shared<graphics::Material>(shader);
                    materialPtr->SetVec3("material.albedo", Vec3(1.0f, 1.0f, 1.0f)); // white color

                    ECS::GetInstance().AddComponent(entity, Material(materialPtr));
                    m_ActiveScene->SetSelectedEntity(entity);
                    ImGui::SetWindowFocus("Inspector");
                }
                if (ImGui::MenuItem("Sphere")) {
                    EntityID entity = m_ActiveScene->CreateEntity("Sphere");
                    ECS::GetInstance().AddComponent(entity, graphics::GeometryFactory::CreateSphere(1.0f));

                    auto shader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/vertex.glsl", "../Resources/Shaders/fragment.glsl");
                    auto materialPtr = std::make_shared<graphics::Material>(shader);
                    materialPtr->SetVec3("material.albedo", Vec3(0.8f, 0.8f, 0.8f)); // light grey color

                    ECS::GetInstance().AddComponent(entity, Material(materialPtr));
                    m_ActiveScene->SetSelectedEntity(entity);
                    ImGui::SetWindowFocus("Inspector");
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Create Light")) {
                EntityID entity = m_ActiveScene->CreateEntity("Light");
                ECS::GetInstance().AddComponent(entity, Light());
                m_ActiveScene->SetSelectedEntity(entity); // ADD THIS LINE
                ImGui::SetWindowFocus("Inspector"); // ADD THIS LINE
            }


            ImGui::Separator();

            EntityID selected = m_ActiveScene->GetSelectedEntity();
            if (selected != 0) {
                // Check if selected entity has a parent
                auto hierarchySystem = ECS::GetInstance().GetSystem<HierarchySystem>();
                if (hierarchySystem->GetParent(selected) != 0) {
                    if (ImGui::MenuItem("Unparent Selected")) {
                        hierarchySystem->UnsetParent(selected);
                        EE_CORE_INFO("Unparented entity {}", selected);
                    }
                }

                if (ImGui::MenuItem("Delete Selected")) {
                    m_ActiveScene->DestroyEntity(selected);
                    ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();
                }

                if (ImGui::MenuItem("Duplicate Selected")) {
                    // TODO: Implement entity duplication
                    EE_CORE_INFO("Duplicate functionality not yet implemented");
                }
            }

            ImGui::EndPopup();
        }
    }

    const char* HierarchyPanel::GetEntityIcon(EntityID entity) const {
        // Return simple text prefixes instead of symbols
        if (ECS::GetInstance().HasComponent<Light>(entity)) {
            return "[Light] ";
        }
        if (ECS::GetInstance().HasComponent<AudioComponent>(entity)) {
            return "[Audio] ";
        }
        if (ECS::GetInstance().HasComponent<Mesh>(entity)) {
            return "[Mesh] ";
        }
        return ""; // No prefix for basic entities
    }
}