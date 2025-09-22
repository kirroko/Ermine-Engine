/* Start Header ************************************************************************/
/*!
\file       FiniteStateMachine.h
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu
\date       16/09/2025
\brief      This file contains declarations for 

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "Entity.h"
#include <unordered_map>
#include "ECS.h"
#include "Components.h"

namespace Ermine
{
    // Base State
    class State
    {
    public:
        virtual ~State() = default;
        virtual void Enter(EntityID entity) = 0;
        virtual void Update(EntityID entity, float deltaTime) = 0;
        virtual void Exit(EntityID entity) = 0;
    };

    // State Machine
    class StateMachine
    {
        State* m_CurrentState = nullptr;

    public:
        void ChangeState(EntityID entity, State* newState);
        void Update(EntityID entity, float deltaTime);
    };

    // State Manager
    class StateManager
    {
        std::unordered_map<EntityID, StateMachine> m_StateMachines;

    public:
        void Init(EntityID entity, State* startState);
        void Update(float deltaTime);
        void Free(EntityID entity);
    };

    // Concrete States
    class IdleState : public State
    {
    public:
        void Enter(EntityID entity) override;
        void Update(EntityID entity, float deltaTime) override;
        void Exit(EntityID entity) override;
    };

    class RoamState : public State
    {
        float angle = 0.0f;
        float speed = 1.0f;
        float radius = 3.0f;
    public:
        void Enter(EntityID entity) override;
        void Update(EntityID entity, float deltaTime) override;
        void Exit(EntityID entity) override;
    };

    class AttackState : public State
    {
    public:
        void Enter(EntityID entity) override;
        void Update(EntityID entity, float deltaTime) override;
        void Exit(EntityID entity) override;
    };

    class DeadState : public State
    {
    public:
        void Enter(EntityID entity) override;
        void Update(EntityID entity, float deltaTime) override;
        void Exit(EntityID entity) override;
    };
}