/* Start Header ************************************************************************/
/*!
\file       Component.tpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       Sep 15, 2024
\brief      To manage all the the different components that needed to be added or removed.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

#include <cassert>

#include "Component.h"

namespace Ermine
{
	/**
	 * @brief Get the component array of type T
	 */
	template <typename T>
	std::shared_ptr<ComponentArray<T>> ComponentManager::GetComponentArray()
	{
		const char* typeName = typeid(T).name();

		return std::static_pointer_cast<ComponentArray<T>>(m_ComponentArrays[typeName]);
	}

	/**
	 * @brief Register a component
	 */
	template <typename T>
	void ComponentManager::RegisterComponent()
	{
		const char* typeName = typeid(T).name();

		assert(m_ComponentTypes.find(typeName) == m_ComponentTypes.end() && "Registering component type more than once.");

		m_ComponentTypes.insert({ typeName, m_NextComponentType++ });

		m_ComponentArrays.insert({ typeName, std::make_shared<ComponentArray<T>>() });
	}

	/**
	 * @brief Get the component type ID of a component
	 */
	template <typename T>
	ComponentTypeID ComponentManager::GetComponentType()
	{
		const char* typeName = typeid(T).name();

		assert(m_ComponentTypes.find(typeName) != m_ComponentTypes.end() && "Component not registered before use.");

		return m_ComponentTypes[typeName];
	}

	/**
	 * @brief Add a component to an entity
	 * @param entity The entity to add the component to
	 * @param component The component to add
	 */
	template <typename T>
	void ComponentManager::AddComponent(EntityID entity, T component)
	{
		GetComponentArray<T>()->InsertData(entity, component);
	}

	/**
	 * @brief Remove a component from an entity
	 * @param entity The entity to add the component to
	 */
	template <typename T>
	void ComponentManager::RemoveComponent(EntityID entity)
	{
		GetComponentArray<T>()->RemoveData(entity);
	}

	/**
	 * @brief Get a reference to a component of type T for an entity
	 * @param entity The entity to add the component to
	 */
	template <typename T>
	T& ComponentManager::GetComponent(EntityID entity)
	{
		return GetComponentArray<T>()->GetData(entity);
	}
}