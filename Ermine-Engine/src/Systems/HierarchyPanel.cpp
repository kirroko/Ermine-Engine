`/* Start Header ************************************************************************/
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
#include "Serialisation.h" // Added for prefab support
#include "Selection.h"

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
        ImGui::Text("Selected: %zu", editor::Selection::All().size());
        ImGui::Separator();

        // Toolbar
        if (ImGui::Button("Create Entity")) {
            EntityID newEntity = m_ActiveScene->CreateEntity("New Entity");
			editor::Selection::SelectSingle(m_ActiveScene, newEntity);
            ImGui::SetWindowFocus("Inspector");
        }
        ImGui::SameLine();

        EntityID primary = editor::Selection::Primary();
        if (primary != 0 && ImGui::Button("Delete Selected")) {
            // Delete all currently selected entities
            auto sel = editor::Selection::All();
            std::vector<EntityID> toDelete(sel.begin(), sel.end());
            for (auto id : toDelete)
            {
                ECS::GetInstance().GetSystem<Physics>()->RemovePhysic(id);
                m_ActiveScene->DestroyEntity(id);
                ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();
            }
            editor::Selection::Clear(m_ActiveScene);
        }

        ImGui::SameLine();
        //ImGui::Checkbox("Show Inactive", &m_ShowInactive);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Show inactive entities (grayed out)");
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

    void HierarchyPanel::DuplicateEntity(EntityID sourceEntity) {
        if (sourceEntity == 0) return;

        auto& ecs = ECS::GetInstance();
        if (!ecs.IsEntityValid(sourceEntity)) return;

        // Store the original transform before duplication
        Transform originalTransform;
        if (ecs.HasComponent<Transform>(sourceEntity)) {
            originalTransform = ecs.GetComponent<Transform>(sourceEntity);
        }

        // Create a temporary prefab path in a system temp directory
        const auto tempPrefabPath = std::filesystem::temp_directory_path() / "temp_duplicate.prefab";

        try {
            // Save source entity as a prefab
            SavePrefabToFile(ecs, sourceEntity, tempPrefabPath);
            
            // Load the prefab which creates our new entity
            EntityID newEntity = LoadPrefabFromFile(ecs, tempPrefabPath);

            // Ensure we maintain the exact transform values
            if (ecs.HasComponent<Transform>(newEntity)) {
                auto& newTransform = ecs.GetComponent<Transform>(newEntity);
                newTransform = originalTransform; // Copy the exact transform
            }

            // Update the name to indicate it's a copy using Unity-style numbering
            if (ecs.HasComponent<ObjectMetaData>(newEntity)) {
                auto& meta = ecs.GetComponent<ObjectMetaData>(newEntity);
                const auto& sourceMeta = ecs.GetComponent<ObjectMetaData>(sourceEntity);
                
                // Get the base name (without any existing numeric suffix)
                std::string baseName = sourceMeta.name;
                size_t parenPos = baseName.find(" (");
                if (parenPos != std::string::npos) {
                    baseName = baseName.substr(0, parenPos);
                }

                // Find the next available number
                int suffix = 1;
                std::string newName;
                bool nameExists;
                do {
                    newName = baseName + " (" + std::to_string(suffix) + ")";
                    nameExists = false;
                    
                    // Check if this name is already taken
                    for (EntityID id = 1; id < MAX_ENTITIES; ++id) {
                        if (id != newEntity && ecs.IsEntityValid(id) && ecs.HasComponent<ObjectMetaData>(id)) {
                            const auto& otherMeta = ecs.GetComponent<ObjectMetaData>(id);
                            if (otherMeta.name == newName) {
                                nameExists = true;
                                break;
                            }
                        }
                    }
                    suffix++;
                } while (nameExists);

                meta.name = newName;
            }

            // Make sure the new entity is added to the scene and selected
            //m_ActiveScene->SetSelectedEntity(newEntity);
			editor::Selection::SelectSingle(m_ActiveScene, newEntity);
            ImGui::SetWindowFocus("Inspector");

            // Clean up the temporary prefab file
            std::filesystem::remove(tempPrefabPath);

            EE_CORE_INFO("Duplicated entity {} to new entity {} using prefab system", sourceEntity, newEntity);
        }
        catch (const std::exception& e) {
            EE_CORE_ERROR("Failed to duplicate entity: {}", e.what());
        }
    }

    void HierarchyPanel::DrawEntityNode(EntityID entity, int depth) {
        if (!ECS::GetInstance().IsEntityValid(entity)) return;

        auto& ecs = ECS::GetInstance();
        auto& metadata = ecs.GetComponent<ObjectMetaData>(entity);
        bool isSelected = editor::Selection::IsSelected(entity);

        // Check if entity is inactive
        bool isInactive = !metadata.selfActive;

        // Push gray color for inactive entities
        bool pushedColor = false;
        if (isInactive) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            pushedColor = true;
        }

        ImGui::PushID((int)entity); // keep this

        // children
        std::vector<EntityID> children;
        if (ecs.HasComponent<HierarchyComponent>(entity)) {
            children = ecs.GetComponent<HierarchyComponent>(entity).children;
        }

        // indent
        float indent = static_cast<float>(depth) * m_indentPadding;
        if (indent > 0) ImGui::Indent(indent);

        // visible name + unique ID suffix
        std::string visible = metadata.name.empty() ? "Entity" : metadata.name;
        std::string label = visible + "##" + std::to_string((uint64_t)entity); // unique ID

        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_SpanAvailWidth
            | ImGuiTreeNodeFlags_FramePadding;

        if (isSelected) nodeFlags |= ImGuiTreeNodeFlags_Selected;
        if (children.empty()) nodeFlags |= ImGuiTreeNodeFlags_Leaf;

        // Auto-expand parent
        if (HasSelectedDescendant(entity))
            ImGui::SetNextItemOpen(true);

        bool nodeOpen = ImGui::TreeNodeEx(label.c_str(), nodeFlags);

        if (isSelected && ImGui::IsWindowAppearing())
            ImGui::SetScrollHereY(0.5f);

        HandleDragDrop(entity);

        if (ImGui::IsItemClicked()) {
            if (ImGui::GetIO().KeyCtrl)
				editor::Selection::Toggle(m_ActiveScene, entity); // Multi-select
            else
				editor::Selection::SelectSingle(m_ActiveScene, entity); // Single select
            m_PendingFocusEntity = editor::Selection::Primary();
        }

        if (ImGui::BeginPopupContextItem(("ctx##" + std::to_string((uint64_t)entity)).c_str())) { // unique popup
            if (ImGui::MenuItem("Delete")) {
                ECS::GetInstance().GetSystem<Physics>()->RemovePhysic(entity);
                m_ActiveScene->DestroyEntity(entity);
                ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Duplicate")) {
                DuplicateEntity(entity);  // Use the right-clicked entity, not the selected one
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (nodeOpen) {
            for (auto child : children) {
                DrawEntityNode(child, depth + 1);
            }
            ImGui::TreePop();
        }

        if (indent > 0) ImGui::Unindent(indent);

        // Pop the color style if we pushed it
        if (pushedColor) {
            ImGui::PopStyleColor();
        }

        ImGui::PopID();
    }


    void HierarchyPanel::HandleDragDrop(EntityID entity) {
        // Drag source code
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("HIERARCHY_ENTITY", &entity, sizeof(EntityID));
            auto& metadata = ECS::GetInstance().GetComponent<ObjectMetaData>(entity);
            ImGui::Text("Moving: %s", metadata.name.c_str());
            ImGui::EndDragDropSource();
        }

        // Drop target code for parenting
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY")) {
                EntityID droppedEntity = *(EntityID*)payload->Data;

                if (droppedEntity != entity) {
                    //auto& parentMeta = ECS::GetInstance().GetComponent<ObjectMetaData>(entity);
                    //auto& childMeta = ECS::GetInstance().GetComponent<ObjectMetaData>(droppedEntity);

                    // ONLY log if both entities are either Cube or Sphere
                    //bool isCubeOrSphere = (parentMeta.name.find("Cube") != std::string::npos ||
                    //    parentMeta.name.find("Sphere") != std::string::npos) &&
                    //    (childMeta.name.find("Cube") != std::string::npos ||
                    //        childMeta.name.find("Sphere") != std::string::npos);

                    //if (isCubeOrSphere) {
                        if (auto hierarchySystem = ECS::GetInstance().GetSystem<HierarchySystem>()) {
                            if (!hierarchySystem->WouldCreateCycle(droppedEntity, entity)) {
                                // Log initial state
                                //const auto& childTransform = ECS::GetInstance().GetComponent<Transform>(droppedEntity);
                                //EE_CORE_INFO("{} (Child) before parenting:", childMeta.name);
                                //EE_CORE_INFO("Position: {},{},{}",
                                //    childTransform.position.x,
                                //    childTransform.position.y,
                                //    childTransform.position.z);

                                // Do the parenting
                                hierarchySystem->SetParent(droppedEntity, entity);
                                hierarchySystem->MarkDirty(entity);
                                hierarchySystem->MarkDirty(droppedEntity);

                                // Log after parenting
                                //const auto& updatedTransform = ECS::GetInstance().GetComponent<Transform>(droppedEntity);
                                //EE_CORE_INFO("{} is now child of {}", childMeta.name, parentMeta.name);
                                //EE_CORE_INFO("New Position: {},{},{}",
                                //    updatedTransform.position.x,
                                //    updatedTransform.position.y,
                                //    updatedTransform.position.z);
                                //EE_CORE_INFO("Is Transform Dirty: {}", updatedTransform.isDirty);
                            }
                        }
                    //}
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
                //m_ActiveScene->SetSelectedEntity(newEntity); // Auto-select the new entity
				editor::Selection::SelectSingle(m_ActiveScene, newEntity);
                ImGui::SetWindowFocus("Inspector");
            }

            if (ImGui::BeginMenu("Create Primitive")) {
                if (ImGui::MenuItem("Cube")) {
                    EntityID entity = m_ActiveScene->CreateEntity("Cube");
                    ECS::GetInstance().AddComponent(entity, graphics::GeometryFactory::CreateCube(1, 1, 1));

                    auto shader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/vertex.glsl", "../Resources/Shaders/fragment.glsl");
                    auto materialPtr = std::make_shared<graphics::Material>(shader);
                    materialPtr->SetVec3("material.albedo", Vec3(1.0f, 1.0f, 1.0f)); // white color

                    ECS::GetInstance().AddComponent(entity, Material(materialPtr));

                    // Log cube creation
                    EE_CORE_INFO("=== Cube Created ===");
                    EE_CORE_INFO("Entity ID: {}", entity);
                    if (ECS::GetInstance().HasComponent<Transform>(entity)) {
                        const auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
                        EE_CORE_INFO("Initial Position: {},{},{}",
                            transform.position.x,
                            transform.position.y,
                            transform.position.z);
                    }
                    EE_CORE_INFO("==================");

                    //m_ActiveScene->SetSelectedEntity(entity);
					editor::Selection::SelectSingle(m_ActiveScene, entity);
                    ImGui::SetWindowFocus("Inspector");
                }
                if (ImGui::MenuItem("Sphere")) {
                    EntityID entity = m_ActiveScene->CreateEntity("Sphere");
                    ECS::GetInstance().AddComponent(entity, graphics::GeometryFactory::CreateSphere(1.0f));

                    auto shader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/vertex.glsl", "../Resources/Shaders/fragment.glsl");
                    auto materialPtr = std::make_shared<graphics::Material>(shader);
                    materialPtr->SetVec3("material.albedo", Vec3(0.8f, 0.8f, 0.8f)); // light grey color

                    ECS::GetInstance().AddComponent(entity, Material(materialPtr));

                    // Log sphere creation
                    EE_CORE_INFO("=== Sphere Created ===");
                    EE_CORE_INFO("Entity ID: {}", entity);
                    if (ECS::GetInstance().HasComponent<Transform>(entity)) {
                        const auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
                        EE_CORE_INFO("Initial Position: {},{},{}",
                            transform.position.x,
                            transform.position.y,
                            transform.position.z);
                    }
                    EE_CORE_INFO("==================");
                    //m_ActiveScene->SetSelectedEntity(entity);
					editor::Selection::SelectSingle(m_ActiveScene, entity);
                    ImGui::SetWindowFocus("Inspector");
                }
                if (ImGui::MenuItem("Cone")) {
                    EntityID entity = m_ActiveScene->CreateEntity("Cone");
                    ECS::GetInstance().AddComponent(entity, graphics::GeometryFactory::CreateCone(1.0f, 2.0f, 32));

                    auto shader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/vertex.glsl", "../Resources/Shaders/fragment.glsl");
                    auto materialPtr = std::make_shared<graphics::Material>(shader);
                    materialPtr->SetVec3("material.albedo", Vec3(0.9f, 0.7f, 0.5f)); // tan/beige color

                    ECS::GetInstance().AddComponent(entity, Material(materialPtr));

                    // Log cone creation
                    EE_CORE_INFO("=== Cone Created ===");
                    EE_CORE_INFO("Entity ID: {}", entity);
                    if (ECS::GetInstance().HasComponent<Transform>(entity)) {
                        const auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);
                        EE_CORE_INFO("Initial Position: {},{},{}",
                            transform.position.x,
                            transform.position.y,
                            transform.position.z);
                    }
                    EE_CORE_INFO("==================");
                    //m_ActiveScene->SetSelectedEntity(entity);
					editor::Selection::SelectSingle(m_ActiveScene, entity);
                    ImGui::SetWindowFocus("Inspector");
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Create Light")) {
                EntityID entity = m_ActiveScene->CreateEntity("Light");
                ECS::GetInstance().AddComponent(entity, Light());
                /*m_ActiveScene->SetSelectedEntity(entity); */
                editor::Selection::SelectSingle(m_ActiveScene, entity);
                ImGui::SetWindowFocus("Inspector"); 
            }


            ImGui::Separator();

			EntityID primary = editor::Selection::Primary();
            if (primary != 0)
            {
				auto hs = ECS::GetInstance().GetSystem<HierarchySystem>();
                if (hs->GetParent(primary) != 0)
                {
	                if (ImGui::MenuItem("Unparent Selected"))
                        for (auto id : editor::Selection::All())
                        {
                            hs->UnsetParent(id);
                            EE_CORE_INFO("Unparented entity {}", id);
                        }
                }

                if (ImGui::MenuItem("Delete Selected"))
                {
	                auto sel = editor::Selection::All();
                    for (auto id : sel)
                    {
                        m_ActiveScene->DestroyEntity(id);
                    }
					editor::Selection::Clear(m_ActiveScene);
					ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();
                }
            }
            //EntityID selected = m_ActiveScene->GetSelectedEntity();
            //if (selected != 0) {
            //    // Check if selected entity has a parent
            //    auto hierarchySystem = ECS::GetInstance().GetSystem<HierarchySystem>();
            //    if (hierarchySystem->GetParent(selected) != 0) {
            //        if (ImGui::MenuItem("Unparent Selected")) {
            //            hierarchySystem->UnsetParent(selected);
            //            EE_CORE_INFO("Unparented entity {}", selected);
            //        }
            //    }

            //    if (ImGui::MenuItem("Delete Selected")) {
            //        m_ActiveScene->DestroyEntity(selected);
            //        ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();
            //    }
            //}

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

    bool HierarchyPanel::HasSelectedDescendant(EntityID entity) const
    {
        if (editor::Selection::IsSelected(entity))
            return true;

        auto& ecs = ECS::GetInstance();
        if (!ecs.HasComponent<HierarchyComponent>(entity))
            return false;

		const auto& hierarchy = ecs.GetComponent<HierarchyComponent>(entity);
        for (auto child : hierarchy.children)
        {
            if (HasSelectedDescendant(child))
                return true;
        }
		return false;
    }
}
