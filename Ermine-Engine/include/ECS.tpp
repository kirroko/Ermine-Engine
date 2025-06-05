/* Start Header ************************************************************************/
/*!
\file       ECS.tpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       Sep 15, 2024
\brief      The ECS will coordinator between the Component, Entity and System managers.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

namespace Ermine
{
	/**
	* @brief Register a component type with the ECS
	* @tparam T The component type to register
	*/
	template <typename T>
	void ECS::RegisterComponent()
	{
		m_ComponentManager->RegisterComponent<T>();
	}

	/**
	* @brief Add a component to an entity, also update each system m_Entity list
	* @tparam T The component type to add
	* @param entity The entity to add the component to
	* @param component The component to add
	*/
	template <typename T>
	void ECS::AddComponent(EntityID entity, T component)
	{
		m_ComponentManager->AddComponent<T>(entity, component);

		auto signature = m_EntityManager->GetSignature(entity);
		signature.set(m_ComponentManager->GetComponentType<T>(), true);
		m_EntityManager->SetSignature(entity, signature);

		m_SystemManager->EntitySignatureChanged(entity, signature);
	}

	/**
	 * @brief Remove a component from an entity
	 * @tparam T The component type to remove
	 * @param entity The entity to remove the component from
	 */
	template <typename T>
	void ECS::RemoveComponent(EntityID entity) const
	{
		m_ComponentManager->RemoveComponent<T>(entity);

		auto signature = m_EntityManager->GetSignature(entity);
		signature.set(m_ComponentManager->GetComponentType<T>(), false);
		m_EntityManager->SetSignature(entity, signature);

		m_SystemManager->EntitySignatureChanged(entity, signature);
	}

	/**
	 * @brief Get a reference to a component of type T for an entity
	 * @tparam T The component type to get
	 * @param entity The entity to get the component from
	 * @return A reference to the component
	 */
	template <typename T>
	T& ECS::GetComponent(EntityID entity) const
	{
		return m_ComponentManager->GetComponent<T>(entity);
	}

	/**
	 * @brief Get a component type ID
	 * @tparam T The component type to get the ID of
	 * @return The component type ID
	 */
	template <typename T>
	ComponentTypeID ECS::GetComponentType() const
	{
		return m_ComponentManager->GetComponentType<T>();
	}

	/**
	 * @brief Register a system with the ECS
	 * @tparam T The system type to register
	 */
	template <typename T>
	void ECS::RegisterSystem() const
	{
		return m_SystemManager->RegisterSystem<T>();
	}

	/**
	 * @brief Get a system from the ECS
	 * @tparam T The system type to get
	 * @return A shared pointer to the system
	 */
	template <typename T>
	std::shared_ptr<T> ECS::GetSystem() const
	{
		return m_SystemManager->GetSystem<T>();
	}

	/**
	 * @brief Set the signature of a system
	 * @tparam T The system type to set the signature of
	 */
	template <typename T>
	void ECS::SetSystemSignature(SignatureID signature) const
	{
		m_SystemManager->SetSystemSignature<T>(signature);
	}
}