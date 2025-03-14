/* Start Header ************************************************************************/
/*!
\file       ECS.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       Sep 15, 2024
\brief      This file contains the definition of the member function of ECS.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "ECS.h"

namespace Ermine
{
	/**
	 * @brief Initialize the ECS
	 */
	void ECS::Init()
	{
		m_ComponentManager = std::make_unique<ComponentManager>();
		m_EntityManager = std::make_unique<EntityManager>();
		m_SystemManager = std::make_unique<SystemManager>();
	}

	/**
	 * @brief Create an entity
	 * @return The ID of the created entity
	 */
	EntityID ECS::CreateEntity() const
	{
		return m_EntityManager->CreateEntity();
	}

	/**
	 * @brief Destroy an entity
	 * @param entity The entity to destroy
	 */
	void ECS::DestroyEntity(EntityID entity) const
	{
		m_EntityManager->DestroyEntity(entity);
		m_ComponentManager->EntityDestroyed(entity);
		m_SystemManager->EntityDestroyed(entity);
	}

	/**
	 * @brief Reload the entity manager
	 */
	void ECS::ReloadEntityManager()
	{
		m_EntityManager.reset(new EntityManager());
	}

	/**
	 * @brief Clone an entity
	 * @param entity The entity to clone
	 * @return The ID of the new entity
	 */
	EntityID ECS::CloneEntity(EntityID entity)
	{
		EntityID newEntity = m_EntityManager->CreateEntity();

		SignatureID originalSignature = m_EntityManager->GetSignature(entity);
		m_EntityManager->SetSignature(newEntity, originalSignature);

		// TODO: Keep updating components as the list grows

		return newEntity;
	}

	unsigned long int ECS::GetLivingEntityCount() const
	{
		return m_EntityManager->GetLivingEntityCount();
	}
}
// 0x4B45414E