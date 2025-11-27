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
    void StateManager::Init(EntityID entity, ScriptNode*)
    {
        auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(entity);
        fsm.manager = this;
        fsm.Init(entity);
    }

    void StateManager::Update(float dt)
    {
        for (auto entity : m_Entities)
        {
            if (!ECS::GetInstance().HasComponent<StateMachine>(entity))
                continue;

            if (ECS::GetInstance().HasComponent<ObjectMetaData>(entity))
            {
                const auto& meta = ECS::GetInstance().GetComponent<ObjectMetaData>(entity);
                if (!meta.selfActive)
                    continue;
            }

            auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(entity);
            fsm.Update(entity, dt);
        }
    }

    void StateManager::RequestNextState(EntityID entity)
    {
        //EE_CORE_INFO("FSM: RequestNextState called for entity {}", entity);

        auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(entity);
        auto it = fsm.scriptTransitions.find(fsm.m_CurrentScript);

        if (it == fsm.scriptTransitions.end())
        {
            //EE_CORE_WARN("FSM: No transition found from current script '{}'", fsm.m_CurrentScript ? fsm.m_CurrentScript->scriptClassName : "NULL");
            return;
        }

        if (it != fsm.scriptTransitions.end())
        {
            fsm.m_CurrentScript->OnExit();
            fsm.m_PreviousScript = fsm.m_CurrentScript;
            fsm.m_CurrentScript = it->second;
            fsm.m_CurrentScript->CreateInstance(entity);
            fsm.m_CurrentScript->OnEnter();
        }

        //EE_CORE_INFO("FSM: Switched to next script '{}'", fsm.m_CurrentScript->scriptClassName);
    }

    void StateManager::RequestPreviousState(EntityID entity)
    {
        auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(entity);
        if (fsm.m_PreviousScript)
        {
            fsm.m_CurrentScript->OnExit();
            fsm.m_CurrentScript = fsm.m_PreviousScript;
            fsm.m_CurrentScript->CreateInstance(entity);
            fsm.m_CurrentScript->OnEnter();
        }
    }
}