/* Start Header ************************************************************************/
/*!
\file       State.h
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu
\date       04/10/2025
\brief      This file contains declarations for State.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"
#include "Entity.h"

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
}