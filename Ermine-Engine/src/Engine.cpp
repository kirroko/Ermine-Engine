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

#include "AssetManager.h"
#include "AssetBrowser.h"
#include "ECS.h"
#include "Components.h"
#include "EditorCamera.h"
#include "FrameController.h"
#include "GeometryFactory.h"
#include "Input.h"
#include "Logger.h"
#include "Renderer.h"
#include "EditorGUI.h"
#include "JobSystem.h"
#include "ScriptEngine.h"
#include "Serialisation.h"
#include "AudioSystem.h"
#include "Particles.h"
#include "Physics.h"
#include "InspectorGUI.h"
#include "AudioImGUI.h"
#include "Skybox.h"
#include "Cubemap.h"

#include <random> // Include for random number generation

#include "ScriptSystem.h"

using namespace Ermine;

#define EE_AUTO_REGISTER_COMPONENT(Type, Name) \
	static bool _##Type##_autoreg = [](){ Ermine::ECS::GetInstance().RegisterComponent<Type>(Name); return true; }();

namespace
{
	bool s_isInitialized = false;
	//std::unique_ptr<editor::EditorCamera> s_EditorCamera = nullptr;

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

	// For Particles
	static std::shared_ptr<Ermine::graphics::Shader> particleShader;
	static std::unique_ptr<Ermine::ParticleEmitter> emitter;
}

bool engine::Init(GLFWwindow* windowContext)
{
	if (s_isInitialized) // Already initialized
		return true;

	//const std::filesystem::path cfgPath = std::filesystem::path("configs") / "ErmineEngine.config";
	const std::filesystem::path cfgPath = "Ermine-Engine.config";

	Config cfg{};
	try {
		cfg = LoadConfigFromFile(cfgPath);
		EE_CORE_INFO("Loaded config: {0}x{1}, fullscreen={2}, maximised={3}, title={4}",
			cfg.windowWidth, cfg.windowHeight, cfg.fullscreen, cfg.maximized, cfg.title);
	}
	catch (const std::exception& e) {
		EE_CORE_WARN("Config not found/invalid ({}). Using defaults.", e.what());
		cfg = { 1920, 1080, false, false, "Ermine Editor 0.1" };
		// Optional: write defaults so the file exists next run
		try { SaveConfigToFile(cfg, cfgPath, /*pretty=*/true); }
		catch (const std::exception& w) { EE_CORE_WARN("Could not write default config: {}", w.what()); }
	}

	// Apply config to the window
	glfwSetWindowTitle(windowContext, cfg.title.c_str());
	if (cfg.fullscreen) {
		GLFWmonitor* mon = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(mon);
		glfwSetWindowMonitor(windowContext, mon, 0, 0,
			mode->width, mode->height,
			mode->refreshRate);
	}
	else {
		glfwSetWindowSize(windowContext, cfg.windowWidth, cfg.windowHeight);

		if (cfg.maximized) {
			glfwMaximizeWindow(windowContext);
		}
		else {
			glfwRestoreWindow(windowContext);
		}
	}

	EnableMemoryLeakChecking();

	Input::Init(windowContext);

	FrameController::Init(120.f, 60.f);

	graphics::GPUProfiler::Init(150); // Track last 150 frames

	job::Initialize();

	ECS::GetInstance().Init();
	EE_CORE_INFO("ECS Initialized");
	EE_CORE_TRACE("Begin Registering of Components and Systems...");

	AudioSystem::Init();
	EE_CORE_INFO("AudioSystem Initialized");

	// TODO: Register all components here, limit of 32 components
	EE_AUTO_REGISTER_COMPONENT(Transform, "Transform")
	EE_AUTO_REGISTER_COMPONENT(Rigidbody3D, "Rigidbody3D")
	EE_AUTO_REGISTER_COMPONENT(Mesh, "Mesh")
	EE_AUTO_REGISTER_COMPONENT(Material, "Material")
	//EE_AUTO_REGISTER_COMPONENT(Script,"Script")
	EE_AUTO_REGISTER_COMPONENT(ObjectMetaData, "ObjectMetaData")
	EE_AUTO_REGISTER_COMPONENT(Light, "Light")
	EE_AUTO_REGISTER_COMPONENT(Particle, "Particle")
	EE_AUTO_REGISTER_COMPONENT(AudioComponent, "AudioComponent")
	EE_AUTO_REGISTER_COMPONENT(GlobalAudioComponent, "GlobalAudioComponent")
	EE_AUTO_REGISTER_COMPONENT(ReflectionProbe, "ReflectionProbe")
	EE_AUTO_REGISTER_COMPONENT(ModelComponent, "ModelComponent")

	// ECS::GetInstance().RegisterComponent<AudioComponent>(); // ADD THIS
	// ECS::GetInstance().RegisterComponent<GlobalAudioComponent>(); // ADD THIS IF YOU WANT GLOBAL AUDIO

	// Special Case for Script component, need to copy over the class name
	ECS::GetInstance().RegisterComponent<Script>("Script",
		[](Ermine::ComponentManager& cm, EntityID src, EntityID dst)
		{
			if (!cm.HasComponent<Script>(src)) return;
			auto& srcScript = cm.GetComponent<Script>(src);
			cm.AddComponent<Script>(dst, Script(srcScript.m_className, dst));
		});

	// TODO: Register all systems here, no limits
	ECS::GetInstance().RegisterSystem<graphics::Renderer>();
	ECS::GetInstance().RegisterSystem<scripting::ScriptSystem>();
	ECS::GetInstance().RegisterSystem<AudioSystem>();
	ECS::GetInstance().RegisterSystem<ParticleSystem>();
	ECS::GetInstance().RegisterSystem <graphics::LightSystem>();

	// TODO: Set the signature for the system as required
	// For Graphics/Renderer system
	SignatureID sig;
	sig.set(ECS::GetInstance().GetComponentType<Transform>());
	sig.set(ECS::GetInstance().GetComponentType<Mesh>());
	//sig.set(ECS::GetInstance().GetComponentType<ModelComponent>());
	sig.set(ECS::GetInstance().GetComponentType<Material>());
	ECS::GetInstance().SetSystemSignature<graphics::Renderer>(sig);

	sig.reset();
	sig.set(ECS::GetInstance().GetComponentType<Script>());
	ECS::GetInstance().SetSystemSignature<scripting::ScriptSystem>(sig);

	sig.reset();
	sig.set(ECS::GetInstance().GetComponentType<AudioComponent>());
	sig.set(ECS::GetInstance().GetComponentType<Transform>());
	ECS::GetInstance().SetSystemSignature<AudioSystem>(sig);

	// For Particles
	sig.reset();
	sig.set(ECS::GetInstance().GetComponentType<Transform>());
	sig.set(ECS::GetInstance().GetComponentType<Mesh>());
	sig.set(ECS::GetInstance().GetComponentType<Material>());
	sig.set(ECS::GetInstance().GetComponentType<Particle>());
	ECS::GetInstance().SetSystemSignature<ParticleSystem>(sig);

	// Lights
	sig.reset();
	sig.set(ECS::GetInstance().GetComponentType<Light>());
	sig.set(ECS::GetInstance().GetComponentType<Transform>());
	ECS::GetInstance().SetSystemSignature<graphics::LightSystem>(sig);

	glfwSetFramebufferSizeCallback(windowContext, []([[maybe_unused]] GLFWwindow* window, int width, int height)
		{
#ifdef _DEBUG
			editor::EditorCamera::GetInstance().SetViewportSize(static_cast<float>(width), static_cast<float>(height));
#endif
			glViewport(0, 0, width, height);
		});

	// Create graphics resources
	auto shader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/vertex.glsl", "../Resources/Shaders/fragment.glsl");
	auto texture = AssetManager::GetInstance().LoadTexture("../Resources/Textures/greybox_grey_grid.png");

	// Load skybox shader and create a simple test cubemap
	auto skyboxShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/skybox_vertex.glsl", "../Resources/Shaders/skybox_fragment.glsl");
	
	 std::array<std::string, 6> cubemapFaces = {
	     "../Resources/Textures/Skybox/right.jpg",   // +X (right)
	     "../Resources/Textures/Skybox/left.jpg",    // -X (left)  
	     "../Resources/Textures/Skybox/bottom.jpg",  // +Y (top) - swapped for correct orientation
	     "../Resources/Textures/Skybox/top.jpg",     // -Y (bottom) - swapped for correct orientation
	     "../Resources/Textures/Skybox/front.jpg",   // +Z (front)
	     "../Resources/Textures/Skybox/back.jpg"     // -Z (back)
	 };
	 environmentCubemap = AssetManager::GetInstance().LoadCubemap(cubemapFaces, "default_skybox");
	
	// Create the skybox if cubemap loaded successfully
	if (environmentCubemap && environmentCubemap->IsValid() && skyboxShader && skyboxShader->IsValid()) {
		skybox = std::make_unique<graphics::Skybox>(environmentCubemap, skyboxShader);
		EE_CORE_INFO("Skybox created successfully");
	} else {
		EE_CORE_WARN("Failed to create skybox - cubemap or shader invalid");
	}
	
	// For now, create a placeholder cubemap
	EE_CORE_INFO("Cubemap system initialized. You can load cubemaps using AssetManager::LoadCubemap() or LoadCubemapFromEquirectangular()");

	// Create shared materials for common use cases
	// These materials can be reused by multiple objects for better memory efficiency
	std::shared_ptr<graphics::Material> basicWhiteMaterial = AssetManager::GetInstance().CreateMaterial("basic_white", shader, "PBR_WHITE");
	std::shared_ptr<graphics::Material> metalMaterial = AssetManager::GetInstance().CreateMaterial("shiny_metal", shader, "PBR_METAL");
	std::shared_ptr<graphics::Material> emissiveWhiteMaterial = AssetManager::GetInstance().CreateMaterial("light_emissive", shader, "EMISSIVE_WHITE");
	
	// Create new materials showcasing refraction and transparency
	std::shared_ptr<graphics::Material> glassMaterial = AssetManager::GetInstance().CreateMaterial("clear_glass", shader, "PBR_GLASS");
	std::shared_ptr<graphics::Material> waterMaterial = AssetManager::GetInstance().CreateMaterial("water_surface", shader, "PBR_WATER");
	
	// Only apply textures to materials that explicitly need them
	// Leave most materials as pure color-based to avoid unwanted texture loading
	if (texture && texture->IsValid()) {
		// Create one textured material variant for demonstration purposes only
		std::shared_ptr<graphics::Material> texturedMaterial = AssetManager::GetInstance().CreateMaterial("textured_demo", shader, "PBR_WHITE");
		texturedMaterial->SetTexture("materialAlbedoMap", texture);
		texturedMaterial->SetBool("materialHasAlbedoMap", true);
		
		// Glass and water materials can optionally use environment textures but don't need albedo textures
		if (environmentCubemap && environmentCubemap->IsValid()) {
			glassMaterial->SetCubemap("materialEnvironmentMap", environmentCubemap);
			glassMaterial->SetCubemap("materialIrradianceMap", environmentCubemap);
			glassMaterial->SetBool("materialHasEnvironmentMap", true);
			glassMaterial->SetBool("materialHasIrradianceMap", true);
			
			waterMaterial->SetCubemap("materialEnvironmentMap", environmentCubemap);
			waterMaterial->SetCubemap("materialIrradianceMap", environmentCubemap);
			waterMaterial->SetBool("materialHasEnvironmentMap", true);
			waterMaterial->SetBool("materialHasIrradianceMap", true);
		}
	}
	
	EE_CORE_INFO("Created shared materials with proper texture assignment control");

	// Random number generation setup
	//std::random_device rd;
	//std::mt19937 gen(rd());
	//std::uniform_real_distribution<float> posDist(-10.0f, 10.0f); // Random positions between -10 and 10
	//std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);  // Random rotations between 0.0f and 360.0f

	// Example of how to create many entities with shared materials (commented out for now)
	//for (int i = 0; i < 500; ++i)
	//{
	//    auto entity = ECS::GetInstance().CreateEntity();
	//    Vec3 randomPosition(posDist(gen), posDist(gen), posDist(gen));
	//    Vec3 randomRotation(rotDist(gen), rotDist(gen), rotDist(gen));
	//    ECS::GetInstance().AddComponent(entity, Transform(randomPosition, randomRotation, Vec3(1.0f, 1.0f, 1.0f)));
	//    ECS::GetInstance().AddComponent(entity, graphics::GeometryFactory::CreateCube(1.0f, 1.0f, 1.0f));
	//
	//    // Use shared material for all these entities - much more memory efficient!
	//    ECS::GetInstance().AddComponent(entity, Material(metalMaterial));
	//}

	auto audioTestEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(audioTestEntity, Transform(Vec3(2, 0, -1), Quaternion(), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent<ObjectMetaData>(audioTestEntity, ObjectMetaData());

	AudioComponent testAudio;
	//testAudio.soundName = "../Resources/Audio/test.wav"; // Replace with your actual sound file path
	//testAudio.volume = 0.5f; // 50% volume
	//testAudio.is3D = false; // 2D sound for testing
	//testAudio.isLooping = false;
	//testAudio.isStreaming = false;
	//testAudio.shouldPlay = true; // We'll trigger this with keyboard input

	ECS::GetInstance().AddComponent(audioTestEntity, testAudio);

	EE_CORE_INFO("Audio test entity created with ID: {} - will auto-play", audioTestEntity);

	//auto entity3 = ECS::GetInstance().CreateEntity();
	//ECS::GetInstance().AddComponent(entity3, Transform(Vec3(1, 1, -3), Vec3(0, 0, 0), Vec3(1, 1, 1)));
	//ECS::GetInstance().AddComponent(entity3, graphics::GeometryFactory::CreateSphere());
	//ECS::GetInstance().AddComponent(entity3, Material(shader, texture));

	// Example FBX entity
	auto fbxEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(fbxEntity, Transform(Vec3(0, 0, -1), Quaternion(), Vec3(0.01f, 0.01f, 0.01f)));
	ECS::GetInstance().AddComponent<ObjectMetaData>(fbxEntity, ObjectMetaData("Character", "Model", true));
	ECS::GetInstance().AddComponent<Mesh>(fbxEntity, Mesh{}); // empty mesh component for renderer signature
	ECS::GetInstance().AddComponent<ModelComponent>(fbxEntity, ModelComponent(AssetManager::GetInstance().LoadModel("../Resources/Models/Walking.fbx")));
	
	// Create a pure metallic material without texture fallback
	auto cubeFBXMaterial = std::make_unique<graphics::Material>(shader);
	cubeFBXMaterial->LoadTemplate(graphics::MaterialTemplates::PBR_METAL());
	
	// Only set texture if we specifically want this model to be textured
	// Remove automatic texture assignment to prevent unwanted texture loading
	// if (texture && texture->IsValid()) {
	//     cubeFBXMaterial->SetTexture("materialAlbedoMap", texture);
	//     cubeFBXMaterial->SetBool("materialHasAlbedoMap", true);
	// }
	
	ECS::GetInstance().AddComponent(fbxEntity, Material(std::move(cubeFBXMaterial)));

	// Create a simple quad mesh for particles
	auto quadMesh = graphics::GeometryFactory::CreateQuad(1.0f, 1.0f);
	auto tex = AssetManager::GetInstance().LoadTexture("../Resources/Textures/greybox_red_solid.png");

	// Particles Emitter
	emitter = std::make_unique<ParticleEmitter>(quadMesh, shader, tex);

	// Create first cube
	//auto entity = ECS::GetInstance().CreateEntity();
	//ECS::GetInstance().AddComponent(entity, Transform(Vec3(0, 0, -1), Vec3(0, 45, 90), Vec3(1, 1, 1)));
	//ECS::GetInstance().AddComponent(entity, ObjectMetaData());
	//ECS::GetInstance().AddComponent(entity, graphics::GeometryFactory::CreateCube(1, 1, 1));

	RegisterDefaultAllocator();

	gPhysics = new Physics();
	gPhysics->Init();
	gPhysics->CreatePhysicsBox(Vec3(0, 5, 0), Vec3(1, 1, 1), 1.0f);
	gPhysics->CreatePhysicsBox(Vec3(0, 8, 0), Vec3(1, 1, 1), 1.0f);

	gPhysics->CreatePhysicsBox(Vec3(0, 0, 0), Vec3(1, 1, 1), 0.0f);

	//InspectorGUI inspector{ entity, "Inspector" };
	//inspector.SetEntity(entity);

	// Create material using UBO template - pure color-based without texture fallback
	auto cubeMaterial = std::make_unique<graphics::Material>(shader);
	cubeMaterial->LoadTemplate(graphics::MaterialTemplates::PBR_WHITE());

	// Removed: Automatic texture assignment to prevent unwanted texture loading
	// Only set textures when explicitly needed for specific visual effects
	// if (texture && texture->IsValid()) {
	//     cubeMaterial->SetTexture("materialAlbedoMap", texture);
	//     cubeMaterial->SetBool("materialHasAlbedoMap", true);
	// }

	//ECS::GetInstance().AddComponent(entity, Material(std::move(cubeMaterial)));

	// Create second cube  
	auto entity2 = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(entity2, Transform(Vec3(0, -1, 0), Quaternion(), Vec3(100, 0.1f, 100)));
	ECS::GetInstance().AddComponent<ObjectMetaData>(entity2, ObjectMetaData());
	ECS::GetInstance().AddComponent<Mesh>(entity2, graphics::GeometryFactory::CreateCube(1, 1, 1));

	// Create a reflective material for demonstration - this one is unique
	auto cube2Material = std::make_shared<graphics::Material>(shader);
	cube2Material->LoadTemplate(graphics::MaterialTemplates::PBR_REFLECTIVE(0.9f, 0.1f)); // Highly reflective metal

	// Removed: Automatic texture assignment to prevent unwanted texture loading
	// Only set textures when explicitly needed for visual effects
	// if (texture && texture->IsValid()) {
	//     cube2Material->SetTexture("materialAlbedoMap", texture);
	//     cube2Material->SetBool("materialHasAlbedoMap", true);
	// }

	// Example: Add environment mapping to the material
	// If you have a cubemap loaded, you can set it like this:
	if (environmentCubemap && environmentCubemap->IsValid()) {
	    cube2Material->SetCubemap("materialEnvironmentMap", environmentCubemap);
	    cube2Material->SetCubemap("materialIrradianceMap", environmentCubemap); // You'd typically use a separate irradiance map
	    cube2Material->SetBool("materialHasEnvironmentMap", true);
	    cube2Material->SetBool("materialHasIrradianceMap", true);
	    cube2Material->SetFloat("materialEnvironmentIntensity", 1.0f);
	    EE_CORE_INFO("Environment mapping applied to reflective cube");
	}

	ECS::GetInstance().AddComponent(entity2, Material(cube2Material));
	ECS::GetInstance().AddComponent(entity2, Script("Sandbox", entity2));

	//// Create lights with balanced intensities
	auto mainLightEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(mainLightEntity, Transform(Vec3(0, 4, 2), Quaternion(0.9f, 0.2f, 0.1f, -0.3f), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent<ObjectMetaData>(mainLightEntity, ObjectMetaData("MainLight", "Light", true));
	ECS::GetInstance().AddComponent<Light>(mainLightEntity, Light(Vec3(1, 1, 1), 0.8f, LightType::DIRECTIONAL, true));
	auto yellowLightSpot = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(yellowLightSpot, Transform(Vec3(0, 10, 0), Quaternion(0.707f, 0.f, 0.f, 0.707f), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent<ObjectMetaData>(yellowLightSpot, ObjectMetaData("Red", "Light", true));
	ECS::GetInstance().AddComponent<Light>(yellowLightSpot, Light(Vec3(1, 0.8f, 0.6f), 1.f, LightType::SPOT, true, 50,60,100.f));

	// Light sphere material - use shared emissive material for all lights
	//ECS::GetInstance().AddComponent(mainLightEntity, graphics::GeometryFactory::CreateSphere(0.1f));
	//ECS::GetInstance().AddComponent(mainLightEntity, Material(emissiveWhiteMaterial));

	// Red accent light - create unique colored emissive materials
	auto redLightEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(redLightEntity, Transform(Vec3(3, 2, 0), Quaternion(), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent<ObjectMetaData>(redLightEntity, ObjectMetaData("LightRed", "Light", true));
	ECS::GetInstance().AddComponent<Light>(redLightEntity, Light(Vec3(1, 0.0, 0.0), 0.5f, LightType::POINT));

	auto redLightMaterial = std::make_shared<graphics::Material>(shader);
	redLightMaterial->LoadTemplate(graphics::MaterialTemplates::EMISSIVE(Vec3(1.0f, 0.f, 0.f), 10.0f));
	ECS::GetInstance().AddComponent(redLightEntity, graphics::GeometryFactory::CreateSphere(0.1f));
	ECS::GetInstance().AddComponent(redLightEntity, Material(redLightMaterial));

	// Blue accent light
	auto blueLightEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(blueLightEntity, Transform(Vec3(-3, 2, 0), Quaternion(), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent(blueLightEntity, ObjectMetaData("LightBlue", "Light", true));
	ECS::GetInstance().AddComponent(blueLightEntity, Light(Vec3(0.0, 0.0, 1), 0.5f, LightType::POINT));

	auto blueLightMaterial = std::make_shared<graphics::Material>(shader);
	blueLightMaterial->LoadTemplate(graphics::MaterialTemplates::EMISSIVE(Vec3(0.f, 0.f, 1.0f), 10.0f));
	ECS::GetInstance().AddComponent(blueLightEntity, graphics::GeometryFactory::CreateSphere(0.1f));
	ECS::GetInstance().AddComponent(blueLightEntity, Material(blueLightMaterial));

	// Green accent light
	auto greenLightEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(greenLightEntity, Transform(Vec3(0, 2, -3), Quaternion(), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent(greenLightEntity, ObjectMetaData("LightGreen", "Light", true));
	ECS::GetInstance().AddComponent(greenLightEntity, Light(Vec3(0.0, 1.0f, 0.0), 0.5f, LightType::POINT));

	auto greenLightMaterial = std::make_shared<graphics::Material>(shader);
	greenLightMaterial->LoadTemplate(graphics::MaterialTemplates::EMISSIVE(Vec3(0.f, 1.f, 0.0f), 10.0f));
	ECS::GetInstance().AddComponent(greenLightEntity, graphics::GeometryFactory::CreateSphere(0.1f));
	ECS::GetInstance().AddComponent(greenLightEntity, Material(greenLightMaterial));

	// Create demo objects showcasing refraction and local reflection probes
	// Glass sphere demonstrating refraction
	auto glassEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(glassEntity, Transform(Vec3(2, 0, -1), Quaternion(), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent(glassEntity, ObjectMetaData("GlassSphere", "Transparent", true));
	ECS::GetInstance().AddComponent(glassEntity, graphics::GeometryFactory::CreateSphere(0.8f));
	ECS::GetInstance().AddComponent(glassEntity, Material(glassMaterial));

	// Water-like cube
	auto waterEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(waterEntity, Transform(Vec3(-2, 0, -1), Quaternion(), Vec3(1, 1, 0.3f)));
	ECS::GetInstance().AddComponent(waterEntity, ObjectMetaData("WaterSlab", "Transparent", true));
	ECS::GetInstance().AddComponent(waterEntity, graphics::GeometryFactory::CreateCube(2, 2, 0.6f));
	ECS::GetInstance().AddComponent(waterEntity, Material(waterMaterial));

	// Create local reflection probes
	// Main reflection probe near the center
	auto mainProbeEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(mainProbeEntity, Transform(Vec3(0, 1, -1), Quaternion(), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent(mainProbeEntity, ObjectMetaData("MainReflectionProbe", "ReflectionProbe", true));
	
	ReflectionProbe mainProbe(Vec3(0, 1, -1), Vec3(8, 8, 8), 1.0f);
	mainProbe.blendDistance = 2.0f;
	mainProbe.priority = 1;
	mainProbe.reflectionCubemap = environmentCubemap; // Use global environment as fallback
	mainProbe.boxMin = Vec3(-4, -4, -4);
	mainProbe.boxMax = Vec3(4, 4, 4);
	ECS::GetInstance().AddComponent(mainProbeEntity, mainProbe);
	
	// Secondary reflection probe for local area  
	auto localProbeEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(localProbeEntity, Transform(Vec3(3, 1, 0), Quaternion(), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent(localProbeEntity, ObjectMetaData("LocalReflectionProbe", "ReflectionProbe", true));
	
	ReflectionProbe localProbe(Vec3(3, 1, 0), Vec3(6, 6, 6), 0.8f);
	localProbe.blendDistance = 1.5f;
	localProbe.priority = 2;
	localProbe.reflectionCubemap = environmentCubemap; // Use global environment as fallback
	localProbe.boxMin = Vec3(-3, -3, -3);
	localProbe.boxMax = Vec3(3, 3, 3);
	ECS::GetInstance().AddComponent(localProbeEntity, localProbe);

	EE_CORE_INFO("Total living entities after creation: {0}", ECS::GetInstance().GetLivingEntityCount());
	EE_CORE_INFO("Created demo objects with refraction materials and local reflection probes");

	// Init Renderer after objects haVe been initialised
	ECS::GetInstance().GetSystem<graphics::Renderer>()->Init(1280, 720);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // Background color

	// Create ImGUI window for Asset Browser
	editor::EditorGUI::CreateImGUIWindow<ImguiUI::AssetBrowser>();
	editor::EditorGUI::CreateImGUIWindow<ParticlesImGUI>(emitter.get());
	editor::EditorGUI::CreateImGUIWindow<AudioImGUI>();
	// Create ImGUI window for Inspector
	//editor::EditorGUI::CreateImGUIWindow<InspectorGUI>();
	editor::EditorGUI::CreateImGUIWindow<InspectorGUI>(entity2, "Inspector");
   
	// Demonstrate different material sharing strategies:
	// 1. Use completely shared material (multiple entities, same appearance)
	//    Example: ECS::GetInstance().AddComponent(anotherEntity, Material(basicWhiteMaterial)); // Same material instance
	// 2. Create unique materials when needed (entities with unique appearance)
	//    Example: auto customMaterial = std::make_shared<graphics::Material>(shader); // Unique material
	// 3. Clone and modify shared materials (similar but slightly different materials)
	//    Example: auto customMaterial = std::make_shared<graphics::Material>(*basicWhiteMaterial);  // Copy
	//             customMaterial->SetFloat("material.roughness", 0.8f);  // Modify the copy

	EE_CORE_INFO("Material system now supports efficient sharing between entities using shared_ptr");
	EE_CORE_INFO("Advanced features implemented:");
	EE_CORE_INFO("  - Fixed reflection with environment cubemaps and local probes");
	EE_CORE_INFO("  - Correct refraction with IOR support (Glass: 1.5, Water: 1.33)");
	EE_CORE_INFO("  - Proper Fresnel-based reflection/refraction mixing");
	EE_CORE_INFO("  - Enhanced material templates with all required parameters");
	EE_CORE_INFO("  - Key controls: 1=PBR, 2=Blinn-Phong, 3=Toggle Deferred");
	EE_CORE_INFO("Systems and components registered successfully, Engine Initialized");
	s_isInitialized = true;
}

// TODO: Shutdown for subsystem should be in order, please be mindful of the order that is already in place.
void engine::Shutdown()
{
	if (!s_isInitialized)
		return;

	//const std::filesystem::path scenePath = "Ermine-Engine.scene";
	//SaveSceneToFile("Ermine-Engine", scenePath);

	Config cfg{};
	int width, height;
	glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
	cfg.windowWidth = width;
	cfg.windowHeight = height;
	cfg.fullscreen = (glfwGetWindowMonitor(glfwGetCurrentContext()) != nullptr);
	cfg.maximized = (glfwGetWindowAttrib(glfwGetCurrentContext(), GLFW_MAXIMIZED) == GLFW_TRUE);
	cfg.title = "Ermine Editor 0.1";

	SaveConfigToFile(cfg, "Ermine-Engine.config", false);

    AssetManager::GetInstance().Clear();
	gPhysics->Shutdown();
	delete gPhysics;
	gPhysics = nullptr;
	emitter.reset();
	skybox.reset();
	environmentCubemap.reset();

    graphics::GPUProfiler::Shutdown();

#ifdef _DEBUG
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

	// Handle shading mode toggle
	HandleShadingToggle(windowContext);

	// Profiler here
	graphics::GPUProfiler::BeginFrame();

	// Update FrameController
	FrameController::BeginFrame();

	// 1. Update input states
	Input::Update();

	glfwPollEvents(); // Do not move me above Input::Update - Friendly Adviser

	// 2. Game state update
	while (FrameController::ShouldUpdateFixed())
	{
		// Fixed update logic here
		ECS::GetInstance().GetSystem<scripting::ScriptSystem>()->FixedUpdate();
	}

	// Other non-fixed logic here
	ECS::GetInstance().GetSystem<scripting::ScriptSystem>()->Update();
	ECS::GetInstance().GetSystem<AudioSystem>()->Update();
	// Update editor camera
	editor::EditorCamera::GetInstance().Update();
	gPhysics->Update(FrameController::GetDeltaTime());

	// Simple test to see if we can select an entity and view it in the inspector
	//if (Input::IsKeyDown(GLFW_KEY_Q))
		//InspectorGUI::SetEntity(entity);

	/*
	if (s_isInitialized && emitter)
	{
		// Emit x number of particles each frame
		for (int i = 0; i < 2; i++)
		{
			Vec3 vel = { ((rand() % 100) / 100.0f - 0.5f) * 2.0f, 2.0f, 0.0f };
			emitter->Emit({ 0,0,-3 }, vel, 2.0f, 0.5f, { 1,0,0,1 });
		}
	}*/
	// Update for Particles
	ECS::GetInstance().GetSystem<ParticleSystem>()->Update(FrameController::GetDeltaTime());
}

void engine::Render(GLFWwindow* window)
{
	if (!s_isInitialized)
		return;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	Mtx44 view = editor::EditorCamera::GetInstance().GetViewMatrix();
	Mtx44 proj = editor::EditorCamera::GetInstance().GetProjectionMatrix();

	// Start GPU timing for rendering
	graphics::GPUProfiler::BeginEvent("Frame");

	ECS::GetInstance().GetSystem<graphics::Renderer>()->Clear();

	// Pass skybox to renderer so it can be rendered in the proper framebuffer
	auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
	if (skybox && skybox->IsValid()) {
		renderer->SetSkybox(skybox.get());
	}

	// Draw scene objects (this now handles skybox, deferred/forward rendering internally)
	renderer->Update(view, proj);

	graphics::GPUProfiler::EndEvent();

	// Render ImGui/Editor on top of everything
	if (editor::EditorGUI::IsInit())
		editor::EditorGUI::Render(); // Render the ImGUI context on-top of the scene

	graphics::GPUProfiler::EndFrame();

	glfwSwapBuffers(window);
}

// Free to use for testing purposes
void engine::Dummy([[maybe_unused]] GLFWwindow* wwindow)
{
	// graphics::VertexBuffer vertex_buffer(vertices, sizeof(vertices));
	//
	// auto shader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/vertex_shader_debug.glsl", "../Resources/Shaders/fragment_shader_debug.glsl");
	// const GLint mvp_location = glGetUniformLocation(shader->GetRendererID(), "MVP");
	//
	// graphics::VertexArray vertex_array;
	// vertex_array.LinkAttribute(0,3,GL_FLOAT, sizeof(Vertex),(void*)offsetof(Vertex, pos));
	// vertex_array.LinkAttribute(1, 3, GL_FLOAT, sizeof(Vertex), (void*) offsetof(Vertex, col));
	//
	// graphics::IndexBuffer ibo(indices, sizeof(indices));

	// Create graphics resources
	// graphics::VertexBuffer vbo(vertices, sizeof(vertices));
	// auto shader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/vertex.glsl", "../Resources/Shaders/fragment.glsl");
	// const GLint mvp_location = glGetUniformLocation(shader->GetRendererID(), "MVP");
	//
	// graphics::VertexArray vao;
	// vao.LinkAttribute(0, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, pos));
	// vao.LinkAttribute(1, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, col));
	// vbo.Unbind();
	//
	// graphics::IndexBuffer ibo(indices, sizeof(indices));
	//
	// auto entity = ECS::GetInstance().CreateEntity();
	// ECS::GetInstance().AddComponent(entity, Transform());
	// ECS::GetInstance().AddComponent(entity, Mesh(std::make_shared<graphics::VertexArray>(vao),std::make_shared<graphics::VertexBuffer>(vbo),std::make_shared<graphics::IndexBuffer>(ibo))); // Basically passing the ID of the VAO, VBO, and IBO
	// ECS::GetInstance().AddComponent(entity, Material(shader));

	// FrameController frame_controller(300.0f,60.0f);
	// while (!glfwWindowShouldClose(wwindow))
	// {
	//     frame_controller.BeginFrame();
	//     int width, height;
	//     glfwGetFramebufferSize(wwindow, &width, &height);
	//     const float ratio = width / (float) height;
	//
	//     glViewport(0, 0, width, height);
	//     glClear(GL_COLOR_BUFFER_BIT);
	//
	//     glm::mat4 model = glm::mat4(1.0f);
	//     glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	//     glm::mat4 projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
	//     glm::mat4 mvp = projection * view * model;
	//
	//     ECS::GetInstance().GetSystem<graphics::Renderer>()->Update(Mtx44(),Mtx44());
		// glUseProgram(shader->GetRendererID());
		// glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) &mvp);
		// vao.Bind();
		// glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(ibo.GetCount()), GL_UNSIGNED_INT, 0);

		// glUseProgram(shader->GetRendererID());
		// glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) &mvp);
		// vertex_array.Bind();
		// glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(ibo.GetCount()), GL_UNSIGNED_INT, 0);
		// glDrawArrays(GL_TRIANGLES, 0, 3);

		// glfwSwapBuffers(wwindow);
		// glfwPollEvents();
		//
		// if (GLFW_PRESS == glfwGetKey(wwindow, GLFW_KEY_ESCAPE)) {
		//     glfwSetWindowShouldClose(wwindow, 1);
		// }
	// }
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
		renderer->SetShadingMode(false); // false = PBR
		EE_CORE_INFO("Switched to PBR shading");
	}

	// Toggle to Blinn-Phong (key 2)  
	if (key2IsPressed && !key2WasPressed) {
		auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
		renderer->SetShadingMode(true); // true = Blinn-Phong
		EE_CORE_INFO("Switched to Blinn-Phong shading");
	}

	// Toggle to Deferred (key 3)
	if (key3IsPressed && !key3WasPressed) {
		auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
		renderer->ToggleDeferredRendering();
	}
	// Toggle to Deferred (key 4)
	if (key4IsPressed && !key4WasPressed) {
		auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
		renderer->m_SSAOEnabled = !renderer->m_SSAOEnabled;
	}

	key1WasPressed = key1IsPressed;
	key2WasPressed = key2IsPressed;
	key3WasPressed = key3IsPressed;
	key4WasPressed = key4IsPressed;
}
