/* Start Header ************************************************************************/
/*!
\file       FiniteStateMachine.h
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu
\date       16/09/2025
\brief      This file contains declarations for Finite State Machine.

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
    // State Manager
    class StateManager : public System
    {
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

    extern IdleState g_IdleState;
    extern RoamState g_RoamState;
}