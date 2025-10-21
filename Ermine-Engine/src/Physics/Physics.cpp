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
#include "PhysicDebugRenderer.h"
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Core/Color.h>
#include <iostream>

#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyManager.h>
#include <Jolt/Renderer/DebugRenderer.h>

#include <Jolt/Physics/Collision/CollisionCollector.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>



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

    // Defines collision rules between object layers
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

    // Maps object layers to broadphase layers (used for efficient collision detection)
    class BPLayerInterfaceImpl : public BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl()
        {
            mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        virtual uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }
        
        // Returns the broadphase layer corresponding to an object layer
        virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
        {
            JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
            return mObjectToBroadPhase[inLayer];
        }

    private:
        BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
    };

    // Defines object vs broadphase layer collision rules
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
    // Handles contact events (collisions)
    class MyContactListener : public ContactListener
    {
    public:
        // Called to validate a potential collision
        virtual ValidateResult OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg, const CollideShapeResult&) override 
        { 
            (void)inBody1; (void)inBody2;
            return ValidateResult::AcceptAllContactsForThisBodyPair;
        }

        // Called when contact begins
        virtual void OnContactAdded(const Body&, const Body&, const ContactManifold&, ContactSettings&) override {}
        
        // Called when contact persists across frames
        virtual void OnContactPersisted(const Body&, const Body&, const ContactManifold&, ContactSettings&) override {}
        
        // Called when contact ends
        virtual void OnContactRemoved(const SubShapeIDPair&) override {}
    };

    // Handles activation/deactivation of bodies (e.g., sleeping/waking up)
    class MyBodyActivationListener : public BodyActivationListener
    {
    public:
        virtual void OnBodyActivated(const BodyID&, uint64) override {}
        virtual void OnBodyDeactivated(const BodyID&, uint64) override {}
    };

    // ------------------ Physics Class ------------------
    /*!*************************************************************************
    \brief
     Constructor for the Physics class. Initializes temporary memory
     allocators, job system, and sets up filter and listener objects.
    ***************************************************************************/
    Physics::Physics()
        : mTempAllocator(10 * 1024 * 1024),  // Allocate 10 MB for temporary physics data
        mJobSystem(cMaxPhysicsJobs, cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1)
    {
        // Create filter and listener objects for collision handling
        mBroadPhaseLayerInterface = new BPLayerInterfaceImpl();
        mObjectVsBroadPhaseLayerFilter = new ObjectVsBroadPhaseLayerFilterImpl();
        mObjectLayerPairFilter = new ObjectLayerPairFilterImpl();
        mBodyActivationListener = new MyBodyActivationListener();
        mContactListener = new MyContactListener();
        wireframe = false;
    }

    /*!*************************************************************************
      \brief
        Destructor for the Physics class. Shuts down the physics system and
        deallocates filter/listener objects.
    ***************************************************************************/
    Physics::~Physics()
    {
        Shutdown();

        // Clean up allocated filter/listener objects
        delete mBroadPhaseLayerInterface;
        delete mObjectVsBroadPhaseLayerFilter;
        delete mObjectLayerPairFilter;
        delete mBodyActivationListener;
        delete mContactListener;
    }

    /*!*************************************************************************
      \brief
        Initializes the Jolt physics system, allocates core objects, and sets up
        gravity, listeners, and broadphase optimizations.
    ***************************************************************************/
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

        JPH::DebugRenderer::sInstance = mDebugRenderer.get();

    }

    /*!*************************************************************************
      \brief
        Shuts down the physics system, clears entity-body mappings, and deletes
        the physics factory.
    ***************************************************************************/
    void Physics::Shutdown()
    {
        // unregister types and clear bodies
        UnregisterTypes();
        mEntityToBody.clear();

        // Clear the global debug renderer so Jolt won't hold dangling pointer
        //JPH::DebugRenderer::sInstance = nullptr;
        mDebugRenderer.reset();

        delete Factory::sInstance;
        Factory::sInstance = nullptr;
    }

    /*!*************************************************************************
      \brief
        Updates the physics system for the given timestep and synchronizes ECS
        transforms with the latest physics body positions and rotations.
      \param[in] deltaTime
        Time elapsed since the last frame, in seconds.
    ***************************************************************************/
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

    //TEMP WILL BE REMOVE
    /*
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
    */

    /*!*************************************************************************
      \brief
        Rebuilds the physics body list from ECS entities. Removes old bodies,
        creates shapes based on component data, and registers them with the
        physics system.
    ***************************************************************************/
    void Physics::UpdatePhysicList()
    {
        auto& ecs = ECS::GetInstance();
        mEntityToBody.clear();

        auto& bodyInterface = mPhysicsSystem.GetBodyInterface();

        for (auto entity : m_Entities)
        {
            if (!ecs.HasComponent<Transform>(entity) || !ecs.HasComponent<PhysicComponent>(entity))
                continue;

            auto& t = ecs.GetComponent<Transform>(entity);
            auto& p = ecs.GetComponent<PhysicComponent>(entity);

            // Remove old body if exists
            if (p.body)
            {
                bodyInterface.RemoveBody(p.bodyID);
                bodyInterface.DestroyBody(p.bodyID);
                p.body = nullptr;
                p.bodyID = JPH::BodyID(JPH::BodyID::cInvalidBodyID);
            }

            // Skip zero-scale transforms
            if (t.scale.x <= 0 || t.scale.y <= 0 || t.scale.z <= 0)
                continue;

            // Determine layer
            ObjectLayer layer = (p.motionType == JPH::EMotionType::Dynamic) ? Layers::MOVING : Layers::NON_MOVING;
            if ((p.motionType == JPH::EMotionType::Dynamic || p.motionType == JPH::EMotionType::Kinematic) && p.mass <= 0.0f)
                p.mass = 1.0f;

            // Create shape
            JPH::Shape* shape = nullptr;
            switch (p.shapeType)
            {
            case ShapeType::Box:
            {
                Vec3 halfExtent = { t.scale.x * 0.5f, t.scale.y * 0.5f, t.scale.z * 0.5f };
                constexpr float minSize = 0.01f;
                halfExtent.x = std::max(halfExtent.x, minSize);
                halfExtent.y = std::max(halfExtent.y, minSize);
                halfExtent.z = std::max(halfExtent.z, minSize);

                // Convex radius must be smaller than all half extents
                float convexRadius = 0.05f;
                convexRadius = std::min(convexRadius,
                std::min({ halfExtent.x, halfExtent.y, halfExtent.z }) * 0.5f);

                shape = new JPH::BoxShape(JPH::Vec3(halfExtent.x, halfExtent.y, halfExtent.z), convexRadius);
                break;
            }
            case ShapeType::Sphere:
                shape = new JPH::SphereShape(t.scale.x); //for our current sphere
                break;
            case ShapeType::Capsule:
                shape = new JPH::CapsuleShape(t.scale.y * 0.5f, t.scale.x * 0.5f);
                break;
            case ShapeType::CustomMesh:

                //check the parent object if got model
                if (ecs.HasComponent<ModelComponent>(entity))
                {
                    auto& modelComp = ecs.GetComponent<ModelComponent>(entity);
                    auto model = modelComp.m_model;
                    if (!model) break;

                    // Fill the custom mesh vertices for physics
                    p.customMeshVertices = model->GetMeshVertices();
                }
                else
                {
                    continue;
                }

                if (p.customMeshVertices.empty())
                    continue;

                const size_t vertexSkip = 5;
                JPH::Array<JPH::Vec3> vertd;
                JPH::Array<JPH::Float3> verts;
                verts.reserve(p.customMeshVertices.size());
                for (size_t i = 0; i < p.customMeshVertices.size(); i ++)
                {
                    const auto& v = p.customMeshVertices[i];
                    if (i % 12 == 0)
                    {
                        verts.push_back(JPH::Float3(v.x * t.scale.x, v.y * t.scale.y, v.z * t.scale.z));
                    }
                    vertd.push_back(JPH::Vec3(v.x * t.scale.x, v.y * t.scale.y, v.z * t.scale.z));
                }

                JPH::RefConst<JPH::Shape> shapeRef;

                if (p.motionType == JPH::EMotionType::Dynamic || p.motionType == JPH::EMotionType::Kinematic)
                {
                    // --- Dynamic mesh: convert to ConvexHullShape or CompoundShape ---
                    if (verts.size() < 4)
                    {
                        std::cerr << "[Physics] Not enough vertices for ConvexHullShape.\n";
                        continue;
                    }

                    // Option 1: Single convex hull
                    JPH::ConvexHullShapeSettings hullSettings(vertd);
                    ShapeSettings::ShapeResult result = hullSettings.Create();
                    if (result.HasError())
                    {
                        std::cerr << "[Physics] ConvexHullShape creation failed: "
                            << result.GetError().c_str() << std::endl;
                        continue;
                    }
                    shapeRef = result.Get();

                    // --- Option 2 (recommended for complex FBX): Convex decomposition ---
                    // std::vector<JPH::ConvexHullShapeSettings*> convexParts;
                    // Split FBX vertices into smaller convex hulls (external tool/library)
                    // JPH::CompoundShapeSettings compoundSettings(convexParts);
                    // shapeRef = compoundSettings.Create();
                }
                else
                {
                    // --- Static mesh: MeshShape ---
                    JPH::Array<JPH::IndexedTriangle> triangles;
                    for (uint32_t i = 0; i + 2 < verts.size(); i += 3)
                        triangles.push_back(JPH::IndexedTriangle(i, i + 1, i + 2));

                    JPH::MeshShapeSettings meshSettings(verts, triangles);
                    meshSettings.mActiveEdgeCosThresholdAngle = 0.999f;

                    ShapeSettings::ShapeResult result = meshSettings.Create();
                    if (result.HasError())
                    {
                        std::cerr << "[Physics] MeshShape creation failed: "
                            << result.GetError().c_str() << std::endl;
                        continue;
                    }

                    shapeRef = result.Get();
                }

                p.shapeRef = shapeRef;
                shape = const_cast<JPH::Shape*>(shapeRef.GetPtr());
                break;
            }

            if (!shape) continue; // Skip invalid shapes

            // Create body settings
            JPH::BodyCreationSettings bodySettings(
                shape,
                JPH::Vec3(t.position.x, t.position.y, t.position.z),
                JPH::Quat(t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w),
                p.motionType,
                layer
            );

            if (p.motionType == JPH::EMotionType::Dynamic || p.motionType == JPH::EMotionType::Kinematic)
            {
                bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
                bodySettings.mMassPropertiesOverride.mMass = p.mass;
            }

            // Create body
            JPH::Body* body = bodyInterface.CreateBody(bodySettings);
            if (!body) continue;

            bodyInterface.AddBody(body->GetID(), JPH::EActivation::Activate);

            // Store in component
            p.body = body;
            p.bodyID = body->GetID();
            mEntityToBody[entity] = p.bodyID;
        }
    }

    /*!*************************************************************************
      \brief
        Retrieves the physics BodyID corresponding to an ECS entity.
      \param[in] objectID
        The ECS entity ID to query.
      \return
        A valid BodyID if found, otherwise an invalid BodyID.
    ***************************************************************************/
    JPH::BodyID Physics::GetBodyID(EntityID objectID)
    {
        auto it = mEntityToBody.find(objectID);
        return (it != mEntityToBody.end())
            ? it->second
            : JPH::BodyID(JPH::BodyID::cInvalidBodyID);
    }

    void Physics::DrawDebug()
    {

#ifdef JPH_DEBUG_RENDERER
        if (!mDebugRenderer) // nothing to draw
            return;

        // If your MyDebugRenderer batches to the engine’s Renderer,
        // make sure you started a frame outside (see step 3).
        BodyManager::DrawSettings ds{};
        ds.mDrawShape = false;  // solid off
        ds.mDrawShapeWireframe = true;   // wireframe on
        ds.mDrawBoundingBox = true;

        // (optional) ds.mDrawConstraints = true; etc.

        mPhysicsSystem.DrawBodies(ds, mDebugRenderer.get());
        // (optional) mPhysicsSystem->DrawConstraints(dbg);
#endif
    }

    void Physics::DrawDebugPhysics()
    {
#ifdef JPH_DEBUG_RENDERER

        // 1) Let Jolt draw primitives via DrawLine (box/sphere/capsule/constraints)
        JPH::BodyManager::DrawSettings ds{};
        //ds.mDrawBoundingBox = true;
        ds.mDrawShape = false;   // no solid fill
        ds.mDrawShapeWireframe = true;    // wireframe only
        // ds.mDrawConstraints = true;    // optional
        mPhysicsSystem.DrawBodies(ds, mDebugRenderer.get());

        // 2) Wireframe for custom meshes via GetTriangles
        JPH::BodyIDVector bodies;
        mPhysicsSystem.GetBodies(bodies);

        const JPH::BodyLockInterface& bli = mPhysicsSystem.GetBodyLockInterface();
        for (JPH::BodyID id : bodies)
        {
            JPH::BodyLockRead lock(bli, id);
            if (!lock.SucceededAndIsInBroadPhase()) continue;

            const JPH::Body& body = lock.GetBody();

            // Collect all leaf shapes (transformed)
            JPH::AllHitCollisionCollector<JPH::TransformedShapeCollector> collector;
            body.GetTransformedShape().CollectTransformedShapes(body.GetWorldSpaceBounds(), collector);

            // Pick a color (same idea as Jolt sample)
            JPH::Color color;
            switch (body.GetMotionType())
            {
            case JPH::EMotionType::Static:    color = JPH::Color::sGrey; break;
            case JPH::EMotionType::Kinematic: color = JPH::Color::sGreen; break;
            case JPH::EMotionType::Dynamic:   color = JPH::Color::sGetDistinctColor(body.GetID().GetIndex()); break;
            default:                          color = JPH::Color::sWhite; break;
            }

            for (const JPH::TransformedShape& ts : collector.mHits)
            {
                // Iterate triangles of this leaf shape
                JPH::Shape::GetTrianglesContext ctx;
                ts.mShape->GetTrianglesStart(ctx, JPH::AABox::sBiggest(), JPH::Vec3::sZero(),
                    JPH::Quat::sIdentity(), JPH::Vec3::sOne());

                // World transform for this leaf (includes body + shape local)
                JPH::Vec3 scale = ts.GetShapeScale();
                JPH::RMat44 matrix = ts.GetCenterOfMassTransform().PreScaled(scale);

                constexpr int cMax = 1000;
                JPH::Float3 verts[3 * cMax];

                for (;;)
                {
                    int triCount = ts.mShape->GetTrianglesNext(ctx, cMax, verts);
                    if (triCount == 0) break;

                    // Emit each triangle’s 3 edges as lines
                    for (int t = 0; t < triCount; ++t)
                    {
                        const JPH::Float3& p0 = verts[3 * t + 0];
                        const JPH::Float3& p1 = verts[3 * t + 1];
                        const JPH::Float3& p2 = verts[3 * t + 2];

                        JPH::RVec3 a((double)p0.x, (double)p0.y, (double)p0.z);
                        JPH::RVec3 b((double)p1.x, (double)p1.y, (double)p1.z);
                        JPH::RVec3 c((double)p2.x, (double)p2.y, (double)p2.z);

                        a = matrix * a;
                        b = matrix * b;
                        c = matrix * c;

                        mDebugRenderer->DrawLine(a, b, color);
                        mDebugRenderer->DrawLine(b, c, color);
                        mDebugRenderer->DrawLine(c, a, color);
                    }
                }
            }
        }
#endif
    }

    void Physics::AttachDebugRenderer(std::shared_ptr<MyDebugRenderer> renderer)
    {
        mDebugRenderer = std::move(renderer);
    }
}