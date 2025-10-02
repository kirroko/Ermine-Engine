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
        /*!***********************************************************************
        \brief
            Called when a state is entered by an entity.
        *************************************************************************/
        virtual void Enter(EntityID entity) = 0;
        /*!***********************************************************************
        \brief
            Called every frame to update the entity in this state.
        *************************************************************************/
        virtual void Update(EntityID entity, float dt) = 0;
        /*!***********************************************************************
        \brief
            Called when a state is exited by an entity.
        *************************************************************************/
        virtual void Exit(EntityID entity) = 0;
    };

    // State Machine
    class StateMachine // should be ECS component
    {
        State* m_CurrentState = nullptr;

    public:
        /*!***********************************************************************
        \brief
           Change the current state of an entity.
        *************************************************************************/
        void ChangeState(EntityID entity, State* newState);
        /*!***********************************************************************
        \brief
           Update the current state of an entity.
        *************************************************************************/
        void Update(EntityID entity, float dt);
    };

    // State Manager
    class StateManager // should be ECS system
    {
        std::unordered_map<EntityID, StateMachine> m_StateMachines;

    public:
        /*!***********************************************************************
        \brief
            Initialize a state machine for an entity with a starting state.
        *************************************************************************/
        void Init(EntityID entity, State* startState);
        /*!***********************************************************************
        \brief
            Update all managed state machines.
        *************************************************************************/
        void Update(float dt);
        /*!***********************************************************************
        \brief
            Free the state machine belonging to an entity.
        *************************************************************************/
        void Free(EntityID entity);
    };

    // Concrete States
    class IdleState : public State
    {
    public:
        void Enter(EntityID entity) override;
        void Update(EntityID entity, float dt) override;
        void Exit(EntityID entity) override;
    };

    class RoamState : public State
    {
        float angle = 0.0f;
        float speed = 1.0f;
        float radius = 3.0f;
    public:
        void Enter(EntityID entity) override;
        void Update(EntityID entity, float dt) override;
        void Exit(EntityID entity) override;
    };

    class AttackState : public State
    {
    public:
        void Enter(EntityID entity) override;
        void Update(EntityID entity, float dt) override;
        void Exit(EntityID entity) override;
    };

    class DeadState : public State
    {
    public:
        void Enter(EntityID entity) override;
        void Update(EntityID entity, float dt) override;
        void Exit(EntityID entity) override;
    };
}