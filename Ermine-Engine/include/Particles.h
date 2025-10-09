/* Start Header ************************************************************************/
/*!
\file       Particles.h
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu
\date       07/09/2025
\brief      This file contains declarations for ParticleSystem, ParticleEmitter and ParticlesImGUI.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "Shader.h"
#include "Renderer.h"
#include "ECS.h"
#include "Components.h"
#include "ImGuiUIWindow.h"
#include "imgui.h"
#include "AssetManager.h"
#include"SceneManager.h"
#include "GeometryFactory.h"

namespace Ermine {
    class ParticleSystem : public System
    {
    public:
        void Init(std::shared_ptr<graphics::Shader> shader);
        void Update(float dt);
        void Emit(const ParticleEmitter& emitter, const Vec3& pos, const Vec3& vel, float lifetime, float size);
    private:
        struct Particle {
            EntityID entity;
            Vec3 velocity;
            float lifetime;
            float age;
        };

        std::vector<Particle> m_Particles;
        Mesh m_QuadMesh;
        std::shared_ptr<graphics::Shader> m_Shader;
        std::shared_ptr<graphics::Texture> m_DefaultTexture;
    };

    class ParticlesImGUI : public ImGUIWindow
    {
    public:
        ParticlesImGUI() : ImGUIWindow("Particles IMGUI") {}

        /*!***********************************************************************
        \brief
           Update logic for the Particles ImGui window.
       *************************************************************************/
        void Update() override;
        /*!***********************************************************************
        \brief
           Render the ImGui window for controlling particles.
       *************************************************************************/
        void Render() override;

    private:

        // Editable parameters
        //glm::vec3 m_Position = { 0.0f, 0.0f, -1.0f };
        //glm::vec3 m_Velocity = { 0.0f, 2.0f, 0.0f };
        //float m_Lifetime = 2.0f;
        //float m_Size = 0.2f;

        //int m_Count = 1;
        //char m_TexturePath[256] = "";
        std::shared_ptr<graphics::Texture> m_SelectedTexture = nullptr;
        //std::vector<std::string> textureNames;

        // Presets
        enum class PresetType { Default, SpreadOut, Fireflies };
        PresetType m_CurrentPreset = PresetType::Default;
    };
}
