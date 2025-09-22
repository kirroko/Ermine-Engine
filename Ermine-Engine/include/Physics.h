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
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include "ECS.h"
#include "Components.h"
#include "GeometryFactory.h"
#include "AssetManager.h"


using namespace JPH;
namespace Ermine
{
    class Physics
    {
    public:
        Physics();
        ~Physics();

        void Init();
        void Shutdown();
        void Update(float deltaTime);

        // Body creation
        BodyID CreateStaticBox(const JPH::Vec3& halfExtents, const RVec3& position);
        BodyID CreateDynamicSphere(float radius, const RVec3& position, const JPH::Vec3& initialVelocity);
        void CreatePhysicsBox(const Ermine::Vec3& position, const Ermine::Vec3& size, float mass);

        BodyInterface& GetBodyInterface() { return mPhysicsSystem.GetBodyInterface(); }

    private:
        // --- Important: allocator first, job system second, physics system third ---
        TempAllocatorImpl       mTempAllocator;
        JobSystemThreadPool     mJobSystem;
        PhysicsSystem           mPhysicsSystem;

        // Layer and filter interfaces
        class BPLayerInterfaceImpl* mBroadPhaseLayerInterface;
        class ObjectVsBroadPhaseLayerFilterImpl* mObjectVsBroadPhaseLayerFilter;
        class ObjectLayerPairFilterImpl* mObjectLayerPairFilter;

        // Listeners
        class MyBodyActivationListener* mBodyActivationListener;
        class MyContactListener* mContactListener;

        std::unordered_map<EntityID, JPH::BodyID> mEntityToBody;

        void SetupLayers();
    };
    extern Physics* gPhysics;
    //extern Physics gPhysics;
}