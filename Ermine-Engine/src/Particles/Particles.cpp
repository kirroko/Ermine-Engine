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
    void ParticleSystem::Init(const Mesh& quadMesh, std::shared_ptr<graphics::Shader> shader, std::shared_ptr<graphics::Texture> texture)
    {
        // Create the emitter
        m_Emitter = std::make_unique<ParticleEmitter>(quadMesh, shader, texture);
    }

    void ParticleSystem::Update(float dt)
    {
        auto& ecs = ECS::GetInstance();
        std::vector<EntityID> toDestroy;

        for (auto entity : m_Entities)
        {
            auto& particle = ecs.GetComponent<Particle>(entity);
            auto& transform = ecs.GetComponent<Transform>(entity);

            // Age particle
            particle.age += dt;
            if (particle.age >= particle.lifetime)
            {
                toDestroy.push_back(entity);
                continue;
            }

            // Move particle
            transform.position += particle.velocity * dt;

            // Fade/scale over time
            float lifeRatio = 1.0f - (particle.age / particle.lifetime);
            transform.scale = Vec3(particle.size * lifeRatio,
                particle.size * lifeRatio,
                particle.size * lifeRatio);
        }

        // Remove expired particles safely
        for (auto entity : toDestroy) {
            if (ECS::GetInstance().IsEntityValid(entity))
                ecs.DestroyEntity(entity);
        }
    }

    void ParticleSystem::ClearEmitter()
    {
        m_Emitter.reset();
    }

    ParticleEmitter::ParticleEmitter(const Mesh& quadMesh, std::shared_ptr<graphics::Shader> shader, std::shared_ptr<graphics::Texture> texture)
        : m_QuadMesh(quadMesh), m_Shader(std::move(shader)), m_Texture(std::move(texture)) {}

    ParticleEmitter::~ParticleEmitter()
    {
        // Release texture and shader references explicitly
        m_Texture.reset();
        m_Shader.reset();
    }

    void ParticleEmitter::Emit(const Vec3& pos, const Vec3& vel, float lifetime, float size, const Vec4& colour)
    {
        auto& ecs = ECS::GetInstance();
        auto entity = ecs.CreateEntity();

        ecs.AddComponent(entity, Transform(pos, Quaternion(), Vec3(size, size, size)));
        ecs.AddComponent(entity, m_QuadMesh);
        ecs.AddComponent(entity, Material(m_Shader, m_Texture));

        Particle particle;
        particle.velocity = vel;
        particle.lifetime = lifetime;
        particle.size = size;
        particle.colour = colour;
        ecs.AddComponent(entity, particle);
    }

    void ParticleEmitter::SetTexture(std::shared_ptr<graphics::Texture> texture)
    {
        m_Texture = std::move(texture);
    }

    void ParticleEmitter::SetTexture(const std::string& path)
    {
        m_Texture = AssetManager::GetInstance().LoadTexture(path);
    }

    ParticlesImGUI::ParticlesImGUI(ParticleEmitter* emitter) : ImGUIWindow("Particle Editor"), m_Emitter(std::move(emitter)) {}

    ParticlesImGUI::~ParticlesImGUI()
    {
        m_Emitter = nullptr;
        m_SelectedTexture.reset();
    }

    void ParticlesImGUI::Update() {}

    void ParticlesImGUI::Render()
    {
        if (ImGui::Begin("Particle Editor"))
        {
            ImGui::Text("Emitter Settings");

            ImGui::InputFloat3("Position", &m_Position[0]);
            ImGui::InputFloat3("Velocity", &m_Velocity[0]);
            ImGui::InputFloat("Lifetime", &m_Lifetime);
            ImGui::InputFloat("Size", &m_Size);
            ImGui::ColorEdit4("Color", &m_Color[0]);
            ImGui::InputInt("Count", &m_Count);

            ImGui::Separator();

            auto& textures = AssetManager::GetInstance().GetLoadedTextures();
            static int currentIndex = 0;
            static std::string loadStatus;

            if (textureNames.size() != textures.size())
            {
                textureNames.clear();
                textureNames.reserve(textures.size());
                for (auto& kv : textures)
                    textureNames.push_back(kv.first);
            }

            if (!textureNames.empty())
            {
                // Drop down list of loaded textures
                if (ImGui::BeginCombo("Texture", textureNames[currentIndex].c_str()))
                {
                    for (int i = 0; i < textureNames.size(); ++i)
                    {
                        bool isSelected = (currentIndex == i);
                        if (ImGui::Selectable(textureNames[i].c_str(), isSelected))
                        {
                            currentIndex = i;
                            //m_SelectedTexture = textures.at(textureNames[i]); // Set the texture in drop down list
                        }
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                // Load button
                if (ImGui::Button("Load Texture"))
                {
                    // Attempt to load texture from the selected dropdown name
                    auto it = textures.find(textureNames[currentIndex]);
                    if (it != textures.end() && it->second)
                    {
                        m_SelectedTexture = it->second;
                        if (m_Emitter)
                            m_Emitter->SetTexture(m_SelectedTexture);

                        loadStatus = "Texture loaded successfully!";
                    }
                    else
                    {
                        loadStatus = "Failed to load texture!";
                    }
                }

                if (!loadStatus.empty())
                {
                    ImGui::SameLine();
                    ImGui::Text("%s", loadStatus.c_str());
                }
            }
            else
            {
                ImGui::Text("No textures loaded.");
            }

            // Preset selection
            const char* presetNames[] = { "Default", "SpreadOut", "Fireflies" };
            int currentPresetIdx = static_cast<int>(m_CurrentPreset);
            if (ImGui::Combo("Preset", &currentPresetIdx, presetNames, IM_ARRAYSIZE(presetNames)))
            {
                m_CurrentPreset = static_cast<PresetType>(currentPresetIdx);
            }

            ImGui::Separator();

            // Emit Particles button
            if (ImGui::Button("Emit Particles"))
            {
                loadStatus = "";
                if (m_Emitter)
                {
                    if (m_SelectedTexture)
                        m_Emitter->SetTexture(m_SelectedTexture);

                    for (int i = 0; i < m_Count; i++)
                    {
                        Vec3 pos = { m_Position.x, m_Position.y, m_Position.z };
                        Vec3 vel = { m_Velocity.x, m_Velocity.y, m_Velocity.z };
                        float lifetime = m_Lifetime;
                        float size = m_Size;
                        Vec4 colour = { m_Color.r, m_Color.g, m_Color.b, m_Color.a };

                        // Add particle emission behaviours here
                        switch (m_CurrentPreset)
                        {
                        case PresetType::Default:
                            // Use UI values directly
                            break;

                        case PresetType::SpreadOut:
                            vel.x = ((rand() % 100) / 100.0f - 0.5f);
                            break;

                        case PresetType::Fireflies:
                            pos.x += ((rand() % 100) / 100.0f - 0.5f) * 2.0f; // spread in X
                            pos.y += ((rand() % 100) / 100.0f) * 2.0f; // float upwards
                            vel = { ((rand() % 100) / 100.0f - 0.5f) * 0.5f, ((rand() % 100) / 100.0f) * 1.0f, ((rand() % 100) / 100.0f - 0.5f) * 0.5f };
                            lifetime = 3.0f + (rand() % 100) / 100.0f * 2.0f; // 3–5s
                            size = 0.1f + (rand() % 100) / 100.0f * 0.2f; // vary size
                            //colour = { 1.0f, 1.0f, 0.3f, 1.0f }; // yellow glow
                            break;
                        }

                        m_Emitter->Emit(pos, vel, lifetime, size, colour);
                    }
                }
            }
        }
        ImGui::End();
    }
}