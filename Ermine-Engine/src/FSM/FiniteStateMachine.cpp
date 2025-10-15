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
    void StateManager::Init(EntityID entity, ScriptNode* startScript)
    {
        auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(entity);
        fsm.manager = this;
        fsm.Init(entity, startScript);
    }

    void StateManager::Update(float dt)
    {
        for (auto entity : m_Entities)
        {
            if (!ECS::GetInstance().HasComponent<StateMachine>(entity))
                continue;

            auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(entity);
            fsm.Update(entity, dt);
        }
    }
}