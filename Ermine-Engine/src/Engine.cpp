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
#include "GraphicsDebugGUI.h"
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
#include <random>

#include "ScriptSystem.h"

using namespace Ermine;

#define EE_AUTO_REGISTER_COMPONENT(Type, Name) \
	static bool _##Type##_autoreg = [](){ Ermine::ECS::GetInstance().RegisterComponent<Type>(Name); return true; }();

namespace
{
	bool s_isInitialized = false;

	// For Particles
	static std::shared_ptr<Ermine::graphics::Shader> particleShader;
	static std::unique_ptr<Ermine::ParticleEmitter> emitter;

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
}

bool engine::Init(GLFWwindow* windowContext)
{
	if (s_isInitialized) // Already initialized
		return true;

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
		EE_AUTO_REGISTER_COMPONENT(ObjectMetaData, "ObjectMetaData")
		EE_AUTO_REGISTER_COMPONENT(Light, "Light")
		EE_AUTO_REGISTER_COMPONENT(Particle, "Particle")
		EE_AUTO_REGISTER_COMPONENT(AudioComponent, "AudioComponent")
		EE_AUTO_REGISTER_COMPONENT(GlobalAudioComponent, "GlobalAudioComponent")
		EE_AUTO_REGISTER_COMPONENT(ModelComponent, "ModelComponent")

		// Special Case for Script component
		ECS::GetInstance().RegisterComponent<Script>("Script",
			[](Ermine::ComponentManager& cm, EntityID src, EntityID dst)
			{
				if (!cm.HasComponent<Script>(src)) return;
				auto& srcScript = cm.GetComponent<Script>(src);
				cm.AddComponent<Script>(dst, Script(srcScript.m_className, dst));
			});

	// Register all systems
	ECS::GetInstance().RegisterSystem<graphics::Renderer>();
	ECS::GetInstance().RegisterSystem<scripting::ScriptSystem>();
	ECS::GetInstance().RegisterSystem<AudioSystem>();
	ECS::GetInstance().RegisterSystem<ParticleSystem>();
	ECS::GetInstance().RegisterSystem<graphics::LightSystem>();

	// Set system signatures
	SignatureID sig;
	sig.set(ECS::GetInstance().GetComponentType<Transform>());
	sig.set(ECS::GetInstance().GetComponentType<Mesh>());
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

	// Audio test entity
	auto audioTestEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(audioTestEntity, Transform(Vec3(2, 0, -1), Quaternion(), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent<ObjectMetaData>(audioTestEntity, ObjectMetaData());

	AudioComponent testAudio;
	ECS::GetInstance().AddComponent(audioTestEntity, testAudio);
	EE_CORE_INFO("Audio test entity created with ID: {}", audioTestEntity);

	// Example FBX entity
	auto fbxEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(fbxEntity, Transform(Vec3(0, 0, -1), Quaternion(), Vec3(0.01f, 0.01f, 0.01f)));
	ECS::GetInstance().AddComponent<ObjectMetaData>(fbxEntity, ObjectMetaData("Character", "Model", true));
	ECS::GetInstance().AddComponent<Mesh>(fbxEntity, Mesh{});
	ECS::GetInstance().AddComponent<ModelComponent>(fbxEntity, ModelComponent(AssetManager::GetInstance().LoadModel("../Resources/Models/Walking.fbx")));

	auto cubeFBXMaterial = std::make_unique<graphics::Material>(shader);
	auto fbxTexture = AssetManager::GetInstance().LoadTexture("../Resources/Textures/Pants_Base_color.png");
	cubeFBXMaterial->LoadTemplate(graphics::MaterialTemplates::PBR_METAL());

	if (fbxTexture && fbxTexture->IsValid()) {
		cubeFBXMaterial->SetTexture("materialAlbedoMap", fbxTexture);
		cubeFBXMaterial->SetBool("materialHasAlbedoMap", true);
	}

	ECS::GetInstance().AddComponent(fbxEntity, Material(std::move(cubeFBXMaterial)));

	// Create a simple quad mesh for particles
	auto quadMesh = graphics::GeometryFactory::CreateQuad(1.0f, 1.0f);
	auto tex = AssetManager::GetInstance().LoadTexture("../Resources/Textures/greybox_red_solid.png");

	// Particles Emitter
	emitter = std::make_unique<ParticleEmitter>(quadMesh, shader, tex);

	RegisterDefaultAllocator();

	gPhysics = new Physics();
	gPhysics->Init();
	gPhysics->CreatePhysicsBox(Vec3(0, 5, 0), Vec3(1, 1, 1), 1.0f);
	gPhysics->CreatePhysicsBox(Vec3(0, 8, 0), Vec3(1, 1, 1), 1.0f);
	gPhysics->CreatePhysicsBox(Vec3(0, 0, 0), Vec3(1, 1, 1), 0.0f);

	// This is the floor  
	auto entity2 = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(entity2, Transform(Vec3(0, -1, 0), Quaternion(), Vec3(100, 0.1f, 100)));
	ECS::GetInstance().AddComponent<ObjectMetaData>(entity2, ObjectMetaData());
	ECS::GetInstance().AddComponent<Mesh>(entity2, graphics::GeometryFactory::CreateCube(1, 1, 1));

	// Apply texture to floor
	auto cube2Material = std::make_shared<graphics::Material>(shader);
	ECS::GetInstance().AddComponent(entity2, Material(cube2Material));
	ECS::GetInstance().AddComponent(entity2, Script("Sandbox", entity2));
	if (texture && texture->IsValid()) {
		cube2Material->SetTexture("materialAlbedoMap", texture);
		cube2Material->SetBool("materialHasAlbedoMap", true);
	}

	// Create lights with balanced intensities
	auto mainLightEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(mainLightEntity, Transform(Vec3(0, 4, 2), Quaternion(0.9f, 0.2f, 0.1f, -0.3f), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent<ObjectMetaData>(mainLightEntity, ObjectMetaData("MainLight", "Light", true));
	ECS::GetInstance().AddComponent<Light>(mainLightEntity, Light(Vec3(1, 1, 1), 0.8f, LightType::DIRECTIONAL, true));

	auto yellowLightSpot = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(yellowLightSpot, Transform(Vec3(0, 10, 0), Quaternion(0.707f, 0.f, 0.f, 0.707f), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent<ObjectMetaData>(yellowLightSpot, ObjectMetaData("Red", "Light", true));
	ECS::GetInstance().AddComponent<Light>(yellowLightSpot, Light(Vec3(1, 0.8f, 0.6f), 1.f, LightType::SPOT, true, 50, 60, 100.f));

	// Red accent light
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
	ECS::GetInstance().AddComponent<ObjectMetaData>(blueLightEntity, ObjectMetaData("LightBlue", "Light", true));
	ECS::GetInstance().AddComponent<Light>(blueLightEntity, Light(Vec3(0.0, 0.0, 1), 0.5f, LightType::POINT));

	auto blueLightMaterial = std::make_shared<graphics::Material>(shader);
	blueLightMaterial->LoadTemplate(graphics::MaterialTemplates::EMISSIVE(Vec3(0.f, 0.f, 1.0f), 10.0f));
	ECS::GetInstance().AddComponent(blueLightEntity, graphics::GeometryFactory::CreateSphere(0.1f));
	ECS::GetInstance().AddComponent(blueLightEntity, Material(blueLightMaterial));

	// Green accent light
	auto greenLightEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(greenLightEntity, Transform(Vec3(0, 2, -3), Quaternion(), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent<ObjectMetaData>(greenLightEntity, ObjectMetaData("LightGreen", "Light", true));
	ECS::GetInstance().AddComponent<Light>(greenLightEntity, Light(Vec3(0.0, 1.0f, 0.0), 0.5f, LightType::POINT));

	auto greenLightMaterial = std::make_shared<graphics::Material>(shader);
	greenLightMaterial->LoadTemplate(graphics::MaterialTemplates::EMISSIVE(Vec3(0.f, 1.f, 0.0f), 10.0f));
	ECS::GetInstance().AddComponent(greenLightEntity, graphics::GeometryFactory::CreateSphere(0.1f));
	ECS::GetInstance().AddComponent(greenLightEntity, Material(greenLightMaterial));

	// Glass sphere demonstrating refraction
	auto glassEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(glassEntity, Transform(Vec3(2, 0, -1), Quaternion(), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent(glassEntity, ObjectMetaData("GlassSphere", "Transparent", true));
	ECS::GetInstance().AddComponent(glassEntity, graphics::GeometryFactory::CreateSphere(0.8f));
	// Use the previously created glass material
	ECS::GetInstance().AddComponent(glassEntity, Material(glassMaterial));

	// Coloured glass
	auto waterEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(waterEntity, Transform(Vec3(-2, 0, -1), Quaternion(), Vec3(1, 1, 0.3f)));
	ECS::GetInstance().AddComponent(waterEntity, ObjectMetaData("ColouredGlass", "Transparent", true));
	ECS::GetInstance().AddComponent(waterEntity, graphics::GeometryFactory::CreateCube(2, 2, 0.6f));
	// Use the previously created water material
	ECS::GetInstance().AddComponent(waterEntity, Material(waterMaterial));

	// Metallic cube
	auto metalCubeEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(metalCubeEntity, Transform(Vec3(-4, 0, -1), Quaternion(), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent<ObjectMetaData>(metalCubeEntity, ObjectMetaData("MetalCube", "Metal", true));
	// Use the previously created metal material
	metalMaterial->LoadTemplate(graphics::MaterialTemplates::PBR_METAL());
	ECS::GetInstance().AddComponent(metalCubeEntity, graphics::GeometryFactory::CreateCube(1, 1, 1));
	ECS::GetInstance().AddComponent(metalCubeEntity, Material(metalMaterial));
	if (texture && texture->IsValid()) {
		metalMaterial->SetTexture("materialAlbedoMap", texture);
		metalMaterial->SetBool("materialHasAlbedoMap", true);
	}

	// Red cube
	auto redCubeEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent<Transform>(redCubeEntity, Transform(Vec3(-6, 0, -1), Quaternion(), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent<ObjectMetaData>(redCubeEntity, ObjectMetaData("redCube", "Red", true));

	// Create red material instance for this cube
	auto redMaterial = std::make_shared<graphics::Material>(shader);
	redMaterial->LoadTemplate(graphics::MaterialTemplates::PBR_RED());
	ECS::GetInstance().AddComponent(redCubeEntity, graphics::GeometryFactory::CreateCube(1, 1, 1));
	ECS::GetInstance().AddComponent(redCubeEntity, Material(redMaterial));
	if (texture && texture->IsValid()) {
		redMaterial->SetTexture("materialAlbedoMap", texture);
		redMaterial->SetBool("materialHasAlbedoMap", true);
	}

	EE_CORE_INFO("Total living entities after creation: {0}", ECS::GetInstance().GetLivingEntityCount());

	// Init Renderer after objects have been initialised
	ECS::GetInstance().GetSystem<graphics::Renderer>()->Init(1280, 720);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

	// Create ImGUI windows
	editor::EditorGUI::CreateImGUIWindow<ImguiUI::AssetBrowser>();
	editor::EditorGUI::CreateImGUIWindow<ParticlesImGUI>(emitter.get());
	editor::EditorGUI::CreateImGUIWindow<AudioImGUI>();
	editor::EditorGUI::CreateImGUIWindow<InspectorGUI>(entity2, "Inspector");
	editor::EditorGUI::CreateImGUIWindow<editor::GraphicsDebugGUI>("Graphics Debug");

	s_isInitialized = true;
	return true;
}

void engine::Shutdown()
{
	if (!s_isInitialized)
		return;

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

	// Profiler
	graphics::GPUProfiler::BeginFrame();

	// Update FrameController
	FrameController::BeginFrame();

	// Update input states
	Input::Update();

	glfwPollEvents();

	// Game state update
	while (FrameController::ShouldUpdateFixed())
	{
		ECS::GetInstance().GetSystem<scripting::ScriptSystem>()->FixedUpdate();
	}

	// Other non-fixed logic
	ECS::GetInstance().GetSystem<scripting::ScriptSystem>()->Update();
	ECS::GetInstance().GetSystem<AudioSystem>()->Update();

	// Update editor camera
	editor::EditorCamera::GetInstance().Update();
	gPhysics->Update(FrameController::GetDeltaTime());

	// Update particles
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

	// Draw scene objects
	auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
	renderer->Update(view, proj);

	graphics::GPUProfiler::EndEvent();

	// Render ImGui/Editor on top of everything
	if (editor::EditorGUI::IsInit())
		editor::EditorGUI::Render();

	graphics::GPUProfiler::EndFrame();

	glfwSwapBuffers(window);
}

void engine::Dummy([[maybe_unused]] GLFWwindow* wwindow)
{
	// Empty dummy function for testing
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