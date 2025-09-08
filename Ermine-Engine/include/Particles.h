/* Start Header ************************************************************************/
/*!
\file       Particles.h
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu
\date       07/09/2025
\brief      This file contains declarations for ParticleSystem and ParticleEmitter.

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

namespace Ermine {
    class ParticleSystem : public System
    {
    public:
        void Update(float dt);

    private:
        std::vector<Particle> m_Particles;
        size_t m_MaxParticles;
    };

    class ParticleEmitter
    {
    public:
        ParticleEmitter(const Mesh& quadMesh, std::shared_ptr<graphics::Shader> shader, std::shared_ptr<graphics::Texture> texture);
        void Emit(const Vec3& pos, const Vec3& vel, float lifetime, float size, const Vec4& colour);

    private:
        Mesh m_QuadMesh;
        std::shared_ptr<graphics::Shader> m_Shader;
        std::shared_ptr<graphics::Texture> m_Texture;
    };
}
