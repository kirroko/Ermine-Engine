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

#include "Input.h"

#include "EditorGUI.h"
#include "HierarchySystem.h"

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
		virtual void OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold&, ContactSettings&) override
		{
			EE_CORE_INFO("[Physics] Collision Begin");

			ECS::GetInstance().GetSystem<Physics>()->HandleCollisionEvent(inBody1, inBody2, Physics::CollisionEventType::Begin);
		}

		// Called when contact persists across frames
		virtual void OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold&, ContactSettings&) override
		{
			ECS::GetInstance().GetSystem<Physics>()->HandleCollisionEvent(inBody1, inBody2, Physics::CollisionEventType::Stay);
		}

		// Called when contact ends
		virtual void OnContactRemoved(const SubShapeIDPair& inPair) override
		{
			// We can still get body references using BodyLockRead
			EE_CORE_INFO("[Physics] Collision End");
			ECS::GetInstance().GetSystem<Physics>()->HandleCollisionEvent(inPair.GetBody1ID(), inPair.GetBody2ID(), Physics::CollisionEventType::End);
		}
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
		mDebugRenderer.reset();
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

		auto& ecs = ECS::GetInstance();
		auto& bodyInterface = mPhysicsSystem.GetBodyInterface();

		if (editor::EditorGUI::s_state == editor::EditorGUI::SimState::stopped)
		{
			for (auto& [entity, rigidBody] : mEntityToBody)
			{
				if (!ecs.IsEntityValid(entity) ||
					!ecs.HasComponent<PhysicComponent>(entity) ||
					!ecs.HasComponent<Transform>(entity))
					continue;

				auto& p = ecs.GetComponent<PhysicComponent>(entity);
				auto& t = ecs.GetComponent<Transform>(entity);

				bodyInterface.SetPositionAndRotationWhenChanged(
					rigidBody,
					JPH::Vec3(t.position.x, t.position.y, t.position.z),
					JPH::Quat(t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w),
					JPH::EActivation::DontActivate);

				if (p.motionType == JPH::EMotionType::Dynamic)
				{
					bodyInterface.SetLinearVelocity(rigidBody, JPH::Vec3::sZero());
					bodyInterface.SetAngularVelocity(rigidBody, JPH::Vec3::sZero());
				}
			}
			return;
		}

		// ECS -> Physics: Update body transforms from ECS for Kinematic bodies
		for (auto& [entity, rigidBody] : mEntityToBody)
		{
			if (!ecs.IsEntityValid(entity) ||
				!ecs.HasComponent<PhysicComponent>(entity) ||
				!ecs.HasComponent<Transform>(entity))
				continue;

			auto& p = ecs.GetComponent<PhysicComponent>(entity);
			auto& t = ecs.GetComponent<Transform>(entity);

			if (p.motionType == JPH::EMotionType::Kinematic)
			{
				bodyInterface.SetPositionAndRotation(
					rigidBody,
					JPH::Vec3(t.position.x, t.position.y, t.position.z),
					JPH::Quat(t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w),
					JPH::EActivation::Activate);
			}
		}

		FlushPendingPairsToEntityEvents();

		for (; !mCollisionEvent.empty(); mCollisionEvent.pop())
		{
			auto& [type, recipientEntity, otherEntity, sensor] = mCollisionEvent.front();
			if (ecs.IsEntityValid(recipientEntity) && ecs.HasComponent<Script>(recipientEntity))
			{
				auto& scriptComp = ecs.GetComponent<Script>(recipientEntity);
				if (!scriptComp.m_instance)
					continue;

				switch (type)
				{
				case CollisionEventType::Begin:
					scriptComp.m_instance->OnCollisionEnter(otherEntity, sensor);
					break;
				case CollisionEventType::Stay:
					scriptComp.m_instance->OnCollisionStay(otherEntity, sensor);
					break;
				case CollisionEventType::End:
					scriptComp.m_instance->OnCollisionExit(otherEntity, sensor);
					break;
				}
			}
		}

		mPhysicsSystem.Update(deltaTime, 1, &mTempAllocator, &mJobSystem);

		for (auto& [entity, rigidBody] : mEntityToBody)
		{
			if (!ecs.IsEntityValid(entity) ||
				!ecs.HasComponent<PhysicComponent>(entity) ||
				!ecs.HasComponent<Transform>(entity))
				continue;

			auto& p = ecs.GetComponent<PhysicComponent>(entity);
			if (p.motionType == JPH::EMotionType::Static)
				continue;

			auto& t = ecs.GetComponent<Transform>(entity);
			JPH::RMat44 transform = bodyInterface.GetWorldTransform(rigidBody);

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

			auto hierarchySystem = ECS::GetInstance().GetSystem<HierarchySystem>();
			hierarchySystem->MarkDirty(entity);
		}

		//if (Input::IsKeyDown(GLFW_KEY_1))
		//{
		//	auto hits = RaycastAll({ 0, 5, 0 }, { 0, -1, 0 }, 100.0f);
		//}
	}

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

		if (m_Entities.empty())
		{
			ClearPhysicBody();
		}

		for (auto entity : m_Entities)
		{
			if (!ecs.HasComponent<Transform>(entity) || !ecs.HasComponent<PhysicComponent>(entity))
				continue;

			auto& t = ecs.GetComponent<Transform>(entity);
			auto& p = ecs.GetComponent<PhysicComponent>(entity);
			JPH::Vec3 meshsize = JPH::Vec3(1, 1, 1);
			if (ecs.HasComponent<Mesh>(entity))
			{
				meshsize = JPH::Vec3(ecs.GetComponent<Mesh>(entity).primitive.size.x, ecs.GetComponent<Mesh>(entity).primitive.size.y, ecs.GetComponent<Mesh>(entity).primitive.size.z);
			}


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
				Vec3 halfExtent = { t.scale.x * 0.5f * meshsize.GetX() * p.collidersize.x, t.scale.y * 0.5f * meshsize.GetY() * p.collidersize.y, t.scale.z * 0.5f * meshsize.GetZ() * p.collidersize.z };
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
			{
				float radius = t.scale.x * meshsize.GetX() * p.collidersize.x;

				if (radius <= 0.0f || !std::isfinite(radius))
				{
					radius = 0.01f;
				}

				p.collidersize.y = p.collidersize.z = std::max(p.collidersize.x, 0.01f);

				shape = new JPH::SphereShape(radius); //for our current sphere
				break;
			}
			case ShapeType::Capsule:
			{
				//shape = new JPH::CapsuleShape(t.scale.y * 0.5f * meshsize.GetY(), t.scale.x * 0.5f * meshsize.GetX());
					// Compute half-height (excluding the hemispherical caps)
				float halfHeight = t.scale.y * 0.5f * meshsize.GetY() * p.collidersize.y;
				float capradius = t.scale.x * 0.5f * meshsize.GetX() * p.collidersize.x;

				// --- Safety checks ---
				if (halfHeight <= 0.0f || !std::isfinite(halfHeight))
				{
					halfHeight = 0.01f; // fallback
				}

				if (capradius <= 0.0f || !std::isfinite(capradius))
				{
					capradius = 0.01f; // fallback
				}

				// Ensure capsule collider size stays valid
				p.collidersize.x = std::max(p.collidersize.x, 0.01f);
				p.collidersize.y = std::max(p.collidersize.y, 0.01f);
				p.collidersize.z = p.collidersize.x; // capsule is symmetric around Y

				// Create shape safely
				shape = new JPH::CapsuleShape(halfHeight, capradius);
				break;
			}
			case ShapeType::CustomMesh:

				//check the parent object if got model
				if (ecs.HasComponent<ModelComponent>(entity))
				{
					auto& modelComp = ecs.GetComponent<ModelComponent>(entity);
					auto model = modelComp.m_model;
					if (!model) break;

					// Fill the custom mesh vertices for physics
					p.customMeshVertices = model->GetSkinnedVertices();
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
				for (size_t i = 0; i < p.customMeshVertices.size(); i++)
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

			bodySettings.mIsSensor = (p.bodyType == PhysicsBodyType::Trigger);

			// Create body
			JPH::Body* body = bodyInterface.CreateBody(bodySettings);
			if (!body) continue;

			body->SetUserData(entity);
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

	EntityID Physics::GetEntityID(JPH::BodyID bodyID)
	{
		for (auto& [entity, rigidBody] : mEntityToBody)
		{
			if (rigidBody == bodyID)
			{
				return entity;
			}
		}

		return 0;
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
				std::vector<JPH::Float3> verts(3 * cMax);

				for (;;)
				{
					int triCount = ts.mShape->GetTrianglesNext(ctx, cMax, verts.data());
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

	void Physics::HandleCollisionEvent(const Body& a, const Body& b, CollisionEventType type)
	{
		std::lock_guard<std::mutex> _l(mPendingMutex);
		mPendingPairs.push_back(PendingPair{ type, a.GetID(), b.GetID() });
		//auto& ecs = ECS::GetInstance();
		//EntityID objectA = 0, objectB = 0;
		//for (auto phylist : mEntityToBody)
		//{
		//	if (phylist.second == a.GetID())
		//	{
		//		objectA = phylist.first;
		//	}
		//	if (phylist.second == b.GetID())
		//	{
		//		objectB = phylist.first;
		//	}
		//}

		//if (!ecs.IsEntityValid(objectB) || !ecs.IsEntityValid(objectA))
		//	return;

		//if (ecs.HasComponent<Script>(objectA))
		//	mCollisionEvent.emplace(type, objectA, b.IsSensor());
		//if (ecs.HasComponent<Script>(objectB))
		//	mCollisionEvent.emplace(type, objectB, a.IsSensor());
		//EE_CORE_INFO("Hi {} {} {}", type, objectB, a.IsSensor());
		//EE_CORE_INFO("Hi {} {} {}", type, objectA, b.IsSensor());

		//switch (type)
		//{
		//case Ermine::Physics::CollisionEventType::Begin:
		//	//EE_CORE_INFO("[Physics] Collision Begin");
		//	if (ecs.HasComponent<Script>(objectA))
		//		ecs.GetComponent<Script>(objectA).m_instance->OnCollisionEnter(objectB, a.IsSensor());
		//	if (ecs.HasComponent<Script>(objectB))
		//		ecs.GetComponent<Script>(objectB).m_instance->OnCollisionEnter(objectA, b.IsSensor());
		//	break;
		//case Ermine::Physics::CollisionEventType::Stay:
		//	//EE_CORE_INFO("[Physics] Collision Stay");
		//	if (ecs.HasComponent<Script>(objectA))
		//		ecs.GetComponent<Script>(objectA).m_instance->OnCollisionStay(objectB, a.IsSensor());
		//	if (ecs.HasComponent<Script>(objectB))
		//		ecs.GetComponent<Script>(objectB).m_instance->OnCollisionStay(objectA, b.IsSensor());
		//	break;
		//case Ermine::Physics::CollisionEventType::End:
		//	//does nth as collision exit alr, if want need lmk
		//	if (ecs.HasComponent<Script>(objectA))
		//		ecs.GetComponent<Script>(objectA).m_instance->OnCollisionExit(objectB, a.IsSensor());
		//	if (ecs.HasComponent<Script>(objectB))
		//		ecs.GetComponent<Script>(objectB).m_instance->OnCollisionExit(objectA, b.IsSensor());
		//	break;
		//default:
		//	break;
		//}
		//run script
	}

	void Physics::HandleCollisionEvent(JPH::BodyID a, JPH::BodyID b, CollisionEventType type)
	{
		std::lock_guard<std::mutex> _l(mPendingMutex);
		mPendingPairs.push_back(PendingPair{ type, a, b });
	}

	bool Physics::Raycast(const JPH::RVec3& origin, const JPH::RVec3& direction, float maxDistance, JPH::RayCastResult& outResult)
	{
		JPH::Vec3 dirNormalized = direction.Normalized();
		JPH::RRayCast ray(origin, dirNormalized * maxDistance);

		// Get a query context from PhysicsSystem
		const JPH::NarrowPhaseQuery& query = mPhysicsSystem.GetNarrowPhaseQuery();

		// Perform the cast
		bool hit = query.CastRay(ray, outResult);

		return hit;
	}

	std::vector<JPH::RayCastResult> Physics::RaycastAll(const JPH::RVec3& origin, const JPH::RVec3& direction, float maxDistance)
	{
		std::vector<JPH::RayCastResult> results;

		// Normalize direction
		JPH::Vec3 dirNormalized = direction.Normalized();

		// Build the ray (RRayCast takes origin and direction *distance)
		JPH::RRayCast ray(origin, dirNormalized * maxDistance);

		// Ray cast settings WIP to add ignore layer
		JPH::RayCastSettings settings;
		settings.SetBackFaceMode(JPH::EBackFaceMode::IgnoreBackFaces);

		JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;

		// Get the narrow phase query and perform the cast
		const JPH::NarrowPhaseQuery& query = mPhysicsSystem.GetNarrowPhaseQuery();
		query.CastRay(ray, settings, collector);

		// collector.mHits is an Array<RayCastResult> — copy into std::vector
		for (const auto& hit : collector.mHits)
			results.push_back(hit);

		// Sort nearest -> farthest (mFraction is 0..1 along the ray)
		std::sort(results.begin(), results.end(),
			[](const JPH::RayCastResult& a, const JPH::RayCastResult& b)
			{
				return a.mFraction < b.mFraction;
			});

		return results;
	}

	void Physics::ClearPhysicBody()
	{
		JPH::BodyInterface& bi = mPhysicsSystem.GetBodyInterfaceNoLock();
		JPH::BodyIDVector bodyIDs;
		mPhysicsSystem.GetBodies(bodyIDs);

		for (JPH::BodyID id : bodyIDs)
		{
			bi.RemoveBody(id);
			bi.DestroyBody(id);
		}
	}

	void Physics::SetPosition(EntityID ID, Ermine::Vec3 position)
	{
		auto& bodyInterface = mPhysicsSystem.GetBodyInterface();
		Ermine::Quaternion rot = ECS::GetInstance().GetComponent<Transform>(ID).rotation;
		bodyInterface.SetPositionAndRotation(
			GetBodyID(ID),
			JPH::Vec3(position.x, position.y, position.z),
			JPH::Quat(rot.x, rot.y, rot.z, rot.w),
			JPH::EActivation::Activate);
	}

	void Physics::SetRotation(EntityID ID, Ermine::Vec3 rotation)
	{
		auto& bodyInterface = mPhysicsSystem.GetBodyInterface();
		Ermine::Vec3 pos = ECS::GetInstance().GetComponent<Transform>(ID).position;
		Ermine::Quaternion rot = FromEulerDegrees(rotation);
		bodyInterface.SetPositionAndRotation(
			GetBodyID(ID),
			JPH::Vec3(pos.x, pos.y, pos.z),
			JPH::Quat(rot.x, rot.y, rot.z, rot.w),
			JPH::EActivation::Activate);
	}

	void Physics::SetRotation(EntityID ID, Ermine::Quaternion rotation)
	{
		auto& bodyInterface = mPhysicsSystem.GetBodyInterface();
		Ermine::Vec3 pos = ECS::GetInstance().GetComponent<Transform>(ID).position;
		bodyInterface.SetPositionAndRotation(
			GetBodyID(ID),
			JPH::Vec3(pos.x, pos.y, pos.z),
			JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
			JPH::EActivation::Activate);
	}

	void Physics::Move(EntityID ID, Ermine::Vec3 position, Ermine::Vec3 rotation)
	{
		auto& bodyInterface = mPhysicsSystem.GetBodyInterface();
		Ermine::Quaternion rot = FromEulerDegrees(rotation);
		bodyInterface.SetPositionAndRotation(
			GetBodyID(ID),
			JPH::Vec3(position.x, position.y, position.z),
			JPH::Quat(rot.x, rot.y, rot.z, rot.w),
			JPH::EActivation::Activate);
	}

	void Physics::Move(EntityID ID, Ermine::Vec3 position, Ermine::Quaternion rotation)
	{
		auto& bodyInterface = mPhysicsSystem.GetBodyInterface();
		bodyInterface.SetPositionAndRotation(
			GetBodyID(ID),
			JPH::Vec3(position.x, position.y, position.z),
			JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
			JPH::EActivation::Activate);
	}

	void Physics::FlushPendingPairsToEntityEvents()
	{
		std::vector<PendingPair> local;
		{
			std::lock_guard<std::mutex> _l(mPendingMutex);
			if (mPendingPairs.empty()) return;
			local.swap(mPendingPairs);
		}

		auto& ecs = ECS::GetInstance();
		const JPH::BodyLockInterface& bli = mPhysicsSystem.GetBodyLockInterface();

		for (const PendingPair& pp : local)
		{
			EntityID entA = 0, entB = 0;
			bool aIsSensor = false, bIsSensor = false;

			{
				JPH::BodyLockRead lockA(bli, pp.a);
				if (lockA.SucceededAndIsInBroadPhase())
				{
					const JPH::Body& ba = lockA.GetBody();
					entA = ba.GetUserData();
					aIsSensor = ba.IsSensor();
				}
			}

			{
				JPH::BodyLockRead lockB(bli, pp.b);
				if (lockB.SucceededAndIsInBroadPhase())
				{
					const JPH::Body& bb = lockB.GetBody();
					entB = bb.GetUserData();
					bIsSensor = bb.IsSensor();
				}
			}

			// Fallback
			if ((!ecs.IsEntityValid(entA) || !ecs.IsEntityValid(entB)))
			{
				for (auto& [entity, bodyID] : mEntityToBody)
				{
					if (entA == 0 && bodyID == pp.a) entA = entity;
					if (entB == 0 && bodyID == pp.b) entB = entity;
					if (entA && entB) break;
				}
			}

			if (!ecs.IsEntityValid(entA) && !ecs.IsEntityValid(entB))
				continue;

			if (ecs.IsEntityValid(entA) && ecs.HasComponent<Script>(entA))
				mCollisionEvent.emplace(pp.type, entA, entB, bIsSensor);
			if (ecs.IsEntityValid(entB) && ecs.HasComponent<Script>(entB))
				mCollisionEvent.emplace(pp.type, entB, entA, aIsSensor);
		}
	}

}
