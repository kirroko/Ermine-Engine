
#include "PreCompile.h"
#include "HierarchyPanel.h"
#include "Components.h"
#include "HierarchySystem.h"
#include "ECS.h"

namespace Ermine {
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
            m_ActiveScene->CreateEntity("New Entity");
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
            DrawEntityNode(entity);
        }

        // Right-click context menu
        DrawContextMenu();

        ImGui::End();
    }

    void HierarchyPanel::DrawEntityNode(EntityID entity) {
        if (!ECS::GetInstance().IsEntityValid(entity)) return;
        if (!ECS::GetInstance().HasComponent<HierarchyComponent>(entity)) return;

        auto& metadata = ECS::GetInstance().GetComponent<ObjectMetaData>(entity);
        auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);

        // Determine tree node flags
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (hierarchy.children.empty()) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (m_ActiveScene->GetSelectedEntity() == entity) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        // Create label with icon
        const char* icon = GetEntityIcon(entity);
        std::string label = std::string(icon) + " " + metadata.name;

        // Draw tree node
        bool nodeOpen = ImGui::TreeNodeEx((void*)(uint64_t)entity, flags, "%s", label.c_str());

        // Handle selection
        if (ImGui::IsItemClicked()) {
            m_ActiveScene->SetSelectedEntity(entity);
        }

        // Handle drag and drop
        HandleDragDrop(entity);

        // Draw children if node is open
        if (nodeOpen) {
            for (auto child : hierarchy.children) {
                DrawEntityNode(child);
            }
            ImGui::TreePop();
        }
    }

    void HierarchyPanel::HandleDragDrop(EntityID entity) {
        // Drag source
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("HIERARCHY_ENTITY", &entity, sizeof(EntityID));

            auto& metadata = ECS::GetInstance().GetComponent<ObjectMetaData>(entity);
            ImGui::Text("Moving: %s", metadata.name.c_str());

            ImGui::EndDragDropSource();
        }

        // Drop target
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY")) {
                EntityID droppedEntity = *(EntityID*)payload->Data;

                if (droppedEntity != entity) {
                    auto hierarchySystem = ECS::GetInstance().GetSystem<HierarchySystem>();

                    // Check for cycles before setting parent
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

    void HierarchyPanel::DrawContextMenu() {
        if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
            if (ImGui::MenuItem("Create Empty Entity")) {
                m_ActiveScene->CreateEntity("Empty Entity");
            }

            if (ImGui::BeginMenu("Create Primitive")) {
                if (ImGui::MenuItem("Cube")) {
                    EntityID entity = m_ActiveScene->CreateEntity("Cube");
                    // Add cube mesh components here if needed
                }
                if (ImGui::MenuItem("Sphere")) {
                    EntityID entity = m_ActiveScene->CreateEntity("Sphere");
                    // Add sphere mesh components here if needed
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Create Light")) {
                EntityID entity = m_ActiveScene->CreateEntity("Light");
                ECS::GetInstance().AddComponent(entity, Light());
            }

            ImGui::Separator();

            EntityID selected = m_ActiveScene->GetSelectedEntity();
            if (selected != 0) {
                if (ImGui::MenuItem("Delete Selected")) {
                    m_ActiveScene->DestroyEntity(selected);
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
        // Return appropriate icon based on components
        if (ECS::GetInstance().HasComponent<Light>(entity)) {
            return "💡";
        }
        if (ECS::GetInstance().HasComponent<AudioComponent>(entity)) {
            return "🔊";
        }
        if (ECS::GetInstance().HasComponent<Mesh>(entity)) {
            return "📦";
        }
        return "⚪"; // Default entity icon
    }
}