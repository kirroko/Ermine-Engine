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
#include "AudioImGUI.h"

#include <random> // Include for random number generation

#include "ScriptSystem.h"

using namespace Ermine;

namespace
{
	bool s_isInitialized = false;
	//std::unique_ptr<editor::EditorCamera> s_EditorCamera = nullptr;

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
	    EE_CORE_INFO("Loaded config: {}x{}, fullscreen={}, maximised={}, title={}",
	        cfg.windowWidth, cfg.windowHeight, cfg.fullscreen, cfg.title);
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
	ECS::GetInstance().RegisterComponent<Transform>();
	ECS::GetInstance().RegisterComponent<Rigidbody3D>();
	ECS::GetInstance().RegisterComponent<Mesh>();
	ECS::GetInstance().RegisterComponent<Material>();
	ECS::GetInstance().RegisterComponent<Script>();
	ECS::GetInstance().RegisterComponent<ObjectMetaData>();
	ECS::GetInstance().RegisterComponent<Light>();
	ECS::GetInstance().RegisterComponent<Particle>();

	ECS::GetInstance().RegisterComponent<AudioComponent>(); // ADD THIS
	ECS::GetInstance().RegisterComponent<GlobalAudioComponent>(); // ADD THIS IF YOU WANT GLOBAL AUDIO

	// TODO: Register all systems here, no limits
	ECS::GetInstance().RegisterSystem<graphics::Renderer>();
	ECS::GetInstance().RegisterSystem<scripting::ScriptSystem>();
	ECS::GetInstance().RegisterSystem<AudioSystem>();
	ECS::GetInstance().RegisterSystem<ParticleSystem>();

	// TODO: Set the signature for the system as required
	// For Graphics/Renderer system
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

	glfwSetFramebufferSizeCallback(windowContext, []([[maybe_unused]] GLFWwindow* window, int width, int height)
		{
#ifdef _DEBUG
			editor::EditorCamera::GetInstance().SetViewportSize(static_cast<float>(width), static_cast<float>(height));
#endif
			glViewport(0, 0, width, height);
		});

	// Create graphics resources
	ECS::GetInstance().GetSystem<graphics::Renderer>()->Init(1280, 720);
	auto shader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/vertex.glsl", "../Resources/Shaders/fragment.glsl");
	auto texture = AssetManager::GetInstance().LoadTexture("../Resources/Textures/greybox_grey_grid.png");

	// Random number generation setup
	//std::random_device rd;
	//std::mt19937 gen(rd());
	//std::uniform_real_distribution<float> posDist(-10.0f, 10.0f); // Random positions between -10 and 10
	//std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);  // Random rotations between 0 and 360

	//for (int i = 0; i < 1000; ++i)
	//{
	//    auto entity = ECS::GetInstance().CreateEntity();
	//    Vec3 randomPosition(posDist(gen), posDist(gen), posDist(gen));
	//    Vec3 randomRotation(rotDist(gen), rotDist(gen), rotDist(gen));
	//    ECS::GetInstance().AddComponent(entity, Transform(randomPosition, randomRotation, Vec3(1.0f, 1.0f, 1.0f)));
	//    ECS::GetInstance().AddComponent(entity, graphics::GeometryFactory::CreateCube(1.0f, 1.0f, 1.0f));
	//    ECS::GetInstance().AddComponent(entity, Material(shader, texture));
	//}
	
	auto audioTestEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent(audioTestEntity, Transform(Vec3(2, 0, -1), Vec3(0, 0, 0), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent(audioTestEntity, ObjectMetaData());

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

	// Create a simple quad mesh for particles
	auto quadMesh = graphics::GeometryFactory::CreateQuad(1.0f, 1.0f);
	auto tex = AssetManager::GetInstance().LoadTexture("../Resources/Textures/greybox_red_solid.png");
	particleShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/vertex.glsl", "../Resources/Shaders/fragment.glsl");

	// Particles Emitter
	emitter = std::make_unique<ParticleEmitter>(quadMesh, particleShader, tex);

	// Create first cube
	auto entity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent(entity, Transform(Vec3(0, 0, -1), Vec3(0, 45, 90), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent(entity, ObjectMetaData());
	ECS::GetInstance().AddComponent(entity, graphics::GeometryFactory::CreateCube(1, 1, 1));

	// Create material using UBO template
	auto cubeMaterial = std::make_unique<graphics::Material>(shader);
	cubeMaterial->LoadTemplate(graphics::MaterialTemplates::PBR_WHITE());

	// Set texture if available
	if (texture && texture->IsValid()) {
		cubeMaterial->SetTexture("materialAlbedoMap", texture);
		cubeMaterial->SetTexture("texture0", texture); // Fallback for compatibility
	}

	ECS::GetInstance().AddComponent(entity, Material(std::move(cubeMaterial)));

	// Create second cube  
	auto entity2 = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent(entity2, Transform(Vec3(-1, 1, -2), Vec3(0, 0, 0), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent(entity2, ObjectMetaData());
	ECS::GetInstance().AddComponent(entity2, graphics::GeometryFactory::CreateCube(1, 1, 1));

	// Create a different material for variety
	auto cube2Material = std::make_unique<graphics::Material>(shader);
	cube2Material->LoadTemplate(graphics::MaterialTemplates::PBR_METAL());

	if (texture && texture->IsValid()) {
		cube2Material->SetTexture("materialAlbedoMap", texture);
		cube2Material->SetTexture("texture0", texture);
	}

	ECS::GetInstance().AddComponent(entity2, Material(std::move(cube2Material)));
	ECS::GetInstance().AddComponent(entity2, Script("Sandbox", entity2));

	// Create lights with balanced intensities
	auto mainLightEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent(mainLightEntity, Transform(Vec3(0, 4, 2), Vec3(0, 0, 0), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent(mainLightEntity, ObjectMetaData("MainLight", "Light", true));
	ECS::GetInstance().AddComponent(mainLightEntity, Light(Vec3(1, 1, 1), 0.8f, LightType::POINT));

	// Light sphere material
	auto lightMaterial = std::make_unique<graphics::Material>(shader);
	lightMaterial->LoadTemplate(graphics::MaterialTemplates::EMISSIVE(Vec3(1.0f, 1.0f, 1.0f), 2.0f));
	ECS::GetInstance().AddComponent(mainLightEntity, graphics::GeometryFactory::CreateSphere(0.1f));
	ECS::GetInstance().AddComponent(mainLightEntity, Material(std::move(lightMaterial)));

	// Red accent light
	auto redLightEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent(redLightEntity, Transform(Vec3(3, 2, 0), Vec3(0, 0, 0), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent(redLightEntity, ObjectMetaData("LightRed", "Light", true));
	ECS::GetInstance().AddComponent(redLightEntity, Light(Vec3(1, 0.0, 0.0), 1.0f, LightType::POINT));

	auto redLightMaterial = std::make_unique<graphics::Material>(shader);
	redLightMaterial->LoadTemplate(graphics::MaterialTemplates::EMISSIVE(Vec3(1.0f, 0.f, 0.f), 1.0f));
	ECS::GetInstance().AddComponent(redLightEntity, graphics::GeometryFactory::CreateSphere(0.1f));
	ECS::GetInstance().AddComponent(redLightEntity, Material(std::move(redLightMaterial)));

	// Blue accent light
	auto blueLightEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent(blueLightEntity, Transform(Vec3(-3, 2, 0), Vec3(0, 0, 0), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent(blueLightEntity, ObjectMetaData("LightBlue", "Light", true));
	ECS::GetInstance().AddComponent(blueLightEntity, Light(Vec3(0.0, 0.0, 1), 1.0f, LightType::POINT));

	auto blueLightMaterial = std::make_unique<graphics::Material>(shader);
	blueLightMaterial->LoadTemplate(graphics::MaterialTemplates::EMISSIVE(Vec3(0.f, 0.f, 1.0f), 1.0f));
	ECS::GetInstance().AddComponent(blueLightEntity, graphics::GeometryFactory::CreateSphere(0.1f));
	ECS::GetInstance().AddComponent(blueLightEntity, Material(std::move(blueLightMaterial)));

	// Green accent light
	auto greenLightEntity = ECS::GetInstance().CreateEntity();
	ECS::GetInstance().AddComponent(greenLightEntity, Transform(Vec3(0, 2, -3), Vec3(0, 0, 0), Vec3(1, 1, 1)));
	ECS::GetInstance().AddComponent(greenLightEntity, ObjectMetaData("LightGreen", "Light", true));
	ECS::GetInstance().AddComponent(greenLightEntity, Light(Vec3(0.0, 1.0f, 0.0), 1.0f, LightType::POINT));

	auto greenLightMaterial = std::make_unique<graphics::Material>(shader);
	greenLightMaterial->LoadTemplate(graphics::MaterialTemplates::EMISSIVE(Vec3(0.f, 1.f, 0.0f), 1.0f));
	ECS::GetInstance().AddComponent(greenLightEntity, graphics::GeometryFactory::CreateSphere(0.1f));
	ECS::GetInstance().AddComponent(greenLightEntity, Material(std::move(greenLightMaterial)));


	EE_CORE_INFO("Total living entities after creation: {0}", ECS::GetInstance().GetLivingEntityCount());



	glClearColor(0.2f,0.3f,0.3f,1.0f); // Background color

   // Create ImGUI window for Asset Browser
   editor::EditorGUI::CreateImGUIWindow<ImguiUI::AssetBrowser>();
   editor::EditorGUI::CreateImGUIWindow<ParticlesImGUI>(emitter.get());
   editor::EditorGUI::CreateImGUIWindow<AudioImGUI>();
   
   EE_CORE_INFO("Systems and components registered successfully, Engine Initialized");
   s_isInitialized = true;
   return true;
}

void engine::Shutdown()
{
	// By right, ECS helps shut all systems down via each system's destructor, while the rest of the singleton classes will be destroyed by the OS
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

	emitter.reset();

    graphics::GPUProfiler::Shutdown();

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

	// 1. Update input states
	Input::Update();

	glfwPollEvents(); // Do not move me above Input::Update - Friendly Adviser

	// Update FrameController
	FrameController::BeginFrame();

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

	// Start GPU timing for rendering
	graphics::GPUProfiler::BeginEvent("Frame Rendering");

	ECS::GetInstance().GetSystem<graphics::Renderer>()->Clear();

	Mtx44 view = editor::EditorCamera::GetInstance().GetViewMatrix();
	Mtx44 proj = editor::EditorCamera::GetInstance().GetProjectionMatrix();

	// Draw
	ECS::GetInstance().GetSystem<graphics::Renderer>()->Update(view, proj);

	graphics::GPUProfiler::EndEvent();

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

	bool key1IsPressed = glfwGetKey(windowContext, GLFW_KEY_1) == GLFW_PRESS;
	bool key2IsPressed = glfwGetKey(windowContext, GLFW_KEY_2) == GLFW_PRESS;
	bool key3IsPressed = glfwGetKey(windowContext, GLFW_KEY_3) == GLFW_PRESS;

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

	key1WasPressed = key1IsPressed;
	key2WasPressed = key2IsPressed;
	key3WasPressed = key3IsPressed;
}
