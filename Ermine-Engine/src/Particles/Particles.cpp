/* Start Header ************************************************************************/
/*!
\file       Particles.cpp
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu
\date       07/09/2025
\brief      This file contains definitions for ParticleSystem, ParticleEmitter and ParticlesImGUI.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "Particles.h"

namespace Ermine {
    void ParticleSystem::Init(std::shared_ptr<graphics::Shader> shader)
    {
        m_QuadMesh = graphics::GeometryFactory::CreateQuad(1.0f, 1.0f);
        m_Shader = shader;
        //m_DefaultTexture = texture;
    }

    void ParticleSystem::Update(float dt)
    {
        auto& ecs = ECS::GetInstance();
        std::vector<size_t> toRemove;

        // Update emitters
        for (auto entity : m_Entities)
        {
            if (!ecs.HasComponent<ParticleEmitter>(entity))
                continue;

            auto& emitter = ecs.GetComponent<ParticleEmitter>(entity);
            if (!emitter.active)
                continue;

            //emitter.timeAccumulator += dt;
            //float emitInterval = 1.0f / emitter.emissionRate;

            //while (emitter.timeAccumulator >= emitInterval)
            //{
            //    emitter.timeAccumulator -= emitInterval;
            //    auto& transform = ecs.GetComponent<Transform>(entity);
            //    Emit(emitter, transform.position, emitter.velocity, emitter.particleLifetime, emitter.particleSize, emitter.color);
            //}
        }

        // Update particle entities
        for (size_t i = 0; i < m_Particles.size();)
        {
            auto& p = m_Particles[i];
            p.age += dt;

            if (p.age >= p.lifetime)
            {
                ecs.DestroyEntity(p.entity);
                m_Particles[i] = m_Particles.back();
                m_Particles.pop_back();
                continue;
            }

            auto& transform = ecs.GetComponent<Transform>(p.entity);
            transform.position += p.velocity * dt;
            ++i;
        }
    }

    void ParticleSystem::Emit(const ParticleEmitter& emitter, const Vec3& pos, const Vec3& vel, float lifetime, float size)
    {
        auto& ecs = ECS::GetInstance();
        auto& assetManager = AssetManager::GetInstance();

        // Create a new entity
        EntityID e = ecs.CreateEntity();

        // Create and configure each component
        Transform transform{};
        transform.position = pos;
        transform.scale = Vec3(size, size, size);
        ecs.AddComponent(e, transform);

        ecs.AddComponent(e, m_QuadMesh);

        // Load texture
        std::shared_ptr<graphics::Texture> texture = nullptr;
        if (!emitter.textureName.empty())
            texture = assetManager.GetTexture(emitter.textureName);
        if (!texture || !texture->IsValid())
            texture = m_DefaultTexture;

        //EE_CORE_INFO("Particle texture: {}", emitter.textureName);
        //if (texture)
        //    EE_CORE_INFO("Texture valid? {}", texture->IsValid() ? "yes" : "NO");
        //else
        //    EE_CORE_INFO("Texture is nullptr!");

        auto gfxMaterial = std::make_shared<graphics::Material>(m_Shader);
        if (texture && texture->IsValid())
        {
            gfxMaterial->SetTexture("materialAlbedoMap", texture);
            gfxMaterial->SetBool("materialHasAlbedoMap", true);
        }

        Material material(gfxMaterial);
        ecs.AddComponent(e, material);

        // Track it manually
        Particle p;
        p.entity = e;
        p.velocity = vel;
        p.lifetime = lifetime;
        p.age = 0.0f;

        m_Particles.push_back(p);
    }

    void ParticlesImGUI::Update() {}

    void ParticlesImGUI::Render()
    {
        //if (ImGui::Begin("Particle Editor"))
        //{
        //    ImGui::Text("Emitter Settings");

        //    ImGui::InputFloat3("Position", &m_Position[0]);
        //    ImGui::InputFloat3("Velocity", &m_Velocity[0]);
        //    ImGui::InputFloat("Lifetime", &m_Lifetime);
        //    ImGui::InputFloat("Size", &m_Size);
        //    ImGui::ColorEdit4("Color", &m_Color[0]);
        //    ImGui::InputInt("Count", &m_Count);

        //    ImGui::Separator();

        //    auto& textures = AssetManager::GetInstance().GetLoadedTextures();
        //    static int currentIndex = 0;
        //    static std::string loadStatus;

        //    if (textureNames.size() != textures.size())
        //    {
        //        textureNames.clear();
        //        textureNames.reserve(textures.size());
        //        for (auto& kv : textures)
        //            textureNames.push_back(kv.first);
        //    }

        //    if (!textureNames.empty())
        //    {
        //        // Drop down list of loaded textures
        //        if (ImGui::BeginCombo("Texture", textureNames[currentIndex].c_str()))
        //        {
        //            for (int i = 0; i < textureNames.size(); ++i)
        //            {
        //                bool isSelected = (currentIndex == i);
        //                if (ImGui::Selectable(textureNames[i].c_str(), isSelected))
        //                {
        //                    currentIndex = i;
        //                    //m_SelectedTexture = textures.at(textureNames[i]); // Set the texture in drop down list
        //                }
        //                if (isSelected)
        //                    ImGui::SetItemDefaultFocus();
        //            }
        //            ImGui::EndCombo();
        //        }

        //        // Load button
        //        if (ImGui::Button("Load Texture"))
        //        {
        //            // Attempt to load texture from the selected dropdown name
        //            auto it = textures.find(textureNames[currentIndex]);
        //            if (it != textures.end() && it->second)
        //            {
        //                m_SelectedTexture = it->second;
        //                if (m_Emitter)
        //                    m_Emitter->SetTexture(m_SelectedTexture);

        //                loadStatus = "Texture loaded successfully!";
        //            }
        //            else
        //            {
        //                loadStatus = "Failed to load texture!";
        //            }
        //        }

        //        if (!loadStatus.empty())
        //        {
        //            ImGui::SameLine();
        //            ImGui::Text("%s", loadStatus.c_str());
        //        }
        //    }
        //    else
        //    {
        //        ImGui::Text("No textures loaded.");
        //    }

        //    // Preset selection
        //    const char* presetNames[] = { "Default", "SpreadOut", "Fireflies" };
        //    int currentPresetIdx = static_cast<int>(m_CurrentPreset);
        //    if (ImGui::Combo("Preset", &currentPresetIdx, presetNames, IM_ARRAYSIZE(presetNames)))
        //    {
        //        m_CurrentPreset = static_cast<PresetType>(currentPresetIdx);
        //    }

        //    ImGui::Separator();

        //    // Emit Particles button
        //    if (ImGui::Button("Emit Particles"))
        //    {
        //        loadStatus = "";
        //        if (m_Emitter)
        //        {
        //            if (m_SelectedTexture)
        //                m_Emitter->SetTexture(m_SelectedTexture);

        //            for (int i = 0; i < m_Count; i++)
        //            {
        //                Vec3 pos = { m_Position.x, m_Position.y, m_Position.z };
        //                Vec3 vel = { m_Velocity.x, m_Velocity.y, m_Velocity.z };
        //                float lifetime = m_Lifetime;
        //                float size = m_Size;
        //                Vec4 colour = { m_Color.r, m_Color.g, m_Color.b, m_Color.a };

        //                // Add particle emission behaviours here
        //                switch (m_CurrentPreset)
        //                {
        //                case PresetType::Default:
        //                    // Use UI values directly
        //                    break;

        //                case PresetType::SpreadOut:
        //                    vel.x = ((rand() % 100) / 100.0f - 0.5f);
        //                    break;

        //                case PresetType::Fireflies:
        //                    pos.x += ((rand() % 100) / 100.0f - 0.5f) * 2.0f; // spread in X
        //                    pos.y += ((rand() % 100) / 100.0f) * 2.0f; // float upwards
        //                    vel = { ((rand() % 100) / 100.0f - 0.5f) * 0.5f, ((rand() % 100) / 100.0f) * 1.0f, ((rand() % 100) / 100.0f - 0.5f) * 0.5f };
        //                    lifetime = 3.0f + (rand() % 100) / 100.0f * 2.0f; // 3–5s
        //                    size = 0.1f + (rand() % 100) / 100.0f * 0.2f; // vary size
        //                    //colour = { 1.0f, 1.0f, 0.3f, 1.0f }; // yellow glow
        //                    break;
        //                }

        //                m_Emitter->Emit(pos, vel, lifetime, size, colour);
        //            }
        //        }
        //    }
        //}
        //ImGui::End();

        ImGui::Begin("Particles");

        EntityID selected = SceneManager::GetInstance().EnsureActiveScene().GetSelectedEntity();
        if (selected == 0) {
            ImGui::Text("No entity selected");
            ImGui::End();
            return;
        }

        auto& ecs = ECS::GetInstance();
        if (ecs.HasComponent<ParticleEmitter>(selected))
        {
            auto& emitter = ecs.GetComponent<ParticleEmitter>(selected);
            ImGui::Checkbox("Active", &emitter.active);
            ImGui::DragFloat("Emission Rate", &emitter.emissionRate, 1.0f, 0.0f, 500.0f);
            ImGui::DragFloat("Lifetime", &emitter.particleLifetime, 0.1f, 0.1f, 10.0f);
            ImGui::DragFloat("Size", &emitter.particleSize, 0.01f, 0.01f, 10.0f);
            ImGui::DragFloat3("Velocity", reinterpret_cast<float*>(&emitter.velocity), 0.1f);

            auto& assetManager = AssetManager::GetInstance();
            const auto& textureFiles = assetManager.GetLoadedTextures();

            ImGui::Text("Texture");
            if (ImGui::BeginCombo("##textureCombo", emitter.textureName.c_str()))
            {
                for (const auto& [name, texturePtr] : textureFiles)  // C++17 structured binding
                {
                    bool isSelected = (emitter.textureName == name);
                    if (ImGui::Selectable(name.c_str(), isSelected))
                        emitter.textureName = name;

                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            const char* presetNames[] = { "Default", "Spread Out", "Fireflies" };
            int currentPresetIndex = static_cast<int>(m_CurrentPreset);

            if (ImGui::Combo("Preset", &currentPresetIndex, presetNames, IM_ARRAYSIZE(presetNames)))
            {
                m_CurrentPreset = static_cast<PresetType>(currentPresetIndex);

                switch (m_CurrentPreset)
                {
                case PresetType::Default:
                    emitter.velocity = { 0.0f, 2.0f, 0.0f };
                    emitter.emissionRate = 50.0f;
                    emitter.particleLifetime = 2.0f;
                    emitter.particleSize = 0.2f;
                    emitter.textureName = "../Resources/Textures/greybox_red_solid.png";
                    break;

                case PresetType::SpreadOut:
                    emitter.velocity = { 0.0f, 2.5f, 0.0f };
                    emitter.emissionRate = 80.0f;
                    emitter.particleLifetime = 1.5f;
                    emitter.particleSize = 0.25f;
                    emitter.textureName = "../Resources/Textures/greybox_orange_solid.png";
                    break;

                case PresetType::Fireflies:
                    emitter.velocity = { 0.0f, 1.0f, 0.0f };
                    emitter.emissionRate = 30.0f;
                    emitter.particleLifetime = 3.5f;
                    emitter.particleSize = 0.15f;
                    emitter.textureName = "../Resources/Textures/greybox_yellow_solid.png";
                    break;
                }

                // Automatically load the preset’s texture
                m_SelectedTexture = AssetManager::GetInstance().GetTexture(emitter.textureName);
            }

            if (ImGui::Button("Emit"))
            {
                EntityID selected = SceneManager::GetInstance().EnsureActiveScene().GetSelectedEntity();

                if (ecs.HasComponent<Transform>(selected) && ecs.HasComponent<ParticleEmitter>(selected))
                {
                    auto& emitter = ecs.GetComponent<ParticleEmitter>(selected);
                    auto& transform = ecs.GetComponent<Transform>(selected);

                    // Since this class is in Particles.cpp, just call Emit directly
                    auto particleSystem = ecs.GetSystem<ParticleSystem>();
                    particleSystem->Emit(emitter, transform.position, emitter.velocity, emitter.particleLifetime, emitter.particleSize);
                }
            }
        }
        else
        {
            ImGui::Text("Select an entity with a Particle Emitter Component.");
        }

        ImGui::End();
    }
}