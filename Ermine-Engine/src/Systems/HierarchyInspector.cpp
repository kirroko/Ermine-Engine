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
#include "Physics.h"

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

        if (ECS::GetInstance().HasComponent<PhysicComponent>(selected)) {
            DrawPhysicsComponent(selected);
        }

        if (ECS::GetInstance().HasComponent<AudioComponent>(selected)) {
            DrawAudioComponent(selected);
        }

        if (ECS::GetInstance().HasComponent<Script>(selected)) {
            DrawScriptComponent(selected);
        }

        //if (ECS::GetInstance().HasComponent<Particle>(selected)) {
        //    DrawParticleComponent(selected);
        //}


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
        
            float position[3] = { transform.position.x, transform.position.y, transform.position.z };
            if (ImGui::DragFloat3("Position", position, 0.1f)) {
                transform.position = Vec3(position[0], position[1], position[2]);
                transform.isDirty = true; // Mark as dirty when modified
                
                // Also mark the hierarchy as dirty if this entity has hierarchy component
                if (ECS::GetInstance().HasComponent<HierarchyComponent>(entity)) {
                    auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
                    hierarchy.isDirty = true;
                }
            }

            // Convert quaternion to Euler angles for display (in degrees)
            Vec3 eulerAngles = QuaternionToEuler(transform.rotation, true); // true = degrees
            float rotation[3] = { eulerAngles.x, eulerAngles.y, eulerAngles.z };
            if (ImGui::DragFloat3("Rotation (Degrees)", rotation, 1.0f)) {
                // Convert degrees to radians and create quaternion from Euler angles
                constexpr float PI_F = 3.14159265358979323846f;
                float radX = rotation[0] * PI_F / 180.0f;
                float radY = rotation[1] * PI_F / 180.0f;
                float radZ = rotation[2] * PI_F / 180.0f;

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
                transform.isDirty = true; // Mark as dirty when modified
                
                // Also mark the hierarchy as dirty if this entity has hierarchy component
                if (ECS::GetInstance().HasComponent<HierarchyComponent>(entity)) {
                    auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
                    hierarchy.isDirty = true;
                }
            }

            float scale[3] = { transform.scale.x, transform.scale.y, transform.scale.z };
            if (ImGui::DragFloat3("Scale", scale, 0.1f, 0.1f, 10.0f)) {
                transform.scale = Vec3(scale[0], scale[1], scale[2]);
                transform.isDirty = true; // Mark as dirty when modified
                
                // Also mark the hierarchy as dirty if this entity has hierarchy component
                if (ECS::GetInstance().HasComponent<HierarchyComponent>(entity)) {
                    auto& hierarchy = ECS::GetInstance().GetComponent<HierarchyComponent>(entity);
                    hierarchy.isDirty = true;
                }
            }
        }
    }

    void HierarchyInspector::DrawMeshComponent(EntityID entity) {
        if (!ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        auto& mesh = ECS::GetInstance().GetComponent<Mesh>(entity);

        // Combo for mesh kind
        const char* kinds[] = { "None", "Primitive", "Asset" };
        int currentKind = static_cast<int>(mesh.kind);
        if (ImGui::Combo("Kind", &currentKind, kinds, IM_ARRAYSIZE(kinds))) {
            mesh.kind = static_cast<Mesh::Kind>(currentKind);
            if (mesh.kind == Mesh::Kind::Primitive)
                mesh.RebuildPrimitive();
        }

        // Primitive controls
        if (mesh.kind == Mesh::Kind::Primitive) {
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
        if (mesh.kind == Mesh::Kind::Asset) {
            char buf[256];
            strcpy_s(buf, mesh.asset.meshName.c_str());
            if (ImGui::InputText("Mesh Name", buf, sizeof(buf))) {
                mesh.asset.meshName = buf;
                // TODO: trigger asset reload here
            }
        }
    }

    void HierarchyInspector::DrawMaterialComponent(EntityID entity) {
        if (!ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        // --- Fetch component & underlying material safely ---
        auto& matComp = ECS::GetInstance().GetComponent<Material>(entity);
        graphics::Material* gm = matComp.GetMaterial();

        if (!gm) {
            ImGui::TextUnformatted("No material bound.");
            if (ImGui::Button("Create Default PBR")) {
                matComp = Material(std::make_shared<graphics::Material>());
                gm = matComp.GetMaterial();
                if (gm) {
                    // Seed defaults on both alias keys so future reads succeed
                    gm->SetVec3("materialAlbedo", { 1.f, 1.f, 1.f }); gm->SetVec3("material.albedo", { 1.f, 1.f, 1.f });
                    gm->SetFloat("materialMetallic", 0.0f);           gm->SetFloat("material.metallic", 0.0f);
                    gm->SetFloat("materialRoughness", 0.5f);           gm->SetFloat("material.roughness", 0.5f);
                    gm->SetVec3("materialEmissive", { 0.f, 0.f, 0.f }); gm->SetVec3("material.emissive", { 0.f, 0.f, 0.f });
                    gm->SetFloat("materialEmissiveIntensity", 1.0f);     gm->SetFloat("material.emissiveIntensity", 1.0f);
                }
            }
            ImGui::Separator();
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

        // --- Albedo ---
        {
            Vec3 v = getVec3("materialAlbedo", "material.albedo", matComp.cacheAlbedo);
            float col[3] = { v.x, v.y, v.z };
            if (ImGui::ColorEdit3("Albedo", col)) {
                v = { col[0], col[1], col[2] };
                matComp.hasAlbedo = true;
                matComp.cacheAlbedo = v;
                setVec3Both("materialAlbedo", "material.albedo", v);
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

        // --- Safe reset button (no null strings involved) ---
        if (ImGui::SmallButton("Reset to Defaults")) {
            Vec3 alb{ 1.f,1.f,1.f }; setVec3Both("materialAlbedo", "material.albedo", alb);
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
            matComp.cacheAlbedo = alb;
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

    void HierarchyInspector::DrawPhysicsComponent(EntityID entity)
    {
        auto& pc = ECS::GetInstance().GetComponent<PhysicComponent>(entity);

        if (ImGui::CollapsingHeader("PhysicComponent", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const char* PhysicsBody[] = {"Rigidbody", "Trigger"};

            if (ImGui::BeginCombo("Physics Body Type", PhysicsBody[(int)pc.bodyType]))
            {
                for (int n = 0; n < 2; n++)
                {
                    const bool is_selected = (pc.bodyType == (PhysicsBodyType)n);
                    if (ImGui::Selectable(PhysicsBody[n], is_selected))
                    {
                        pc.bodyType = (PhysicsBodyType)n;
                        ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();
                    }

                    //Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            const char* EmotionType[] = {"Static", "Kinematic", "Dynamic"};

            if (ImGui::BeginCombo("Emotion Type", EmotionType[(int)pc.motionType]))
            {
                for (int n = 0; n < 3; n++)
                {
                    const bool is_selected = (pc.motionType == (JPH::EMotionType)n);
                    if (ImGui::Selectable(EmotionType[n], is_selected))
                    {
                        pc.motionType = (JPH::EMotionType)n;
                        ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();
                    }

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (ImGui::DragFloat("##mas", &pc.mass))
            {
                constexpr float kMinScale = 0.f;
                pc.mass = fmaxf(pc.mass, kMinScale);
                ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();
            }

            const char* ShapeTypeList[] = { "Box", "Sphere", "Capsule", "CustomMesh" };

            if (ImGui::BeginCombo("Shape Type", ShapeTypeList[(int)pc.shapeType]))
            {
                for (int n = 0; n < (int)ShapeType::Total; n++)
                {
                    const bool is_selected = (pc.shapeType == (ShapeType)n);
                    if (ImGui::Selectable(ShapeTypeList[n], is_selected))
                    {
                        pc.shapeType = (ShapeType)n;
                        ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();
                    }

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

            ImGui::Separator();
    }

    void HierarchyInspector::DrawAudioComponent(EntityID entity)
    {
        if (!ImGui::CollapsingHeader("Audio", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        auto& audio = ECS::GetInstance().GetComponent<AudioComponent>(entity);

        // Sound name input
        char buf[256];
        strcpy_s(buf, audio.soundName.c_str());
        if (ImGui::InputText("Sound", buf, sizeof(buf))) {
            audio.soundName = buf;
            // TODO: hook into AssetManager to actually load the sound
        }

        // Volume
        if (ImGui::SliderFloat("Volume", &audio.volume, 0.0f, 1.0f)) {
            // If you have FMOD instance, update volume immediately
            // audio.UpdateVolume();
        }

        // Toggles
        ImGui::Checkbox("Looping", &audio.isLooping);
        ImGui::Checkbox("Play On Awake", &audio.isPlaying);
        ImGui::Checkbox("Spatial", &audio.is3D);

        ImGui::Separator();

        // Preview controls (optional)
        if (ImGui::Button("Play")) {
            // TODO: hook into your FMOD wrapper: AudioSystem::Get().Play(audio.soundName, entity);
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            // TODO: AudioSystem::Get().Stop(entity);
        }
    }

    /*void HierarchyInspector::DrawParticleComponent(EntityID entity)
    {
        if (!ImGui::CollapsingHeader("Particle")) return;

        auto& particle = ECS::GetInstance().GetComponent<Particle>(entity);

        float vel[3] = { particle.velocity.x, particle.velocity.y, particle.velocity.z };
        if (ImGui::DragFloat3("Velocity", vel, 0.1f)) {
            particle.velocity = Vec3(vel[0], vel[1], vel[2]);
        }

        ImGui::DragFloat("Lifetime", &particle.lifetime, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Age", &particle.age, 0.1f, 0.0f, particle.lifetime);

        float col[4] = { particle.colour.x, particle.colour.y, particle.colour.z, particle.colour.w };
        if (ImGui::ColorEdit4("Colour", col)) {
            particle.colour = Vec4(col[0], col[1], col[2], col[3]);
        }

        ImGui::DragFloat("Size", &particle.size, 0.1f, 0.01f, 100.0f);
    }*/

    void HierarchyInspector::DrawScriptComponent(EntityID entity)
    {
        if (!ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        auto& script = ECS::GetInstance().GetComponent<Script>(entity);

        // Editable class name
        char buf[256];
        strcpy_s(buf, script.m_className.c_str());
        if (ImGui::InputText("Class", buf, sizeof(buf))) {
            script.m_className = buf;
            // TODO: rebuild ScriptInstance here if you support hot-reload
        }

        // Enable toggle
        ImGui::Checkbox("Enabled", &script.m_enabled);

        // Status
        ImGui::Text("Started: %s", script.m_started ? "Yes" : "No");

        // Optional runtime controls
        if (ImGui::Button("Start")) {
            // TODO: call into your script system: script.Start(entity);
            script.m_started = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            // TODO: ScriptSystem::Stop(entity);
            script.m_started = false;
        }
    }

    void HierarchyInspector::DrawAddComponentMenu(EntityID entity) {
        if (ImGui::MenuItem("Transform") && !ECS::GetInstance().HasComponent<Transform>(entity)) {
            ECS::GetInstance().AddComponent(entity, Transform());
        }
        if (ImGui::MenuItem("Mesh") && !ECS::GetInstance().HasComponent<Mesh>(entity)) {
            ECS::GetInstance().AddComponent(entity, graphics::GeometryFactory::CreateCube());
            ECS::GetInstance().AddComponent(entity, Material());
        }
        if (ImGui::MenuItem("Light") && !ECS::GetInstance().HasComponent<Light>(entity)) {
            ECS::GetInstance().AddComponent(entity, Light());
        }
        if (ImGui::MenuItem("Physics") && !ECS::GetInstance().HasComponent<PhysicComponent>(entity)) {
            ECS::GetInstance().AddComponent(entity, PhysicComponent());
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
        // Add more component types as needed
    }

} // namespace Ermine::editor