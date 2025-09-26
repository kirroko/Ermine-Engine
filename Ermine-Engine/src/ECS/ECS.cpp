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
	 * @brief Shutdown the ECS, and release all resources via their destructors
	 */
	void ECS::Shutdown()
	{
		EE_CORE_TRACE("Shutting down ECS...");
		m_SystemManager.reset();
		m_ComponentManager.reset();
		m_EntityManager.reset();
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

	EntityID ECS::CloneEntity(EntityID entity)
	{
		EntityID newEntity = m_EntityManager->CreateEntity();

		m_ComponentManager->CloneAllComponents(entity, newEntity);

		SignatureID originalSignature = m_EntityManager->GetSignature(entity);
		m_EntityManager->SetSignature(newEntity, originalSignature);
		m_SystemManager->EntitySignatureChanged(newEntity, originalSignature);

		return newEntity;
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
  //  EntityID ECS::CloneEntity(EntityID entity)
  //  {
		//EntityID newEntity = m_EntityManager->CreateEntity();

		//SignatureID originalSignature = m_EntityManager->GetSignature(entity);
		//m_EntityManager->SetSignature(newEntity, originalSignature);

	 //   // Iterate through all possible components
	 //   for (const auto& [componentName, componentType] : m_ComponentManager->GetComponentTypes())
	 //   {
		//	// Check if the entity has the component
		//	if (m_ComponentManager->HasComponent(entity,componentName))
		//	{
		//		// Get the component data from the original entity
		//		auto& originalComponent = m_ComponentManager->GetComponent(entity);

		//		// Add the component to the new entity with the same data
		//		m_ComponentManager->AddComponent(newEntity, originalComponent);
		//	}
	 //   }

		//return newEntity;
  //  }

	unsigned long int ECS::GetLivingEntityCount() const
	{
		return m_EntityManager->GetLivingEntityCount();
	}

	void ECS::ClearEntities()
	{
		std::vector<EntityID> toDestroy;
		toDestroy.reserve(static_cast<size_t>(GetLivingEntityCount()));
		for (EntityID id = 0; id < MAX_ENTITIES; ++id) {
			if (IsEntityValid(id))
				toDestroy.push_back(id);
		}

		for (EntityID id : toDestroy) {
			DestroyEntity(id);
		}

		m_EntityManager = std::make_unique<EntityManager>();
	}

}
// 0x4B45414E