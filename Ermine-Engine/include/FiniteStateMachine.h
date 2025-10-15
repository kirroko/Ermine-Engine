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
        void Init(EntityID entity, ScriptNode* startScript);
        /*!***********************************************************************
        \brief
            Update all managed state machines.
        *************************************************************************/
        void Update(float dt);
    };
}