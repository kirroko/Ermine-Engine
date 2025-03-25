/* Start Header ************************************************************************/
/*!
\file       Engine.cpp
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This file contains the declaration of the Engine system.
            Expose key engine functions to be called from the editor's main loop
            Helps managed subsystems of the engine
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "Engine.h"

#include "AssetManager.h"
#include "ECS.h"
#include "Components.h"
#include "EditorCamera.h"
#include "FrameController.h"
#include "GeometryFactory.h"
#include "Input.h"
#include "Logger.h"
#include "Renderer.h"
#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/glm.hpp"
#include "Utility/JobSystem.h"

using namespace Ermine;

namespace Ermine::Engine
{
    static bool s_isInitialized = false;
    static std::unique_ptr<editor::EditorCamera> s_EditorCamera = nullptr;
    
    void EnableMemoryLeakChecking(int breakAlloc = -1)
    {
        int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag(tmpDbgFlag);
        if (breakAlloc != -1)
            _CrtSetBreakAlloc(breakAlloc);
    }
}

bool Engine::Init(GLFWwindow* windowContext)
{
    if (s_isInitialized) // Already initialized
        return true;

    EnableMemoryLeakChecking();

    Input::Init(windowContext);

    FrameController::Init(120.f, 60.f);

    job::Initialize();
    
    ECS::GetInstance().Init();
    EE_CORE_INFO("ECS Initialized");
    EE_CORE_TRACE("Begin Registering of Components and Systems...");
    
    // TODO: Register all components here, limit of 32 components
    ECS::GetInstance().RegisterComponent<Transform>();
    ECS::GetInstance().RegisterComponent<Rigidbody3D>();
    ECS::GetInstance().RegisterComponent<Mesh>();
    ECS::GetInstance().RegisterComponent<Material>();
    
    // TODO: Register all systems here, no limits
    // ReSharper disable once CppExpressionWithoutSideEffects
    ECS::GetInstance().RegisterSystem<graphics::Renderer>();
    
    // TODO: Set the signature for the system as required
    // For Graphics/Renderer system
    SignatureID sig;
    sig.set(ECS::GetInstance().GetComponentType<Transform>());
    sig.set(ECS::GetInstance().GetComponentType<Mesh>());
    sig.set(ECS::GetInstance().GetComponentType<Material>());
    ECS::GetInstance().SetSystemSignature<graphics::Renderer>(sig);

    glfwSetFramebufferSizeCallback(windowContext, []([[maybe_unused]] GLFWwindow* window, int width, int height)
    {
        s_EditorCamera->SetViewportSize(static_cast<float>(width),static_cast<float>(height));
        glViewport(0,0,width,height); 
    });

    // Create graphics resources
    auto shader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/vertex.glsl", "../Resources/Shaders/fragment.glsl");
    auto texture = AssetManager::GetInstance().LoadTexture("../Resources/Textures/greybox_grey_grid.png");
    auto entity = ECS::GetInstance().CreateEntity();
    ECS::GetInstance().AddComponent(entity, Transform(Vec3(0,0,-1),Vec3(0,45,90),Vec3(1,1,1)));
    ECS::GetInstance().AddComponent(entity, graphics::GeometryFactory::CreateCube(1,1,1));
    ECS::GetInstance().AddComponent(entity, Material(shader, texture));

    auto entity2 = ECS::GetInstance().CreateEntity();
    ECS::GetInstance().AddComponent(entity2, Transform(Vec3(-1,1,-2),Vec3(0,0,0),Vec3(1,1,1)));
    ECS::GetInstance().AddComponent(entity2, graphics::GeometryFactory::CreateCube(1,1,1));
    ECS::GetInstance().AddComponent(entity2, Material(shader, texture));
    
    auto entity3 = ECS::GetInstance().CreateEntity();
    ECS::GetInstance().AddComponent(entity3, Transform(Vec3(1,1,-3),Vec3(0,0,0),Vec3(1,1,1)));
    ECS::GetInstance().AddComponent(entity3, graphics::GeometryFactory::CreateSphere());
    ECS::GetInstance().AddComponent(entity3, Material(shader, texture));

    s_EditorCamera = std::make_unique<editor::EditorCamera>(45.0f);
    
    glClearColor(0.2f,0.3f,0.3f,1.0f); // Background color
    
    EE_CORE_INFO("Systems and components registered successfully, Engine Initialized");
    s_isInitialized = true;
    return true;
}

void Engine::Shutdown()
{
    // By right, ECS should shut all its systems down via each systems destructor
    if (!s_isInitialized)
        return;

    s_EditorCamera.reset();

    job::Shutdown();
    
    s_isInitialized = false;
}

void Engine::Update([[maybe_unused]] GLFWwindow* windowContext)
{
    if (!s_isInitialized)
        return;
    
    // Update input states
    Input::Update();
    
    glfwPollEvents();
    
    // Update FrameController
    FrameController::BeginFrame();

    while (FrameController::ShouldUpdateFixed())
    {
        // Fixed update logic here
    }

    // Other non-fixed logic here
    if (Input::IsKeyPressed(GLFW_KEY_1)) // Kick a job
    {
        job::Declaration job;
        job.m_pEntry = [](uintptr_t param)
        {
            int jobId = static_cast<int>(param);
            EE_CORE_INFO("Job {0} executing on thread {1}", jobId, std::hash<std::thread::id>()(std::this_thread::get_id()));

            // Simulate some work
            std::this_thread::sleep_for(std::chrono::milliseconds(10 + jobId % 50));
        };
        job.m_param = static_cast<uintptr_t>(1);
        job.m_priority = job::Priority::NORMAL;

        auto startTime = std::chrono::high_resolution_clock::now();
        job::KickJobAndWait(job);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        EE_CORE_INFO("a job completed in {0}ms", duration);
    }
    
    // Update editor camera
    if (s_EditorCamera)
    {
        s_EditorCamera->Update(FrameController::GetDeltaTime());
    }
}

void Engine::Render(GLFWwindow* window)
{
    if (!s_isInitialized)
        return;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0,0,width,height);

    ECS::GetInstance().GetSystem<graphics::Renderer>()->Clear();

    Mtx44 view = s_EditorCamera->GetViewMatrix();
    Mtx44 proj = s_EditorCamera->GetProjectionMatrix();
    
    // Draw
    ECS::GetInstance().GetSystem<graphics::Renderer>()->Update(view, proj);

    glfwSwapBuffers(window);
}

// Free to use for testing purposes
void Engine::Dummy([[maybe_unused]] GLFWwindow* wwindow)
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
