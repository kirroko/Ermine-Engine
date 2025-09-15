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
#include <ranges>
#include <utility>

#include "Component.h"

namespace Ermine
{
	/**
	 * @brief Get the component array of type T
	 */
	template <typename T>
	std::shared_ptr<ComponentArray<T>> ComponentManager::GetComponentArray()
	{
		auto typeIdx = std::type_index(typeid(T));
		auto itName = m_TypeIndexToName.find(typeIdx);
		assert(itName != m_TypeIndexToName.end() && "Component type not registered before use!");
		auto& key = itName->second;
		return std::static_pointer_cast<ComponentArray<T>>(m_ComponentArrays[key]);
	}

	/**
	 * @brief Register a component
	 */
	template <typename T>
	void ComponentManager::RegisterComponent()
	{
		RegisterComponent<T>(typeid(T).name());
	}

	template <typename T>
	void ComponentManager::RegisterComponent(std::string_view customName)
	{
		std::string nameStr{customName};
		auto typeIdx = std::type_index(typeid(T));

		assert(m_ComponentTypes.find(nameStr) == m_ComponentTypes.end() && "Registering component name more than once!");
		assert(m_TypeIndexToName.find(typeIdx) == m_TypeIndexToName.end() && "Registering component type more than once!");

		m_ComponentTypes.insert({nameStr,m_NextComponentType++});
		m_ComponentArrays.insert({nameStr, std::make_shared<ComponentArray<T>>()});
		m_TypeIndexToName.insert({typeIdx,nameStr});

		ComponentDescriptor desc {
		.name = nameStr,
		.typeID = m_NextComponentType,
		.size = sizeof(T),
		.has = [this](EntityID entity) { return this->HasComponent<T>(entity); }
		};
		m_Descriptors.emplace(nameStr,std::move(desc));
	}

	template <typename T, typename CloneFn>
	void ComponentManager::RegisterComponent(std::string_view customName, CloneFn customClone)
	{
		std::string nameStr(customName);
		auto typeIdx = std::type_index(typeid(T));

		assert(m_ComponentTypes.find(nameStr) == m_ComponentTypes.end() && "Registering component name more than once.");
		assert(m_TypeIndexToName.find(typeIdx) == m_TypeIndexToName.end() && "Registering component type more than once.");

		m_ComponentTypes.insert({nameStr, m_NextComponentType});
		m_ComponentArrays.insert({nameStr, std::make_shared<ComponentArray<T>>()});
		m_TypeIndexToName.insert({typeIdx, nameStr});

		ComponentDescriptor desc{
		.name = nameStr,
		.typeID = m_NextComponentType,
		.size = sizeof(T),
		.has = [this](EntityID e) { return this->HasComponent<T>(e);},
		.clone = [this, clone = std::move(customClone)](EntityID s, EntityID d) { clone(*this,s,d); }
		};
		m_Descriptors.emplace(nameStr,std::move(desc));

		++m_NextComponentType;
	}

	/**
	 * @brief Get the component type ID of a component
	 */
	template <typename T>
	ComponentTypeID ComponentManager::GetComponentType()
	{
		auto typeIdx = std::type_index(typeid(T));
		auto itName = m_TypeIndexToName.find(typeIdx);
		assert(itName != m_TypeIndexToName.end() && "Component not registered before use!");
		return m_ComponentTypes[itName->second];
	}

	/**
	 * @brief Add a component to an entity
	 * @param entity The entity to add the component to
	 * @param component The component to add
	 */
	template <typename T>
	void ComponentManager::AddComponent(EntityID entity, T component)
	{
		GetComponentArray<T>()->InsertData(entity, std::move(component));
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

	template <typename T>
	void ComponentManager::ForEachComponentType(T&& fn) const
	{
		for (const auto& desc : m_Descriptors | std::views::values)
			fn(desc);
	}

}