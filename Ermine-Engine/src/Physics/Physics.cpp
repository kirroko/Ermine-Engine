/* Start Header ************************************************************************/
/*!
\file       Physics.cpp
\author     Tan Si Han, t.sihan, 2301264, t.sihan\@digipen.edu
\date       Sept 02, 2025
\brief      This file contains the definition of the Physics structure.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "Physics.h"
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include "MathVector.h"
#include <iostream>
using namespace std;

namespace Ermine
{
    // ------------------ Layer & Filter Implementations ------------------
    namespace Layers
    {
        static constexpr ObjectLayer NON_MOVING = 0;
        static constexpr ObjectLayer MOVING = 1;
        static constexpr ObjectLayer NUM_LAYERS = 2;
    };

    namespace BroadPhaseLayers
    {
        static constexpr BroadPhaseLayer NON_MOVING(0);
        static constexpr BroadPhaseLayer MOVING(1);
        static constexpr uint NUM_LAYERS(2);
    };

    class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
    {
    public:
        virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
        {
            switch (inObject1)
            {
            case Layers::NON_MOVING: return inObject2 == Layers::MOVING;
            case Layers::MOVING:     return true;
            default: JPH_ASSERT(false); return false;
            }
        }
    };

    class BPLayerInterfaceImpl : public BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl()
        {
            mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        virtual uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }
        virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
        {
            JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
            return mObjectToBroadPhase[inLayer];
        }

    private:
        BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
    };

    class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
    {
    public:
        virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
        {
            switch (inLayer1)
            {
            case Layers::NON_MOVING: return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:     return true;
            default: JPH_ASSERT(false); return false;
            }
        }
    };

    // ------------------ Listener Implementations ------------------
    class MyContactListener : public ContactListener
    {
    public:
        virtual ValidateResult OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg, const CollideShapeResult&) override { return ValidateResult::AcceptAllContactsForThisBodyPair; }
        virtual void OnContactAdded(const Body&, const Body&, const ContactManifold&, ContactSettings&) override {}
        virtual void OnContactPersisted(const Body&, const Body&, const ContactManifold&, ContactSettings&) override {}
        virtual void OnContactRemoved(const SubShapeIDPair&) override {}
    };

    class MyBodyActivationListener : public BodyActivationListener
    {
    public:
        virtual void OnBodyActivated(const BodyID&, uint64) override {}
        virtual void OnBodyDeactivated(const BodyID&, uint64) override {}
    };

    // ------------------ Physics Class ------------------
    Physics::Physics()
        : mTempAllocator(10 * 1024 * 1024),  // 10 MB
        mJobSystem(cMaxPhysicsJobs, cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1)
    {
        // Allocate filter / listener objects
        mBroadPhaseLayerInterface = new BPLayerInterfaceImpl();
        mObjectVsBroadPhaseLayerFilter = new ObjectVsBroadPhaseLayerFilterImpl();
        mObjectLayerPairFilter = new ObjectLayerPairFilterImpl();
        mBodyActivationListener = new MyBodyActivationListener();
        mContactListener = new MyContactListener();
    }

    Physics::~Physics()
    {
        Shutdown();

        delete mBroadPhaseLayerInterface;
        delete mObjectVsBroadPhaseLayerFilter;
        delete mObjectLayerPairFilter;
        delete mBodyActivationListener;
        delete mContactListener;
    }

    void Physics::Init()
    {
        RegisterDefaultAllocator();

        Factory::sInstance = new Factory();
        RegisterTypes();

        const uint cMaxBodies = 1024;
        const uint cNumBodyMutexes = 0;
        const uint cMaxBodyPairs = 1024;
        const uint cMaxContactConstraints = 1024;

        mPhysicsSystem.Init(
            cMaxBodies,
            cNumBodyMutexes,
            cMaxBodyPairs,
            cMaxContactConstraints,
            *mBroadPhaseLayerInterface,
            *mObjectVsBroadPhaseLayerFilter,
            *mObjectLayerPairFilter
        );

        mPhysicsSystem.SetGravity(JPH::Vec3(0.0f, -0.981f, 0.0f));
        mPhysicsSystem.SetBodyActivationListener(mBodyActivationListener);
        mPhysicsSystem.SetContactListener(mContactListener);
        mPhysicsSystem.OptimizeBroadPhase();
    }

    void Physics::Shutdown()
    {
        UnregisterTypes();
        delete Factory::sInstance;
        Factory::sInstance = nullptr;
    }

    void Physics::Update(float deltaTime)
    {
        mPhysicsSystem.Update(deltaTime, 1, &mTempAllocator, &mJobSystem);

        auto& bodyInterface = mPhysicsSystem.GetBodyInterface();
        for (auto& [entity, rigidBody] : mEntityToBody)
        {
            JPH::RMat44 transform = bodyInterface.GetWorldTransform(rigidBody);

            Transform& t = ECS::GetInstance().GetComponent<Transform>(entity);
            t.position = Vec3(
                transform.GetTranslation().GetX(),
                transform.GetTranslation().GetY(),
                transform.GetTranslation().GetZ()
            );

            // Rotation too
            JPH::Quat rot = transform.GetRotation().GetQuaternion();
			t.rotation.w = rot.GetW();
			t.rotation.x = rot.GetX();
            t.rotation.y = rot.GetY();
            t.rotation.z = rot.GetZ();
        }
    }

    //BodyID Physics::CreateStaticBox(const JPH::Vec3& halfExtents, const RVec3& position)
    //{
    //    BoxShapeSettings settings(halfExtents);
    //    settings.SetEmbedded();
    //    ShapeRefC shape = settings.Create().Get();
    //    BodyCreationSettings bodySettings(shape, position, Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);
    //    Body* body = mPhysicsSystem.GetBodyInterface().CreateBody(bodySettings);
    //    mPhysicsSystem.GetBodyInterface().AddBody(body->GetID(), EActivation::DontActivate);
    //    return body->GetID();
    //}

    //BodyID Physics::CreateDynamicSphere(float radius, const RVec3& position, const JPH::Vec3& initialVelocity)
    //{
    //    BodyCreationSettings settings(new SphereShape(radius), position, Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
    //    BodyID bodyID = mPhysicsSystem.GetBodyInterface().CreateAndAddBody(settings, EActivation::Activate);
    //    mPhysicsSystem.GetBodyInterface().SetLinearVelocity(bodyID, initialVelocity);
    //    return bodyID;
    //}

    //void Physics::CreatePhysicsBox(const Ermine::Vec3& position, const Ermine::Vec3& size, float mass)
    //{
    //    // 1. Create an ECS entity
    //    auto entity = ECS::GetInstance().CreateEntity();

    //    // 2. Add Transform
    //    ECS::GetInstance().AddComponent(entity, Transform(position, Quaternion(), size));

    //    // 3. Add Mesh
    //    ECS::GetInstance().AddComponent(entity, graphics::GeometryFactory::CreateCube(size.x, size.y, size.z));

    //    // 4. Add Material (optional, use existing shader/texture)
    //    auto shader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/vertex.glsl", "../Resources/Shaders/fragment.glsl");
    //    auto texture = AssetManager::GetInstance().LoadTexture("../Resources/Textures/greybox_grey_grid.png");
    //    auto material = std::make_unique<graphics::Material>(shader);
    //    material->LoadTemplate(graphics::MaterialTemplates::PBR_WHITE());
    //    if (texture && texture->IsValid())
    //        material->SetTexture("materialAlbedoMap", texture);
    //    ECS::GetInstance().AddComponent(entity, Material(std::move(material)));

    //    // 5. Create Jolt Physics box shape
    //    ObjectLayer layer = mass > 0 ? Layers::MOVING : Layers::NON_MOVING;
    //    JPH::BodyCreationSettings bodySettings(
    //        new JPH::BoxShape(JPH::Vec3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f)), // half extents
    //        JPH::Vec3(position.x, position.y, position.z),
    //        JPH::Quat::sIdentity(),
    //        mass > 0 ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static,
    //        layer
    //    );
    //    if (mass > 0)
    //        bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    //    bodySettings.mMassPropertiesOverride.mMass = mass;
    //    // 6. Create body and add it to physics
    //    JPH::Body* body = mPhysicsSystem.GetBodyInterface().CreateBody(bodySettings);
    //    mPhysicsSystem.GetBodyInterface().AddBody(body->GetID(), JPH::EActivation::Activate);

    //    // 7. Optionally store body pointer or ID in a component if needed
    //    mEntityToBody[entity] = body->GetID();
    //}

    void Physics::UpdatePhysicList()
    {
        auto& ecs = ECS::GetInstance();

        for (auto entity : m_Entities)
        {
            if (!ecs.HasComponent<Transform>(entity) ||
                !ecs.HasComponent<PhysicComponent>(entity))
                continue;

            auto& t = ecs.GetComponent<Transform>(entity);
            auto& p = ecs.GetComponent<PhysicComponent>(entity);

            if (p.bodyID != JPH::BodyID(JPH::BodyID::cInvalidBodyID))
                continue;

            // Determine the layer
            ObjectLayer layer = (p.motionType == JPH::EMotionType::Dynamic) ? Layers::MOVING : Layers::NON_MOVING;

            // Create the correct shape
            JPH::Shape* shape = nullptr;
            switch (p.shapeType)
            {
            case ShapeType::Box:
                shape = new JPH::BoxShape(JPH::Vec3(t.scale.x * 0.5f, t.scale.y * 0.5f, t.scale.z * 0.5f));
                break;
            case ShapeType::Sphere:
                shape = new JPH::SphereShape(t.scale.x * 0.5f); // radius
                break;
            case ShapeType::Capsule:
                shape = new JPH::CapsuleShape(t.scale.y * 0.5f, t.scale.x * 0.5f); // half height, radius
                break;
            case ShapeType::CustomMesh:
                std::vector<JPH::Vec3> jphVerts;
                jphVerts.reserve(p.customMeshVertices.size());
                for (const auto& v : p.customMeshVertices)
                    jphVerts.emplace_back(v.x, v.y, v.z);

                // Create settings
                JPH::ConvexHullShapeSettings settings(jphVerts.data(), jphVerts.size());

                // Create shape from settings
                shape = settings.Create().Get();
                break;
            //case ShapeType::Compound:
            //    shape = BuildCompoundShape(p); // not implemented
            //    break;
            }

            // Create body settings
            JPH::BodyCreationSettings settings(
                shape,
                JPH::Vec3(t.position.x, t.position.y, t.position.z),
                JPH::Quat(t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w),
                p.motionType,
                layer
            );

            if (p.motionType == JPH::EMotionType::Dynamic)
            {
                settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
                settings.mMassPropertiesOverride.mMass = p.mass;
            }

            auto& bodyInterface = mPhysicsSystem.GetBodyInterface();
            JPH::Body* body = bodyInterface.CreateBody(settings);
            bodyInterface.AddBody(body->GetID(), JPH::EActivation::Activate);

            p.bodyID = body->GetID();
            mEntityToBody[entity] = body->GetID();
        }

    }

}