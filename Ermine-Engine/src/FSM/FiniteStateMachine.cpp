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

            auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(entity);

            // 1) Rebind runtime-only pointer after load
            if (fsm.manager == nullptr)
                fsm.manager = this;

            // 2) If scene load cleared current state, initialize it now
            auto currentValid = [&](StateMachine& f)->bool
                {
                    if (f.m_CurrentScript == nullptr) return false;

                    // 1) Must exist inside the node list (prevents stale pointer after load)
                    bool found = false;
                    for (auto& n : f.m_Nodes)
                    {
                        if (n.get() == f.m_CurrentScript)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found) return false;

                    // 2) Must have a script class name to instantiate
                    if (f.m_CurrentScript->scriptClassName.empty()) return false;

                    // 3) Must have an instance (or be able to create one)
                    // If your ScriptNode has `instance` pointer:
                    if (!f.m_CurrentScript->instance) return false;

                    return true;
                };

            if (!currentValid(fsm))
            {
                EE_CORE_INFO("[FSM] Current invalid after load, re-init entity {}", entity);

                fsm.Init(entity);

                // IMPORTANT: ensure state is actually running after init
                if (fsm.m_CurrentScript && !fsm.m_CurrentScript->instance)
                {
                    fsm.m_CurrentScript->CreateInstance(entity);
                    fsm.m_CurrentScript->OnEnter();
                }
            }

            // 3) Normal update
            fsm.Update(entity, dt);
        }
    }

    void StateManager::RequestNextState(EntityID entity)
    {
        //EE_CORE_INFO("FSM: RequestNextState called for entity {}", entity);

        auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(entity);

        if (fsm.m_CurrentScript == nullptr)
            fsm.Init(entity);

        int fromId = fsm.m_CurrentScript->id;

        auto it = fsm.scriptTransitions.find(fromId);
        if (it == fsm.scriptTransitions.end())
            return;

        ScriptNode* next = fsm.FindNodeById(it->second);
        if (!next)
            return;

        fsm.m_History.push_back(fromId);

        fsm.m_CurrentScript->OnExit();
        fsm.m_PreviousScript = fsm.m_CurrentScript;
        fsm.m_CurrentScript = next;

        fsm.m_CurrentScript->CreateInstance(entity);
        fsm.m_CurrentScript->OnEnter();

        //EE_CORE_INFO("FSM: Switched to next script '{}'", fsm.m_CurrentScript->scriptClassName);
    }

    void StateManager::RequestPreviousState(EntityID entity)
    {
        auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(entity);

        if (fsm.m_History.empty() || fsm.m_CurrentScript == nullptr)
            return;

        // Find a valid previous node (in case something got deleted)
        ScriptNode* prev = nullptr;
        while (!fsm.m_History.empty() && !prev)
        {
            int prevId = fsm.m_History.back();
            fsm.m_History.pop_back();
            prev = fsm.FindNodeById(prevId);
        }

        if (!prev)
            return;

        fsm.m_CurrentScript->OnExit();
        fsm.m_CurrentScript = prev;
        fsm.m_CurrentScript->CreateInstance(entity);
        fsm.m_CurrentScript->OnEnter();
    }
}