/* Start Header ************************************************************************/
/*!
\file       Particles.cpp
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu
\date       07/09/2025
\brief      This file contains definitions for ParticleSystem and ParticleEmitter.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "Particles.h"

namespace Ermine {
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

    ParticleEmitter::ParticleEmitter(const Mesh& quadMesh, std::shared_ptr<graphics::Shader> shader, std::shared_ptr<graphics::Texture> texture)
        : m_QuadMesh(quadMesh), m_Shader(std::move(shader)), m_Texture(std::move(texture)) {}

    void ParticleEmitter::Emit(const Vec3& pos, const Vec3& vel, float lifetime, float size, const Vec4& colour)
    {
        auto& ecs = ECS::GetInstance();
        auto entity = ecs.CreateEntity();

        ecs.AddComponent(entity, Transform(pos, Vec3(0, 0, 0), Vec3(size, size, size)));
        ecs.AddComponent(entity, m_QuadMesh);
        ecs.AddComponent(entity, Material(m_Shader, m_Texture));

        Particle particle;
        particle.velocity = vel;
        particle.lifetime = lifetime;
        particle.size = size;
        particle.colour = colour;
        ecs.AddComponent(entity, particle);
    }
}