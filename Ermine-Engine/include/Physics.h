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
#include "ECS.h"
#include "Components.h"
#include "GeometryFactory.h"
#include "AssetManager.h"


using namespace JPH;
namespace Ermine
{
    class Physics : public System
    {
    public:
        Physics();
        ~Physics();

        void Init();
        void Shutdown();
        void Update(float deltaTime);

        // Body creation
        //BodyID CreateStaticBox(const JPH::Vec3& halfExtents, const RVec3& position);
        //BodyID CreateDynamicSphere(float radius, const RVec3& position, const JPH::Vec3& initialVelocity);
        //void CreatePhysicsBox(const Ermine::Vec3& position, const Ermine::Vec3& size, float mass);

        void UpdatePhysicList();

        BodyInterface& GetBodyInterface() { return mPhysicsSystem.GetBodyInterface(); }

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

        void SetupLayers();
    };
}