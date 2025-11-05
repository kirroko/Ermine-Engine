/* Start Header ************************************************************************/
/*!
\file       Engine.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       09/03/2025
\brief      This file contains the declaration of the Engine system.
			Expose key engine functions to be called from the editor's main loop
			Helps managed subsystems of the engine

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "Engine.h"
// ECS and Components
#include "ECS.h"
#include "Components.h"
// Internal Engine Systems
#include "Input.h"
#include "Logger.h"
#include "AssetManager.h"
#include "FrameController.h"
#include "GeometryFactory.h"
#include "JobSystem.h"
#include "Serialisation.h"
// Engine Systems
#include "Renderer.h"
#include "ScriptEngine.h"
#include "AudioSystem.h"
#include "Particles.h"
#include "Physics.h"
#include "FiniteStateMachine.h"
#include "Skybox.h"
#include "Cubemap.h"
#include "ScriptSystem.h"
#include "AnimationManager.h"
#include "ConsoleGUI.h"
#include "GuidRegistry.h"
#include "Scene.h"
#include "HierarchySystem.h"
#include "GameCamera.h"

#if defined(EE_EDITOR)
#include "GraphicsDebugGUI.h"
#include "AssetBrowser.h"
#include "EditorCamera.h"
#include "EditorGUI.h"
#include "ViewPortGUI.h"
#include "AudioImGUI.h"
#include "SceneManager.h"
#include "FSMEditor.h"
#include "AnimationGUI.h"
#include "ResourcePipe.h"
#endif

using namespace Ermine;

#define EE_AUTO_REGISTER_COMPONENT(Type, Name) \
	static bool _##Type##_autoreg = [](){ Ermine::ECS::GetInstance().RegisterComponent<Type>(Name); return true; }();

namespace
{
	bool s_isInitialized = false;

	// For Skybox/Environment mapping
	static std::unique_ptr<Ermine::graphics::Skybox> skybox;
	static std::shared_ptr<Ermine::graphics::Cubemap> environmentCubemap;

	void EnableMemoryLeakChecking(int breakAlloc = -1)
	{
		int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
		tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
		_CrtSetDbgFlag(tmpDbgFlag);
		if (breakAlloc != -1)
			_CrtSetBreakAlloc(breakAlloc);
	}

	EntityID fbxEntity = 0;

	// Unity-style duplicate name generator
	std::string GenerateUnityStyleName(const std::string& baseName)
	{
		auto& ecs = ECS::GetInstance();
	
		// Extract base name without existing number suffix
		std::string cleanBaseName = baseName;
		std::smatch match;
		std::regex pattern(R"(^(.+)\s+\((\d+)\)$)");
		
		if (std::regex_match(baseName, match, pattern))
		{
			cleanBaseName = match[1].str();
		}
		
		// Find the next available number
		int maxNumber = 0;
		bool baseNameExists = false;
		
		// Check all existing entities for name conflicts
		for (EntityID e = 1; e < MAX_ENTITIES; ++e)
		{
			if (!ecs.IsEntityValid(e) || !ecs.HasComponent<ObjectMetaData>(e))
				continue;
				
			const auto& meta = ecs.GetComponent<ObjectMetaData>(e);
			
			// Check if exact base name exists
			if (meta.name == cleanBaseName)
			{
				baseNameExists = true;
			}
			
			// Check for numbered variants
			std::smatch numberMatch;
			if (std::regex_match(meta.name, numberMatch, pattern))
			{
				if (numberMatch[1].str() == cleanBaseName)
				{
					int num = std::stoi(numberMatch[2].str());
					maxNumber = std::max(maxNumber, num);
				}
			}
		}
		
		// If base name exists or we found numbered variants, use next number
		if (baseNameExists || maxNumber > 0)
		{
			return cleanBaseName + " (" + std::to_string(maxNumber + 1) + ")";
		}
		
		// Otherwise, append (1)
		return cleanBaseName + " (1)";
	}
}

bool engine::Init(GLFWwindow* windowContext)
{
	if (s_isInitialized) // Already initialized
		return true;

	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	//std::string pipelinePath = "../../../../Ermine-ResourcePipeline";
	//std::cout << "Contents of Ermine-ResourcePipeline:" << std::endl;
	//try {
	//	for (const auto& entry : std::filesystem::directory_iterator(pipelinePath)) {
	//		std::cout << "  " << entry.path().filename().string() << std::endl;
	//	}
	//}
	//catch (const std::exception& e) {
	//	std::cout << "Error reading pipeline directory: " << e.what() << std::endl;
	//}

	//// Try the full path
	//std::string databasePath = "../../../../Ermine-ResourcePipeline/Ermine-Game.lion_rcdbase";
	//if (std::filesystem::exists(databasePath)) {
	//	std::cout << "Found database at: " << std::filesystem::absolute(databasePath) << std::endl;

	//	if (!AssetManager::GetInstance().Initialize(databasePath)) {
	//		EE_CORE_WARN("AssetManager database initialization failed");
	//	}
	//}
	//else {
	//	std::cout << "Database still not found at: " << databasePath << std::endl;
	//}

	//std::cout << "Engine working directory: " << std::filesystem::current_path() << std::endl;

	std::string databasePath = "../Ermine-Game.lion_rcdbase";  // Adjust path as needed
	std::string projectGuid = "";  // Leave empty to auto-detect, or put your actual project GUID

	if (!AssetManager::GetInstance().Initialize(databasePath, projectGuid)) {
		EE_CORE_WARN("AssetManager database initialization failed, falling back to direct loading");
	}

	EnableMemoryLeakChecking();

	Input::Init(windowContext);

	FrameController::Init(120.f, 60.f);

	graphics::GPUProfiler::Init(150); // Track last 150 frames

	job::Initialize();

	ECS::GetInstance().Init();

	AudioSystem::Init();

	// TODO: Register all components here, limit of 255 components
	EE_AUTO_REGISTER_COMPONENT(Transform, "Transform")
	EE_AUTO_REGISTER_COMPONENT(Rigidbody3D, "Rigidbody3D")
	EE_AUTO_REGISTER_COMPONENT(Mesh, "Mesh")
	EE_AUTO_REGISTER_COMPONENT(Material, "Material")
	EE_AUTO_REGISTER_COMPONENT(ObjectMetaData, "ObjectMetaData")
	EE_AUTO_REGISTER_COMPONENT(Light, "Light")
	EE_AUTO_REGISTER_COMPONENT(AudioComponent, "AudioComponent") 
	EE_AUTO_REGISTER_COMPONENT(GlobalAudioComponent, "GlobalAudioComponent")
	EE_AUTO_REGISTER_COMPONENT(PhysicComponent, "PhysicComponent")
	EE_AUTO_REGISTER_COMPONENT(ModelComponent, "ModelComponent")
	EE_AUTO_REGISTER_COMPONENT(AnimationComponent, "AnimationComponent")
	EE_AUTO_REGISTER_COMPONENT(HierarchyComponent, "HierarchyComponent")
	EE_AUTO_REGISTER_COMPONENT(StateMachine, "StateMachine")
	EE_AUTO_REGISTER_COMPONENT(GlobalTransform, "GlobalTransform")
	EE_AUTO_REGISTER_COMPONENT(ParticleEmitter, "ParticleEmitter");
	EE_AUTO_REGISTER_COMPONENT(CameraComponent, "CameraComponent");

	// NOTE : THESE ARE SPECIAL CASES DUE TO THE FACT THAT THEIR COMPONENTS ARE UNIQUE AND WOULDN'T WORK BY SHALLOW COPIED OR DEEP COPIED
	// THE CLONING FUNCTIONALITY HAVE BEEN CONSIDERED INTO ECS ITSELF. UNSURE, ASK.
	// Special case for Script component, ctor a new script with same class name
	ECS::GetInstance().RegisterComponent<Script>("Script",
		[](ComponentManager& cm, EntityID src, EntityID dst)
		{
			if (!cm.HasComponent<Script>(src)) return;
			auto& srcScript = cm.GetComponent<Script>(src);
			cm.AddComponent<Script>(dst, Script(srcScript.m_className, dst));
		});

	// Special case for IDComponent with custom clone to force new GUID, as IDs should be unique
	ECS::GetInstance().RegisterComponent<IDComponent>("IDComponent",
		[](ComponentManager& cm, [[maybe_unused]] EntityID src, EntityID dst)
		{
			auto g = Guid::New();
			cm.AddComponent<IDComponent>(dst, IDComponent{ g });
			ECS::GetInstance().GetGuidRegistry().Register(dst, g);
		});

	// Register all systems
	ECS::GetInstance().RegisterSystem<graphics::Renderer>();
	ECS::GetInstance().RegisterSystem<graphics::ModelSystem>();
	ECS::GetInstance().RegisterSystem<graphics::MaterialSystem>();
	ECS::GetInstance().RegisterSystem<scripting::ScriptSystem>();
	ECS::GetInstance().RegisterSystem<AudioSystem>();
	ECS::GetInstance().RegisterSystem<ParticleSystem>();
	ECS::GetInstance().RegisterSystem<graphics::LightSystem>();
	ECS::GetInstance().RegisterSystem<graphics::AnimationManager>();
	ECS::GetInstance().RegisterSystem<HierarchySystem>();
	ECS::GetInstance().RegisterSystem<StateManager>();
	ECS::GetInstance().RegisterSystem<graphics::GameCamera>();

	//Register JPH::TempAllocatorImpl for Physcis
	RegisterDefaultAllocator();
	ECS::GetInstance().RegisterSystem<Physics>();
	ECS::GetInstance().GetSystem<Physics>()->Init();
	ECS::GetInstance().GetSystem<Physics>()->AttachDebugRenderer(std::make_shared<MyDebugRenderer>());

	// Set system signatures
	SignatureID sig;

	// For Renderer system
	sig.set(ECS::GetInstance().GetComponentType<Mesh>());
	ECS::GetInstance().SetSystemSignature<graphics::Renderer>(sig);

	// For Model system
	sig.reset();
	sig.set(ECS::GetInstance().GetComponentType<ModelComponent>());
	ECS::GetInstance().SetSystemSignature<graphics::ModelSystem>(sig);

	// For Material system
	sig.reset();
	sig.set(ECS::GetInstance().GetComponentType<Material>());
	ECS::GetInstance().SetSystemSignature<graphics::MaterialSystem>(sig);

	// For GameCamera system
	SignatureID gameCameraSig;
	// GameCamera doesn't require any components to exist (it's a singleton system)
	ECS::GetInstance().SetSystemSignature<graphics::GameCamera>(gameCameraSig);

	// For Script system
	sig.reset();
	sig.set(ECS::GetInstance().GetComponentType<Script>());
	ECS::GetInstance().SetSystemSignature<scripting::ScriptSystem>(sig);

	// For Audio system
	sig.reset();
	sig.set(ECS::GetInstance().GetComponentType<AudioComponent>());
	sig.set(ECS::GetInstance().GetComponentType<Transform>());
	ECS::GetInstance().SetSystemSignature<AudioSystem>(sig);

	// For Particles
	sig.reset();
	sig.set(ECS::GetInstance().GetComponentType<Transform>());
	sig.set(ECS::GetInstance().GetComponentType<ParticleEmitter>());
	ECS::GetInstance().SetSystemSignature<ParticleSystem>(sig);

	// For Physics
	sig.reset();
	sig.set(ECS::GetInstance().GetComponentType<PhysicComponent>());
	sig.set(ECS::GetInstance().GetComponentType<Transform>());
	ECS::GetInstance().SetSystemSignature<Physics>(sig);

	// Lights
	sig.reset();
	sig.set(ECS::GetInstance().GetComponentType<Light>());
	sig.set(ECS::GetInstance().GetComponentType<Transform>());
	ECS::GetInstance().SetSystemSignature<graphics::LightSystem>(sig);

	// For Animation system
	sig.reset();
	sig.set(ECS::GetInstance().GetComponentType<AnimationComponent>());
	ECS::GetInstance().SetSystemSignature<graphics::AnimationManager>(sig);

	// For Hierarchy System
	SignatureID hierarchySig;
	hierarchySig.set(ECS::GetInstance().GetComponentType<HierarchyComponent>());
	hierarchySig.set(ECS::GetInstance().GetComponentType<Transform>());
	ECS::GetInstance().SetSystemSignature<HierarchySystem>(hierarchySig);

	// For FSM
	SignatureID fsmSig;
	fsmSig.set(ECS::GetInstance().GetComponentType<StateMachine>());
	fsmSig.set(ECS::GetInstance().GetComponentType<Transform>());
	ECS::GetInstance().SetSystemSignature<StateManager>(fsmSig);

	glfwSetFramebufferSizeCallback(windowContext, []([[maybe_unused]] GLFWwindow* window, int width, int height)
		{
#if defined(EE_EDITOR)
			editor::EditorCamera::GetInstance().SetViewportSize(static_cast<float>(width), static_cast<float>(height));
#else
			glViewport(0, 0, width, height);
			auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
			if (renderer && width > 0 && height > 0)
				renderer->OnWindowResize(width, height);
#endif
		});

	// Create graphics resources
	auto shader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/vertex.glsl", "../Resources/Shaders/fragment.glsl");
	auto texture = AssetManager::GetInstance().LoadTexture("../Resources/Textures/greybox_light_grid.png");

	// Load skybox shader and create a simple test cubemap
	auto skyboxShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/skybox_vertex.glsl", "../Resources/Shaders/skybox_fragment.glsl");

	std::array<std::string, 6> cubemapFaces = {
		"../Resources/Textures/Skybox/right.jpg",   // +X (right)
		"../Resources/Textures/Skybox/left.jpg",    // -X (left)  
		"../Resources/Textures/Skybox/bottom.jpg",  // +Y (top) 
		"../Resources/Textures/Skybox/top.jpg",     // -Y (bottom) 
		"../Resources/Textures/Skybox/front.jpg",   // +Z (front)
		"../Resources/Textures/Skybox/back.jpg"     // -Z (back)
	};
	environmentCubemap = AssetManager::GetInstance().LoadCubemap(cubemapFaces, "default_skybox");

	// Create the skybox if cubemap loaded successfully
	if (environmentCubemap && environmentCubemap->IsValid() && skyboxShader && skyboxShader->IsValid()) {
		skybox = std::make_unique<graphics::Skybox>(environmentCubemap, skyboxShader);
		EE_CORE_INFO("Skybox created successfully");
		// Register skybox with renderer so it actually gets rendered
		auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
		if (renderer)
			renderer->SetSkybox(skybox.get());
	}
	else {
		EE_CORE_WARN("Failed to create skybox - cubemap or shader invalid");
	}


	// Create shared materials for common use cases
	std::shared_ptr<graphics::Material> basicWhiteMaterial = AssetManager::GetInstance().CreateMaterial("basic_white", shader, "PBR_WHITE");
	std::shared_ptr<graphics::Material> metalMaterial = AssetManager::GetInstance().CreateMaterial("shiny_metal", shader, "PBR_METAL");
	std::shared_ptr<graphics::Material> emissiveWhiteMaterial = AssetManager::GetInstance().CreateMaterial("light_emissive", shader, "EMISSIVE_WHITE");
	std::shared_ptr<graphics::Material> glassMaterial = AssetManager::GetInstance().CreateMaterial("clear_glass", shader, "PBR_GLASS");
	std::shared_ptr<graphics::Material> waterMaterial = AssetManager::GetInstance().CreateMaterial("water_surface", shader, "PBR_WATER");

	EE_CORE_INFO("Created shared materials with proper texture assignment control");

	// Initialize game camera
	auto gameCamera = ECS::GetInstance().GetSystem<graphics::GameCamera>();
	int windowWidth, windowHeight;
	glfwGetFramebufferSize(windowContext, &windowWidth, &windowHeight);
	gameCamera->SetViewportSize(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
	// Audio test entity
	//auto audioTestEntity = ECS::GetInstance().CreateEntity();
	//ECS::GetInstance().AddComponent(audioTestEntity, Transform(Vec3(2, 0, -1), Quaternion(), Vec3(1, 1, 1)));
	//ECS::GetInstance().AddComponent(audioTestEntity, ObjectMetaData());
	//ECS::GetInstance().AddComponent<Ermine::HierarchyComponent>(audioTestEntity, Ermine::HierarchyComponent{});

	//AudioComponent testAudio;
	//ECS::GetInstance().AddComponent(audioTestEntity, testAudio);
	//EE_CORE_INFO("Audio test entity created with ID: {} - will auto-play", audioTestEntity);

	// Example FBX entity
	//fbxEntity = ECS::GetInstance().CreateEntity();
	//auto model = AssetManager::GetInstance().LoadModel("../Resources/Models/Walking.fbx");
	//ECS::GetInstance().AddComponent(fbxEntity, Transform(Vec3(2, -0.5f, 0), Quaternion(), Vec3(0.01f, 0.01f, 0.01f)));
	////ECS::GetInstance().AddComponent(
	////	fbxEntity,
	////	PhysicComponent(
	////		PhysicsBodyType::Rigid,         // "rigid body", "trigger"
	////		JPH::EMotionType::Dynamic,      // static, dynamic, or kinematic
	////		1.0f,                            // mass ( 0 for static , else is dynamic)
	////		ShapeType::Capsule				// Box, Sphere, Capsule, CustomMesh(need pass vertex)
	////	));
	//ECS::GetInstance().AddComponent(fbxEntity, ObjectMetaData("Character", "Model", true));
	//ECS::GetInstance().AddComponent(fbxEntity, Mesh{}); // empty mesh component for renderer signature
	//ECS::GetInstance().AddComponent(fbxEntity, ModelComponent(model));
	//ECS::GetInstance().AddComponent(fbxEntity, AnimationComponent("Walking"));
	//ECS::GetInstance().AddComponent<Ermine::HierarchyComponent>(fbxEntity, Ermine::HierarchyComponent{});

	//// Adding animation component
	//const aiScene* scene = model->GetAssimpScene(); // Read animations from aiScene
	//if (scene && scene->mNumAnimations > 0) {
	//	ECS::GetInstance().AddComponent(fbxEntity, AnimationComponent(model));
	//}

	//// Adding material component
	//auto FBXMaterial = std::make_unique<graphics::Material>(shader);
	//auto fbxTexture = AssetManager::GetInstance().LoadTexture("../Resources/Textures/Pants_Base_color.png");
	//FBXMaterial->LoadTemplate(graphics::MaterialTemplates::PBR_WHITE());

	//if (fbxTexture && fbxTexture->IsValid()) {
	//	FBXMaterial->SetTexture("materialAlbedoMap", fbxTexture);
	//	FBXMaterial->SetBool("materialHasAlbedoMap", true);
	//}
	//ECS::GetInstance().AddComponent(fbxEntity, Material(std::move(FBXMaterial)));

	// Create a simple quad mesh for particles
	//auto tex = AssetManager::GetInstance().LoadTexture("../Resources/Textures/greybox_red_solid.png");

	// initialize particles emitter
	ECS::GetInstance().GetSystem<ParticleSystem>()->Init(shader);

	// Create first cube
	//auto entity = ECS::GetInstance().CreateEntity();
	//ECS::GetInstance().AddComponent(entity, Transform(Vec3(0, -1, -1), FromEulerDegrees(0.0f, 0.0f, 0.0f), Vec3(5, 0.1f, 5)));
	//ECS::GetInstance().AddComponent(entity, ObjectMetaData());
	//ECS::GetInstance().AddComponent(entity, graphics::GeometryFactory::CreateCube(1, 1, 1));
	//ECS::GetInstance().AddComponent(
	//	entity,
	//	PhysicComponent(
	//		PhysicsBodyType::Rigid,        // "rigid body", "trigger"
	//		JPH::EMotionType::Static,      // static, dynamic, or kinematic
	//		0.0f,                          // mass ( 0 for static , else is dynamic)
	//		ShapeType::Box				   // Box, Sphere, Capsule, CustomMesh(need pass vertex)
	//	));

	////InspectorGUI inspector{ entity, "Inspector" };
	////inspector.SetEntity(entity);

	// Create second cube  
	//auto entity2 = ECS::GetInstance().CreateEntity();
	//ECS::GetInstance().AddComponent(entity2, Transform(Vec3(0, -1, 0), Quaternion(), Vec3(100, 0.1f, 100)));
	//ECS::GetInstance().AddComponent(entity2, ObjectMetaData());
	//ECS::GetInstance().AddComponent(entity2, graphics::GeometryFactory::CreateCube(1, 1, 1));
	//ECS::GetInstance().AddComponent(
	//	entity2,
	//	PhysicComponent(
	//		PhysicsBodyType::Rigid,        // "rigid body", "trigger"
	//		JPH::EMotionType::Static,      // static, dynamic, or kinematic
	//		1.0f,                          // mass ( 0 for static , else is dynamic)
	//		ShapeType::Box				   // Box, Sphere, Capsule, CustomMesh(need pass vertex)
	//	));

	//ECS::GetInstance().AddComponent<Ermine::HierarchyComponent>(entity2, Ermine::HierarchyComponent{});

	//auto& mesh = ECS::GetInstance().GetComponent<Mesh>(entity2);
	//mesh.kind = Mesh::Kind::Primitive;
	//mesh.primitive.type = "Cube";
	//mesh.primitive.size = { 1,1,1 };

	//ECS::GetInstance().AddComponent(entity2, HierarchyComponent());

	// Apply texture to floor
	//auto cube2Material = std::make_shared<graphics::Material>(shader);
	//ECS::GetInstance().AddComponent(entity2, Material(cube2Material));
	//ECS::GetInstance().AddComponent(entity2, Script("Sandbox", entity2));
	//if (texture && texture->IsValid()) {
	//	cube2Material->SetTexture("materialAlbedoMap", texture);
	//	cube2Material->SetBool("materialHasAlbedoMap", true);
	//}

	//// Create lights with balanced intensities
	//auto mainLightEntity = ECS::GetInstance().CreateEntity();
	//ECS::GetInstance().AddComponent(mainLightEntity, Transform(Vec3(0, 4, 2), Quaternion(0.9f, 0.2f, 0.1f, -0.3f), Vec3(1, 1, 1)));
	//ECS::GetInstance().AddComponent(mainLightEntity, ObjectMetaData("MainLight", "Light", true));
	//ECS::GetInstance().AddComponent(mainLightEntity, Light(Vec3(1, 1, 1), 0.8f, LightType::DIRECTIONAL, true));
	//ECS::GetInstance().AddComponent<Ermine::HierarchyComponent>(mainLightEntity, Ermine::HierarchyComponent{});

	//auto yellowLightSpot = ECS::GetInstance().CreateEntity();
	//ECS::GetInstance().AddComponent<Transform>(yellowLightSpot, Transform(Vec3(0, 10, 0), Quaternion(0.707f, 0.f, 0.f, 0.707f), Vec3(1, 1, 1)));
	//ECS::GetInstance().AddComponent<ObjectMetaData>(yellowLightSpot, ObjectMetaData("Red", "Light", true));
	//ECS::GetInstance().AddComponent<Light>(yellowLightSpot, Light(Vec3(1, 0.8f, 0.6f), 1.f, LightType::SPOT, true, 50, 60, 100.f));
	//auto yellowLightSpotMaterial = std::make_shared<graphics::Material>(shader);
	//yellowLightSpotMaterial->LoadTemplate(graphics::MaterialTemplates::EMISSIVE(Vec3(1, 0.8f, 0.6f), 2.0f));
	//ECS::GetInstance().AddComponent(yellowLightSpot, graphics::GeometryFactory::CreateCube(0.1f, 0.1f, 0.1f));
	//ECS::GetInstance().AddComponent(yellowLightSpot, Material(yellowLightSpotMaterial));
	//ECS::GetInstance().AddComponent<Ermine::HierarchyComponent>(yellowLightSpot, Ermine::HierarchyComponent{});

	// Red accent light
	//auto redLightEntity = ECS::GetInstance().CreateEntity();
	//ECS::GetInstance().AddComponent<Transform>(redLightEntity, Transform(Vec3(3, 2, 0), Quaternion(), Vec3(1, 1, 1)));
	//ECS::GetInstance().AddComponent<ObjectMetaData>(redLightEntity, ObjectMetaData("LightRed", "Light", true));
	//ECS::GetInstance().AddComponent<Light>(redLightEntity, Light(Vec3(1, 0.0, 0.0), 0.5f, LightType::POINT));

	//auto redLightMaterial = std::make_shared<graphics::Material>(shader);
	//redLightMaterial->LoadTemplate(graphics::MaterialTemplates::EMISSIVE(Vec3(1.0f, 0.f, 0.f), 10.0f));
	//ECS::GetInstance().AddComponent(redLightEntity, graphics::GeometryFactory::CreateCube(0.1f, 0.1f, 0.1f));
	//ECS::GetInstance().AddComponent(redLightEntity, Material(redLightMaterial));
	//ECS::GetInstance().AddComponent<Ermine::HierarchyComponent>(redLightEntity, Ermine::HierarchyComponent{});

	//auto& redlight = ECS::GetInstance().GetComponent<Mesh>(redLightEntity);
	//redlight.kind = Mesh::Kind::Primitive;
	//redlight.primitive.type = "Sphere";
	//redlight.primitive.size = { 0.1f,1,1 };

	// Blue accent light
	//auto blueLightEntity = ECS::GetInstance().CreateEntity();
	//ECS::GetInstance().AddComponent<Transform>(blueLightEntity, Transform(Vec3(-3, 2, 0), Quaternion(), Vec3(1, 1, 1)));
	//ECS::GetInstance().AddComponent<ObjectMetaData>(blueLightEntity, ObjectMetaData("LightBlue", "Light", true));
	//ECS::GetInstance().AddComponent<Light>(blueLightEntity, Light(Vec3(0.0, 0.0, 1), 0.5f, LightType::POINT));

	//auto blueLightMaterial = std::make_shared<graphics::Material>(shader);
	//blueLightMaterial->LoadTemplate(graphics::MaterialTemplates::EMISSIVE(Vec3(0.f, 0.f, 1.0f), 10.0f));
	//ECS::GetInstance().AddComponent(blueLightEntity, graphics::GeometryFactory::CreateCube(0.1f, 0.1f, 0.1f));
	//ECS::GetInstance().AddComponent(blueLightEntity, Material(blueLightMaterial));
	//ECS::GetInstance().AddComponent<Ermine::HierarchyComponent>(blueLightEntity, Ermine::HierarchyComponent{});

	//auto& bluelight = ECS::GetInstance().GetComponent<Mesh>(blueLightEntity);
	//bluelight.kind = Mesh::Kind::Primitive;
	//bluelight.primitive.type = "Sphere";
	//bluelight.primitive.size = { 0.1f,1,1 };

	//// Green accent light
	//auto greenLightEntity = ECS::GetInstance().CreateEntity();
	//ECS::GetInstance().AddComponent<Transform>(greenLightEntity, Transform(Vec3(0, 2, -3), Quaternion(), Vec3(1, 1, 1)));
	//ECS::GetInstance().AddComponent<ObjectMetaData>(greenLightEntity, ObjectMetaData("LightGreen", "Light", true));
	//ECS::GetInstance().AddComponent<Light>(greenLightEntity, Light(Vec3(0.0, 1.0f, 0.0), 0.5f, LightType::POINT));

	//auto greenLightMaterial = std::make_shared<graphics::Material>(shader);
	//greenLightMaterial->LoadTemplate(graphics::MaterialTemplates::EMISSIVE(Vec3(0.f, 1.f, 0.0f), 10.0f));
	//ECS::GetInstance().AddComponent(greenLightEntity, graphics::GeometryFactory::CreateCube(0.1f, 0.1f, 0.1f));
	//ECS::GetInstance().AddComponent(greenLightEntity, Material(greenLightMaterial));
	//ECS::GetInstance().AddComponent<Ermine::HierarchyComponent>(greenLightEntity, Ermine::HierarchyComponent{});

	//auto& greenlight = ECS::GetInstance().GetComponent<Mesh>(greenLightEntity);
	//greenlight.kind = Mesh::Kind::Primitive;
	//greenlight.primitive.type = "Sphere";
	//greenlight.primitive.size = { 0.1f,1,1 };

	//after creating all the physic object, update to physic system
	//ECS::GetInstance().GetSystem<Physics>()->UpdatePhysicList();

	//// Glass sphere demonstrating refraction
	//auto glassEntity = ECS::GetInstance().CreateEntity();
	//ECS::GetInstance().AddComponent<Transform>(glassEntity, Transform(Vec3(2, 0, -1), Quaternion(), Vec3(1, 1, 1)));
	//ECS::GetInstance().AddComponent(glassEntity, ObjectMetaData("GlassSphere", "Transparent", true));
	//ECS::GetInstance().AddComponent(glassEntity, graphics::GeometryFactory::CreateSphere(0.8f));
	//// Use the previously created glass material
	//ECS::GetInstance().AddComponent(glassEntity, Material(glassMaterial));
	//ECS::GetInstance().AddComponent<Ermine::HierarchyComponent>(glassEntity, Ermine::HierarchyComponent{});

	//// Coloured glass
	//auto waterEntity = ECS::GetInstance().CreateEntity();
	//ECS::GetInstance().AddComponent<Transform>(waterEntity, Transform(Vec3(-2, 0, -1), Quaternion(), Vec3(1, 1, 0.3f)));
	//ECS::GetInstance().AddComponent(waterEntity, ObjectMetaData("ColouredGlass", "Transparent", true));
	//ECS::GetInstance().AddComponent(waterEntity, graphics::GeometryFactory::CreateCube(2, 2, 0.6f));
	//// Use the previously created water material
	//ECS::GetInstance().AddComponent(waterEntity, Material(waterMaterial));
	//ECS::GetInstance().AddComponent<Ermine::HierarchyComponent>(waterEntity, Ermine::HierarchyComponent{});

	//// Metallic cube
	//auto metalCubeEntity = ECS::GetInstance().CreateEntity();
	//ECS::GetInstance().AddComponent<Transform>(metalCubeEntity, Transform(Vec3(-4, 0, -1), Quaternion(), Vec3(1, 1, 1)));
	//ECS::GetInstance().AddComponent<ObjectMetaData>(metalCubeEntity, ObjectMetaData("MetalCube", "Metal", true));
	//// Use the previously created metal material
	//metalMaterial->LoadTemplate(graphics::MaterialTemplates::PBR_METAL());
	//ECS::GetInstance().AddComponent(metalCubeEntity, graphics::GeometryFactory::CreateCube(1, 1, 1));
	//ECS::GetInstance().AddComponent(metalCubeEntity, Material(metalMaterial));
	//if (texture && texture->IsValid()) {
	//	metalMaterial->SetTexture("materialAlbedoMap", texture);
	//	metalMaterial->SetBool("materialHasAlbedoMap", true);
	//}
	//ECS::GetInstance().AddComponent<Ermine::HierarchyComponent>(metalCubeEntity, Ermine::HierarchyComponent{});

	//// Red cube
	//auto redCubeEntity = ECS::GetInstance().CreateEntity();
	//ECS::GetInstance().AddComponent<Transform>(redCubeEntity, Transform(Vec3(-6, 0, -1), Quaternion(), Vec3(1, 1, 1)));
	//ECS::GetInstance().AddComponent<ObjectMetaData>(redCubeEntity, ObjectMetaData("redCube", "Red", true));

	//// Create red material instance for this cube
	//auto redMaterial = std::make_shared<graphics::Material>(shader);
	//redMaterial->LoadTemplate(graphics::MaterialTemplates::PBR_RED());
	//ECS::GetInstance().AddComponent(redCubeEntity, graphics::GeometryFactory::CreateCube(1, 1, 1));
	//ECS::GetInstance().AddComponent(redCubeEntity, Material(redMaterial));
	//if (texture && texture->IsValid()) {
	//	redMaterial->SetTexture("materialAlbedoMap", texture);
	//	redMaterial->SetBool("materialHasAlbedoMap", true);
	//}
	//ECS::GetInstance().AddComponent<Ermine::HierarchyComponent>(redCubeEntity, Ermine::HierarchyComponent{});

	EE_CORE_INFO("Total living entities after creation: {0}", ECS::GetInstance().GetLivingEntityCount());

	//EE_CORE_INFO("FSM Test Cube created with ID: {}", s_FSMCube);
	if (windowWidth > 0 && windowHeight > 0)
		ECS::GetInstance().GetSystem<graphics::Renderer>()->Init(windowWidth, windowHeight);
	else
		ECS::GetInstance().GetSystem<graphics::Renderer>()->Init(1920, 1080); // Fallback to default size

	SceneManager::GetInstance().NewScene();

	EE_CORE_INFO("Material system now supports efficient sharing between entities using shared_ptr");
	EE_CORE_INFO("Systems and components registered successfully, Engine Initialized");

	// Editor windows
#if defined(EE_EDITOR)
	editor::EditorGUI::CreateImGUIWindow<ParticlesImGUI>();
	editor::EditorGUI::CreateImGUIWindow<AudioImGUI>();
	editor::EditorGUI::CreateImGUIWindow<editor::GraphicsDebugGUI>("Graphics Debug"); // TODO: Namespace required?
	editor::EditorGUI::CreateImGUIWindow<ViewPortGUI>();
	editor::EditorGUI::CreateImGUIWindow<FSMEditorImGUI>();
	editor::EditorGUI::CreateImGUIWindow<AnimationEditorImGUI>();
	editor::EditorGUI::CreateImGUIWindow<ConsoleGUI>();
	editor::EditorGUI::CreateImGUIWindow<ImguiUI::AssetBrowser>(); //TODO: Standardize please, do we want namespace ImGui for all window or not

	{
		static Ermine::ResourcePipeline pipeline;
		if (pipeline.Initialize("../Resources")) { 
			EE_CORE_INFO("ResourcePipeline initialized successfully");

			auto* assetBrowser = editor::EditorGUI::GetWindow<ImguiUI::AssetBrowser>();
			if (assetBrowser) {
				assetBrowser->InitWithPipeline(&pipeline);
				EE_CORE_INFO("AssetBrowser connected to ResourcePipeline");
			}
			else {
				EE_CORE_ERROR("Failed to get AssetBrowser window");
			}
		}
		else {
			EE_CORE_ERROR("Failed to initialize ResourcePipeline");
		}
	}

	auto defaultScene = std::make_shared<Scene>("Main Scene");
	editor::EditorGUI::SetActiveScene(defaultScene);
	SceneManager::GetInstance().SetActiveScene(defaultScene);
	EE_CORE_INFO("Created and set active scene: Main Scene");
#endif

	s_isInitialized = true;
}

void engine::Shutdown()
{
	if (!s_isInitialized)
		return;

	//const std::filesystem::path scenePath = "Ermine-Engine.scene";
	//SaveSceneToFile("Ermine-Engine", scenePath);
	//SaveCurrentScene("Level01");

	Config cfg{};
	int width, height;
	glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
	cfg.windowWidth = width;
	cfg.windowHeight = height;
	cfg.fullscreen = (glfwGetWindowMonitor(glfwGetCurrentContext()) != nullptr);
	cfg.maximized = (glfwGetWindowAttrib(glfwGetCurrentContext(), GLFW_MAXIMIZED) == GLFW_TRUE);
	cfg.title = "Ermine Editor 0.2";

	SaveConfigToFile(cfg, "Ermine-Engine.config", false);

	skybox.reset();           // Destroy skybox before cubemap
	environmentCubemap.reset(); // Destroy cubemap before AssetManager cleanup

	CoUninitialize();

	AssetManager::GetInstance().Clear();
	ECS::GetInstance().GetSystem<Physics>()->Shutdown();

	graphics::GPUProfiler::Shutdown();

#if defined(EE_EDITOR)
	auto scriptSys = ECS::GetInstance().GetSystem<scripting::ScriptSystem>();
	if (scriptSys && scriptSys->m_ScriptEngine)
	{
		scriptSys->m_ScriptEngine->StopWatchingScriptSources();
		scriptSys->m_ScriptEngine->StopWatchingGameAssembly();
	}
#endif

	job::Shutdown();

	ECS::GetInstance().GetSystem<scripting::ScriptSystem>()->m_ScriptEngine->Shutdown();
	AudioSystem::Shutdown();

	ECS::GetInstance().Shutdown();

	s_isInitialized = false;
}

void engine::Update([[maybe_unused]] GLFWwindow* windowContext)
{
	if (!s_isInitialized)
		return;

	// Update FrameController
	FrameController::BeginFrame();

	// Profiler
	graphics::GPUProfiler::BeginFrame();

	// Handle shading mode toggle
	HandleShadingToggle(windowContext);

	// Update input states
	Input::Update();

	glfwPollEvents();

	// Game state update
	while (FrameController::ShouldUpdateFixed())
	{
		ECS::GetInstance().GetSystem<scripting::ScriptSystem>()->FixedUpdate();
		ECS::GetInstance().GetSystem<Physics>()->Update(FrameController::GetFixedDeltaTime());
	}

	// Other non-fixed logic
	ECS::GetInstance().GetSystem<scripting::ScriptSystem>()->Update();
	ECS::GetInstance().GetSystem<AudioSystem>()->Update();

	ECS::GetInstance().GetSystem<HierarchySystem>()->UpdateHierarchy();
	
	// Update editor camera
#if defined(EE_EDITOR)
	// Update appropriate camera based on play state
	if (editor::EditorGUI::isPlaying)
	{
		// Update game camera when playing
		auto gameCamera = ECS::GetInstance().GetSystem<graphics::GameCamera>();
		if (gameCamera)
		{
			// If camera doesn't have a valid entity, try to find one
			if (!gameCamera->HasValidCamera())
			{
				// Find the first entity with CameraComponent that's marked as isGameCamera
				auto& ecs = ECS::GetInstance();
				for (EntityID entity = 1; entity <= MAX_ENTITIES; ++entity)
				{
					if (ecs.IsEntityValid(entity) && ecs.HasComponent<CameraComponent>(entity))
					{
						auto& camComp = ecs.GetComponent<CameraComponent>(entity);
						if (camComp.isGameCamera && camComp.isPrimary)
						{
							gameCamera->SetCameraEntity(entity);
							EE_CORE_INFO("GameCamera: Found and set camera entity {}", entity);
							break;
						}
					}
				}
			}
			gameCamera->Update();
		}
	}
	else
	{
		// Update editor camera when not playing
		editor::EditorCamera::GetInstance().Update();
	}
#else
	// In standalone build, always update game camera
	auto gameCamera = ECS::GetInstance().GetSystem<graphics::GameCamera>();
	if (gameCamera)
	{
		// If camera doesn't have a valid entity, try to find one
		if (!gameCamera->HasValidCamera())
		{
			// Find the first entity with CameraComponent that's marked as isGameCamera
			auto& ecs = ECS::GetInstance();
			for (EntityID entity = 1; entity <= MAX_ENTITIES; ++entity)
			{
				if (ecs.IsEntityValid(entity) && ecs.HasComponent<CameraComponent>(entity))
				{
					auto& camComp = ecs.GetComponent<CameraComponent>(entity);
					if (camComp.isGameCamera && camComp.isPrimary)
					{
						gameCamera->SetCameraEntity(entity);
						break;
					}
				}
			}
		}
		gameCamera->Update();
	}
#endif
	// Update for Particles
	ECS::GetInstance().GetSystem<ParticleSystem>()->Update(FrameController::GetDeltaTime());

	// Animation Update
	ECS::GetInstance().GetSystem<graphics::AnimationManager>()->Update(FrameController::GetDeltaTime());

	// FSM update
	ECS::GetInstance().GetSystem<StateManager>()->Update(FrameController::GetFixedDeltaTime());
}

void engine::Render(GLFWwindow* window)
{
	if (!s_isInitialized)
		return;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	Mtx44 view;
	Mtx44 proj;

#if defined(EE_EDITOR)
	// Use appropriate camera based on play state
	if (editor::EditorGUI::isPlaying)
	{
		auto gameCamera = ECS::GetInstance().GetSystem<graphics::GameCamera>();
		if (gameCamera && gameCamera->HasValidCamera())
		{
			view = gameCamera->GetViewMatrix();
			proj = gameCamera->GetProjectionMatrix();
		}
		else
		{
			// Fallback to editor camera if no valid game camera
			view = editor::EditorCamera::GetInstance().GetViewMatrix();
			proj = editor::EditorCamera::GetInstance().GetProjectionMatrix();
		}
	}
	else
	{
		// Use editor camera when not playing
		view = editor::EditorCamera::GetInstance().GetViewMatrix();
		proj = editor::EditorCamera::GetInstance().GetProjectionMatrix();
	}
#else
	// Standalone build - use game camera
	auto gameCamera = ECS::GetInstance().GetSystem<graphics::GameCamera>();
	if (gameCamera && gameCamera->HasValidCamera())
	{
		view = gameCamera->GetViewMatrix();
		proj = gameCamera->GetProjectionMatrix();
	}
	else
	{
		// Fallback if no camera is available
		view = Mtx44(); // Identity matrix
		proj = Mtx44(); // Identity matrix
	}
#endif

	// Start GPU timing for rendering
	graphics::GPUProfiler::BeginEvent("Frame");

	auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();

	// Clear buffers
	renderer->Clear();

	// Draw scene objects
	renderer->Update(view, proj);

	// Stop GPU timing for rendering
	graphics::GPUProfiler::EndEvent();

	// Render ImGui/Editor on top of everything
#if defined(EE_EDITOR)
	if (editor::EditorGUI::IsInit())
		editor::EditorGUI::Render();
#endif

	glfwSwapBuffers(window);

	graphics::GPUProfiler::EndFrame();
}

void engine::HandleShadingToggle(GLFWwindow* windowContext)
{
	static bool key1WasPressed = false;
	static bool key2WasPressed = false;
	static bool key3WasPressed = false;
	static bool key4WasPressed = false;

	bool key1IsPressed = glfwGetKey(windowContext, GLFW_KEY_1) == GLFW_PRESS;
	bool key2IsPressed = glfwGetKey(windowContext, GLFW_KEY_2) == GLFW_PRESS;
	bool key3IsPressed = glfwGetKey(windowContext, GLFW_KEY_3) == GLFW_PRESS;
	bool key4IsPressed = glfwGetKey(windowContext, GLFW_KEY_4) == GLFW_PRESS;

	// Toggle to PBR (key 1)
	if (key1IsPressed && !key1WasPressed) {
		auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
		renderer->SetShadingMode(false);
		EE_CORE_INFO("Switched to PBR shading");
	}

	// Toggle to Blinn-Phong (key 2)  
	if (key2IsPressed && !key2WasPressed) {
		auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
		renderer->SetShadingMode(true);
		EE_CORE_INFO("Switched to Blinn-Phong shading");
	}

	// Toggle to Deferred (key 3)
	if (key3IsPressed && !key3WasPressed) {
		auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
		renderer->ToggleDeferredRendering();
	}

	// Toggle SSAO (key 4)
	if (key4IsPressed && !key4WasPressed) {
		auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
		renderer->m_SSAOEnabled = !renderer->m_SSAOEnabled;
	}

	key1WasPressed = key1IsPressed;
	key2WasPressed = key2IsPressed;
	key3WasPressed = key3IsPressed;
	key4WasPressed = key4IsPressed;
}