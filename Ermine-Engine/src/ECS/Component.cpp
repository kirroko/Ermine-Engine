/* Start Header ************************************************************************/
/*!
\file       Component.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       Sep 15, 2024
\brief      This file contains the definition of the member function of ComponentManager.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "Component.h"

namespace Ermine
{
	/**
	 * @brief Register a component
	 * @param entity The entity to add the component to
	 */
	void ComponentManager::EntityDestroyed(EntityID entity) const
	{
		for (auto const& pair : m_ComponentArrays) // Iterate over all component arrays
		{
			auto const& component = pair.second;

			component->EntityDestroyed(entity);
		}
	}
}
// 0x4B45414E