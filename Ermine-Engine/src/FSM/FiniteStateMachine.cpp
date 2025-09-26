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

#include "PreCompile.h"
#include "FiniteStateMachine.h"

namespace Ermine
{
    inline Vec3 Normalize(const Vec3& v)
    {
        float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (len > 0.0001f)
            return Vec3(v.x / len, v.y / len, v.z / len);
        return Vec3(0.0f, 0.0f, 0.0f);
    }

    // StateMachine
    void StateMachine::ChangeState(EntityID entity, State* newState)
    {
        if (m_CurrentState)
            m_CurrentState->Exit(entity);

        m_CurrentState = newState;

        if (m_CurrentState)
            m_CurrentState->Enter(entity);
    }

    void StateMachine::Update(EntityID entity, float deltaTime)
    {
        if (m_CurrentState)
            m_CurrentState->Update(entity, deltaTime);
    }

    // StateManager
    void StateManager::Init(EntityID entity, State* startState)
    {
        m_StateMachines[entity].ChangeState(entity, startState);
    }

    void StateManager::Update(float deltaTime)
    {
        for (auto& [entity, machine] : m_StateMachines)
        {
            machine.Update(entity, deltaTime);
        }
    }

    void StateManager::Free(EntityID entity)
    {
        m_StateMachines.erase(entity);
    }

    // IdleState
    void IdleState::Enter(EntityID entity)
    {
        //EE_CORE_INFO("Entity {0} entered Idle state.", entity);
    }

    void IdleState::Update(EntityID entity, float deltaTime)
    {
        //EE_CORE_INFO("Entity {0} is idling...", entity);
    }

    void IdleState::Exit(EntityID entity)
    {
        //EE_CORE_INFO("Entity {0} exiting Idle state.", entity);
    }

    // RoamState
    void RoamState::Enter(EntityID entity)
    {
        //EE_CORE_INFO("Entity {0} entered Roam state.", entity);
    }

    void RoamState::Update(EntityID entity, float deltaTime)
    {
        //EE_CORE_INFO("Entity {0} is roaming...", entity);
        if (!ECS::GetInstance().IsEntityValid(entity))
            return;

        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);

        // Move cube
        //transform.position.x += 1.0f * deltaTime;

        // Update orbit angle
        angle += speed * deltaTime;
        if (angle > 2.0f * M_PI) angle -= 2.0f * M_PI;

        // Compute new position along circle (XZ plane)
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        transform.position = Vec3(x, 0.0f, z);

        // Compute forward direction (tangent to circle)
        Vec3 forward(-sin(angle), 0.0f, cos(angle));
        forward = Normalize(forward);

        // Convert forward vector into quaternion facing that way
        float yaw = atan2(forward.x, forward.z);  // yaw in radians
        float halfYaw = yaw * 0.5f;
        transform.rotation = Quaternion(0.0f, sin(halfYaw), 0.0f, cos(halfYaw));

        //EE_CORE_INFO("Entity {0} is roaming at position ({1}, {2}, {3})",
        //    entity, transform.position.x, transform.position.y, transform.position.z);
    }

    void RoamState::Exit(EntityID entity)
    {
        //EE_CORE_INFO("Entity {0} exiting Roam state.", entity);
    }

    // AttackState
    void AttackState::Enter(EntityID entity)
    {
        //EE_CORE_INFO("Entity {0} entered Attack state.", entity);
    }

    void AttackState::Update(EntityID entity, float deltaTime)
    {
        //EE_CORE_INFO("Entity {0} is attacking!", entity);
    }

    void AttackState::Exit(EntityID entity)
    {
        //EE_CORE_INFO("Entity {0} exiting Attack state.", entity);
    }

    // DeadState
    void DeadState::Enter(EntityID entity)
    {
        //EE_CORE_INFO("Entity {0} entered Dead state.", entity);
    }

    void DeadState::Update(EntityID entity, float deltaTime)
    {
        //EE_CORE_INFO("Entity {0} is dead...", entity);
    }

    void DeadState::Exit(EntityID entity)
    {
        //EE_CORE_INFO("Entity {0} exiting Dead state.", entity);
    }
}