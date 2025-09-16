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
        EE_CORE_INFO("Entity {0} entered Idle state.", entity);
    }

    void IdleState::Update(EntityID entity, float deltaTime)
    {
        EE_CORE_INFO("Entity {0} is idling...", entity);
    }

    void IdleState::Exit(EntityID entity)
    {
        EE_CORE_INFO("Entity {0} exiting Idle state.", entity);
    }

    // RoamState
    void RoamState::Enter(EntityID entity)
    {
        EE_CORE_INFO("Entity {0} entered Roam state.", entity);
    }

    void RoamState::Update(EntityID entity, float deltaTime)
    {
        EE_CORE_INFO("Entity {0} is roaming...", entity);

        auto& transform = ECS::GetInstance().GetComponent<Transform>(entity);

        // Move cube forward
        transform.position.x += 1.0f * deltaTime;

        EE_CORE_INFO("Entity {0} is roaming at position ({1}, {2}, {3})",
            entity, transform.position.x, transform.position.y, transform.position.z);
    }

    void RoamState::Exit(EntityID entity)
    {
        EE_CORE_INFO("Entity {0} exiting Roam state.", entity);
    }

    // AttackState
    void AttackState::Enter(EntityID entity)
    {
        EE_CORE_INFO("Entity {0} entered Attack state.", entity);
    }

    void AttackState::Update(EntityID entity, float deltaTime)
    {
        EE_CORE_INFO("Entity {0} is attacking!", entity);
    }

    void AttackState::Exit(EntityID entity)
    {
        EE_CORE_INFO("Entity {0} exiting Attack state.", entity);
    }

    // DeadState
    void DeadState::Enter(EntityID entity)
    {
        EE_CORE_INFO("Entity {0} entered Dead state.", entity);
    }

    void DeadState::Update(EntityID entity, float deltaTime)
    {
        EE_CORE_INFO("Entity {0} is dead...", entity);
    }

    void DeadState::Exit(EntityID entity)
    {
        EE_CORE_INFO("Entity {0} exiting Dead state.", entity);
    }
}