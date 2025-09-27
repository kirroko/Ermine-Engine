/* Start Header ************************************************************************/
/*!
\file       AnimationManager.h
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       26/09/2025
\brief      This file contains the declaration of the animation manager.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "ECS.h"

namespace Ermine::graphics
{
	/**
	 * @brief ECS system that updates Animator components.
	 */
	class AnimationManager : public System {
	public:
		/**
		 * @brief Update all entities with AnimationComponent.
		 * @param deltaTime Frame time step in seconds
		 */
		void Update(double deltaTime);
	};
}
