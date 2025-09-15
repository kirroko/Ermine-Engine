/* Start Header ************************************************************************/
/*!
\file       Component.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       Sep 15, 2024
\brief      To manage all the the different components that needed to be added or removed.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include "Registry.h"

namespace Ermine
{
	// Runtime descriptor
	struct ComponentDescriptor
	{
		std::string name; // Runtime name/key
		ComponentTypeID typeID{}; // Dense index used in signatures
		size_t size{}; // sizeof(component)
		std::function<bool(EntityID)> has; // Check if an entity has this component
		std::function<void(EntityID, EntityID)> clone; // Clone component from src to dst
	};

	// The component manager class that manages all the different component arrays that are attached to the component type
	class ComponentManager
	{
		// Here is where we store all the known component types whose ID = ComponentType (i.e transform, sprite, etc)
		std::unordered_map<std::string, ComponentTypeID> m_ComponentTypes{};

		// This is where we store all the component arrays () that is attached to ComponentType's name
		std::unordered_map<std::string, std::shared_ptr<IComponentArray>> m_ComponentArrays{}; // Map from type string pointer to a component array

		// Map from type index to runtime component name
		std::unordered_map<std::type_index, std::string> m_TypeIndexToName{};

		// Runtime descriptors
		std::unordered_map<std::string, ComponentDescriptor> m_Descriptors{};

		// The component type to be assigned to the next registered component - starting a 0
		ComponentTypeID m_NextComponentType{};

		// Convenience function to get the statically casted pointer to the ComponentArray of type T.
		template <typename T>
		std::shared_ptr<ComponentArray<T>> GetComponentArray();

	public:
		// Register a component
		template <typename T>
		void RegisterComponent();

		// Register with custom name
		template <typename T>
		void RegisterComponent(std::string_view customName);

		// Register with custom name and a custom clone function for special components
		// that requires special handling during duplication (e.g Script component)
		template <typename T, typename CloneFn>
		void RegisterComponent(std::string_view customName, CloneFn customClone);

		// Get the component type ID of a component
		template <typename T>
		ComponentTypeID GetComponentType();

		/**
		 * @brief Get the component types
		 * @return A reference to the component types
		 */
		std::unordered_map<std::string, ComponentTypeID>& GetComponentTypes() { return m_ComponentTypes; }

		// Add a component to an entity
		template<typename T>
		void AddComponent(EntityID entity, T component);

		// Remove a component from an entity
		template<typename T>
		void RemoveComponent(EntityID entity);

		// Get a reference to a component of type T for an entity
		template<typename T>
		T& GetComponent(EntityID entity);

		// Notify all component arrays that an entity has been destroyed
		void EntityDestroyed(EntityID entity) const;

		// Verify whether an entity has a particular component 
		template<typename T>
		bool HasComponent(EntityID entity) {
			// Retrieve the component array for type T
			auto componentArray = GetComponentArray<T>();
			// Return true if the entity is present in the component array
			return componentArray->HasEntity(entity);
		}

		// Iterate over all registered component descriptors
		template<typename T>
		void ForEachComponentType(T&& fn) const;

		bool HasComponent(EntityID entity, std::string_view name) const;

		void CloneAllComponents(EntityID src, EntityID dst);

		std::vector<std::string> GetComponentNames(EntityID entity) const;

		const ComponentDescriptor* GetDescriptor(std::string_view name) const;
	};
}
#include "Component.tpp"
// 0x4B45414E
