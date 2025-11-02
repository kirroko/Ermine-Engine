/* Start Header ************************************************************************/
/*!
\file       Physics.h
\author     Tan Si Han, t.sihan, 2301264, t.sihan\@digipen.edu
\date       Sept 02, 2025
\brief      This file contains the declaration of the Physics structure.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>  // ConvexHullShape
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>    // CompoundShape
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>

#include "ECS.h"
#include "Components.h"
#include "GeometryFactory.h"
#include "AssetManager.h"
#include "PhysicDebugRenderer.h"
#include <tuple>
#include <queue>

using namespace JPH;
namespace Ermine
{
	struct BodyDrawSettings
	{
		bool mDrawShapeWireframe = true;   // toggle wireframe drawing
		bool mDrawInactiveBodies = true;   // optionally draw inactive bodies
		bool mDrawCenterOfMass = false;    // optional
		bool mDrawBodyAxes = false;        // optional
	};

	class Physics : public System
	{
	public:
		/*!***********************************************************************
		  \brief
			Constructor. Initializes memory allocator, job system, and prepares
			listeners/filters for the physics system.
		*************************************************************************/
		Physics();

		/*!***********************************************************************
		  \brief
			Destructor. Shuts down the physics system and cleans up resources.
		*************************************************************************/
		~Physics();

		/*!***********************************************************************
		  \brief
			Initializes the Jolt physics system, registers types, sets up gravity,
			listeners, and optimizes the broadphase.
		*************************************************************************/
		void Init();

		/*!***********************************************************************
		  \brief
			Shuts down the physics system and clears entity-to-body mappings.
		*************************************************************************/
		void Shutdown();

		/*!***********************************************************************
		  \brief
			Updates the physics simulation for a single frame and synchronizes ECS
			transforms with physics bodies.
		  \param[in] deltaTime
			Time elapsed since the last frame, in seconds.
		*************************************************************************/
		void Update(float deltaTime);

		//place holder, will be remove
		//BodyID CreateStaticBox(const JPH::Vec3& halfExtents, const RVec3& position);
		//BodyID CreateDynamicSphere(float radius, const RVec3& position, const JPH::Vec3& initialVelocity);
		//void CreatePhysicsBox(const Ermine::Vec3& position, const Ermine::Vec3& size, float mass);

		/*!***********************************************************************
		  \brief
			Rebuilds the list of physics bodies from ECS entities. Old bodies are
			removed and new shapes are created and registered in the physics world.
		*************************************************************************/
		void UpdatePhysicList();

		/*!***********************************************************************
		  \brief
			Retrieves the physics body ID associated with a given ECS entity.
		  \param[in] objectID
			The ECS entity ID to query.
		  \return
			A valid BodyID if the entity has a physics body, otherwise an invalid BodyID.
		*************************************************************************/
		JPH::BodyID GetBodyID(EntityID objectID);

		EntityID GetEntityID(JPH::BodyID bodyID);

		/*!***********************************************************************
		  \brief
			Provides access to the Jolt BodyInterface for manual body operations.
		  \return
			Reference to the BodyInterface instance.
		*************************************************************************/
		BodyInterface& GetBodyInterface() { return mPhysicsSystem.GetBodyInterface(); }

		/*!***********************************************************************
		  \brief
			Renders the current physics world using Jolt's debug renderer.
			Typically used for wireframe visualization in the editor.
		*************************************************************************/
		void DrawDebug();

		void DrawDebugPhysics();

		void AttachDebugRenderer(std::shared_ptr<MyDebugRenderer> renderer);

		std::shared_ptr<MyDebugRenderer> mDebugRenderer;

		bool wireframe;

		enum class CollisionEventType : char { Begin, Stay, End };

		void HandleCollisionEvent(const Body& a, const Body& b, CollisionEventType type);

		void HandleCollisionEvent(JPH::BodyID a, JPH::BodyID b, CollisionEventType type);
	
		bool Raycast(const JPH::RVec3& origin, const JPH::RVec3& direction, float maxDistance, JPH::RayCastResult& outResult);

		std::vector<JPH::RayCastResult> RaycastAll(const JPH::RVec3& origin, const JPH::RVec3& direction, float maxDistance);
	private:
		// --- Important: allocator first, job system second, physics system third ---
		JPH::TempAllocatorImpl       mTempAllocator;
		JPH::JobSystemThreadPool     mJobSystem;
		JPH::PhysicsSystem           mPhysicsSystem;

		class BPLayerInterfaceImpl* mBroadPhaseLayerInterface = nullptr;
		class ObjectVsBroadPhaseLayerFilterImpl* mObjectVsBroadPhaseLayerFilter = nullptr;
		class ObjectLayerPairFilterImpl* mObjectLayerPairFilter = nullptr;

		class MyBodyActivationListener* mBodyActivationListener = nullptr;
		class MyContactListener* mContactListener = nullptr;

		std::unordered_map<EntityID, JPH::BodyID> mEntityToBody;

		JPH::BodyManager::DrawSettings mBodyDrawSettings{};

		std::queue<std::tuple<CollisionEventType, EntityID, EntityID, bool>> mCollisionEvent;

		struct PendingPair
		{
			CollisionEventType type;
			JPH::BodyID a;
			JPH::BodyID b;
		};
		std::mutex mPendingMutex;
		std::vector<PendingPair> mPendingPairs;

		void FlushPendingPairsToEntityEvents();

		//not in used
		//void SetupLayers();
	};
}