/* Start Header ************************************************************************/
/*!
\file       FiniteStateMachine.cpp
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu
\date       07/09/2025
\brief      This file contains declarations for Finite State Machine.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "FiniteStateMachine.h"

namespace Ermine
{
    // StateManager
    void StateManager::Init(EntityID entity, State* startState)
    {
        auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(entity);
        fsm.manager = this;
        fsm.Init(entity, startState);
    }

    void StateManager::Update(float dt)
    {
        for (auto entity : m_Entities)
        {
            if (!ECS::GetInstance().HasComponent<StateMachine>(entity))
                continue;

            auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(entity);
            if (!fsm.m_CurrentState)
                continue;

            fsm.m_CurrentState->Update(entity, dt);

            fsm.stateTimer += dt;
            if (fsm.stateTimer > fsm.stateDuration)
            {
                fsm.stateTimer = 0.0f;
                fsm.m_CurrentState->Exit(entity);

                auto it = fsm.transitions.find(fsm.m_CurrentState);
                if (it != fsm.transitions.end())
                {
                    fsm.m_CurrentState = it->second;
                    fsm.m_CurrentState->Enter(entity);
                }
            }
        }
    }

    void StateManager::Free(EntityID entity)
    {
        if (!ECS::GetInstance().HasComponent<StateMachine>(entity))
            return;

        auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(entity);

        // Call Exit() on the current state before clearing
        if (fsm.m_CurrentState)
            fsm.m_CurrentState->Exit(entity);

        fsm.m_CurrentState = nullptr;
        fsm.manager = nullptr;
        fsm.stateTimer = 0.0f;
    }
}