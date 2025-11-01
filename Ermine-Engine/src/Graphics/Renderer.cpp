/* Start Header ************************************************************************/
/*!
\file       Renderer.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\co-author  Jeremy Lim Ting Jie, jeremytingjie.lim, 2301370, jeremytingjie.lim\@digipen.edu
\co-author  Ridhwan Afandi, moahamedridhwan.b, 2301367, moahamedridhwan.b\@digipen.edu
\co-author  Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       09/27/2025
\brief      This file contains the definition of the Renderer system.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "Renderer.h"
#include "Material.h"

#include <numeric> // For std::iota

#include "ECS.h"
#include "Logger.h"
#include "MathUtils.h"
#include "Matrix3x3.h"
#include "Components.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/glm.hpp"
#include "Input.h"
#include "GeometryFactory.h"
#include "AssetManager.h"
#include "Skybox.h"
#include <random>  
#include "Physics.h"
#include "AnimationManager.h"

#include <GLFW/glfw3.h>

using namespace Ermine::graphics;

unsigned int SHADOW_MAX_LAYERS = SHADOW_MAX_LAYERS_DESIRED;

/**
 * @brief Checks for OpenGL errors and logs them.
 * @param file Source file name.
 * @param line Line number.
 * @return GLenum The last OpenGL error code.
 */
GLenum glCheckError_(const char* file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM";  break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		EE_CORE_ERROR("{0} | {1} ({2})", error, file, line);
		assert(false && "Check logs");
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void Renderer::InitializeShadowMapResources()
{
	if (m_ShadowMapArrayHandle != 0) {
		glMakeTextureHandleNonResidentARB(m_ShadowMapArrayHandle);
		m_ShadowMapArrayHandle = 0;
	}

	if (m_ShadowMapArray != 0) {
		glDeleteTextures(1, &m_ShadowMapArray);
		m_ShadowMapArray = 0;
	}

	if (m_ShadowMapFBO != 0) {
		glDeleteFramebuffers(1, &m_ShadowMapFBO);
		m_ShadowMapFBO = 0;
	}

	// Clear any OpenGL errors from cleanup
	while (glGetError() != GL_NO_ERROR);

	InitializeShadowMap();
	CreateShadowMapArray();


	EE_CORE_INFO("Shadow map initialized: FBO={0}, Texture={1}, Layers={2}",
		m_ShadowMapFBO, m_ShadowMapArray, SHADOW_MAX_LAYERS);
}

/**
 * @brief Initializes the renderer and its resources.
 * @param screenWidth Width of the screen.
 * @param screenHeight Height of the screen.
 */
void Renderer::Init(const int& screenWidth, const int& screenHeight)
{
	// Check for ARB_bindless_texture support
	if (!glfwExtensionSupported("GL_ARB_bindless_texture") || !GL_ARB_bindless_texture)
	{
		EE_CORE_WARN("GL_ARB_bindless_texture not supported. Deferred rendering will be disabled.");
		m_UseDeferredRendering = false;
	}
	// Create a fullscreen quad for rendering the offscreen buffer to the screen

	// Initialize MeshManager for centralized mesh storage and indirect rendering
	m_MeshManager.Initialize();

	// Connect AnimationManager to SkeletalSSBO for efficient bone transform updates
	auto animationManager = Ermine::ECS::GetInstance().GetSystem<AnimationManager>();
	if (animationManager)
	{
		animationManager->SetSkeletalSSBO(&m_MeshManager.m_SkeletalSSBO);
	}

	// Add light system reference
	m_LightSystem = Ermine::ECS::GetInstance().GetSystem<LightSystem>();

	m_QuadMesh = GeometryFactory::CreateQuad(2.0f, 2.0f);

	// Load deferred shading shaders
	m_ShadowMapInstancedShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/shadowmap_instanced_vertex.glsl", "../Resources/Shaders/shadowmap_fragment.glsl");
	m_GBufferShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/gBuffer_vertex.glsl", "../Resources/Shaders/gBuffer_fragment.glsl");
	m_LightPassShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/lighting_vertex.glsl", "../Resources/Shaders/lighting_fragment.glsl");
	m_BloomShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/bloom_vertex.glsl", "../Resources/Shaders/bloom_fragment.glsl");
	m_PostProcessShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/postprocess_vertex.glsl", "../Resources/Shaders/postprocess_fragment.glsl");
	m_AAShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/FXAA_vertex.glsl", "../Resources/Shaders/FXAA_fragment.glsl");
	// Load forward rendering shader for transparent objects
	m_ForwardShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/vertex.glsl", "../Resources/Shaders/fragment_enhanced.glsl");
	if (!m_ForwardShader || !m_ForwardShader->IsValid()) {
		EE_CORE_WARN("Forward shader for transparency not loaded, using enhanced fragment shader from assets");
		// Fallback to the existing enhanced fragment shader
		m_ForwardShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/vertex.glsl", "../Resources/Shaders/fragment_enhanced.glsl");
	}
	// Create initial g-buffer
	CreateGBuffer(screenWidth, screenHeight);
	CreatePostProcessBuffer(screenWidth, screenHeight);

	// Create shadow map FBO and texture
	InitializeShadowMapResources();

	// Create pre-skinned positions SSBO for geometry pass to write and shadow pass to read
	// Geometry pass writes skinned positions here, shadow pass reuses them (eliminates redundant bone calculations)
	glGenBuffers(1, &m_PreSkinnedPositionsSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_PreSkinnedPositionsSSBO);
	// Allocate large enough buffer for all vertices (will resize if needed)
	glBufferData(GL_SHADER_STORAGE_BUFFER, 50000 * sizeof(glm::vec4), nullptr, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, m_PreSkinnedPositionsSSBO); // Binding 8
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	m_PreSkinnedBufferSize = 50000 * sizeof(glm::vec4);

	// Setup shadow VAOs that include pre-skinned position attribute (location 6)
	// These VAOs allow hardware vertex fetching instead of SSBO random access for better performance
	m_MeshManager.SetupShadowVAOs(m_PreSkinnedPositionsSSBO);

	m_PickingShader = AssetManager::GetInstance().LoadShader(
		"../Resources/Shaders/picking_vertex.glsl",
		"../Resources/Shaders/picking_fragment_uint.glsl"
	);

	CreatePickingBuffer(screenWidth, screenHeight);

	m_MaterialsDirty = true;
}

void Renderer::SubmitDebugLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color)
{
	m_DebugLines.push_back({ from, color });
	m_DebugLines.push_back({ to, color });
}

void Renderer::RenderDebugLines(const glm::mat4& view, const glm::mat4& proj)
{
	if (m_DebugLines.empty()) return;

	// 1) Create VAO/VBO once
	if (m_DebugVAO == 0) {
		glGenVertexArrays(1, &m_DebugVAO);
		glGenBuffers(1, &m_DebugVBO);

		glBindVertexArray(m_DebugVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_DebugVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(DebugVertex) * 65536, nullptr, GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(0); // position
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex),
			(void*)offsetof(DebugVertex, position));
		glEnableVertexAttribArray(1); // color
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex),
			(void*)offsetof(DebugVertex, color));
		glBindVertexArray(0);
	}

	// 2) Load shader once (make sure it�s valid)
	if (!debugShader) {
		debugShader = AssetManager::GetInstance().LoadShader(
			"../Resources/Shaders/debug_line_vert.glsl",
			"../Resources/Shaders/debug_line_frag.glsl"
		);
		if (!debugShader || !debugShader->IsValid()) {
			m_DebugLines.clear();
			return; // avoid Bind() on null
		}
	}

	// 3) Bind VAO/VBO and upload THIS FRAME�S data
	glBindVertexArray(m_DebugVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_DebugVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0,
		(GLsizeiptr)(m_DebugLines.size() * sizeof(DebugVertex)),
		m_DebugLines.data());

	// 4) Shader + uniforms
	debugShader->Bind();
	debugShader->SetUniformMatrix4fv("uView", view);
	debugShader->SetUniformMatrix4fv("uProj", proj);

	// 5) States (depth to taste)
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_CULL_FACE);
	glLineWidth(2.0f);

	// 6) Draw and cleanup
	glDrawArrays(GL_LINES, 0, (GLsizei)m_DebugLines.size());
	glBindVertexArray(0);
	m_DebugLines.clear();
}

/**
 * @brief Create an offscreen buffer for viewport/scene rendering
 * @param width The width of the offscreen buffer
 * @param height The height of the offscreen buffer
 * @return OffscreenBuffer The offscreen buffer
 */
Renderer::OffscreenBuffer Renderer::CreateOffscreenBuffer(const int& width, const int& height)
{
	OffscreenBuffer buffer{};

	// If an offscreen buffer already exists, delete its OpenGL resources before creating a new one.
	if (m_OffscreenBuffer)
	{
		glDeleteFramebuffers(1, &m_OffscreenBuffer->FBO);
		glDeleteTextures(1, &m_OffscreenBuffer->ColorTexture);
		glDeleteRenderbuffers(1, &m_OffscreenBuffer->RBO);
	}

	if (!m_LightsUBO)
	{
		glGenBuffers(1, &m_LightsUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, m_LightsUBO);
		const GLsizeiptr headerSize = static_cast<GLsizeiptr>(sizeof(glm::vec4));
		const GLsizeiptr bodySize = static_cast<GLsizeiptr>(MAX_LIGHTS * sizeof(LightGPU));
		glBufferData(GL_UNIFORM_BUFFER, headerSize + bodySize, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, LightsBindingPoint, m_LightsUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glCheckError();
	}

	// Create FBO
	glGenFramebuffers(1, &buffer.FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, buffer.FBO);

	// Check for errors
	glCheckError();

	// Create color texture
	glGenTextures(1, &buffer.ColorTexture);
	glBindTexture(GL_TEXTURE_2D, buffer.ColorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer.ColorTexture, 0);

	// Check for errors
	glCheckError();

	// Create RBO for depth & stencil attachment
	glGenRenderbuffers(1, &buffer.RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, buffer.RBO);

	// Making sure dimensions are non-zero
	if (width <= 0 || height <= 0)
	{
		EE_CORE_ERROR("ERROR: Invalid framebuffer dimensions: {0}x{1}", width, height);
	}
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, buffer.RBO);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, buffer.RBO);

	// Check for errors
	glCheckError();

	// Explicitly specify draw buffer
	GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	// Check overall framebuffer completeness
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		switch (status)
		{
		case GL_FRAMEBUFFER_UNDEFINED:
			EE_CORE_ERROR("ERROR: Framebuffer is undefined!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			EE_CORE_ERROR("ERROR: Framebuffer incomplete attachment!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			EE_CORE_ERROR("ERROR: Framebuffer missing attachment!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			EE_CORE_ERROR("ERROR: Framebuffer incomplete draw buffer!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			EE_CORE_ERROR("ERROR: Framebuffer incomplete read buffer!");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			EE_CORE_ERROR("ERROR: Framebuffer unsupported!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			EE_CORE_ERROR("ERROR: Framebuffer incomplete multisample!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			EE_CORE_ERROR("ERROR: Framebuffer incomplete layer targets!");
			break;
		default:
			EE_CORE_ERROR("ERROR: Framebuffer unknown error!");
			break;
		}
		EE_CORE_FATAL("Framebuffer Failed!!!");
		assert(false && "Check logs");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	buffer.width = width;
	buffer.height = height;
	m_OffscreenBuffer = std::make_shared<OffscreenBuffer>(buffer);
	return buffer;
}

/**
 * @brief Resize the offscreen buffer to new dimensions without recreating the FBO
 * @param width New width
 * @param height New height
 */
void Renderer::ResizeOffscreenBuffer(const int& width, const int& height)
{
	if (!m_OffscreenBuffer)
	{
		CreateOffscreenBuffer(width, height);
		return;
	}

	if (m_OffscreenBuffer->width == width && m_OffscreenBuffer->height == height)
		return;

	// Resize color texture
	glBindTexture(GL_TEXTURE_2D, m_OffscreenBuffer->ColorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Resize depth-stencil renderbuffer
	glBindRenderbuffer(GL_RENDERBUFFER, m_OffscreenBuffer->RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

	// Validate framebuffer completeness after resize
	glBindFramebuffer(GL_FRAMEBUFFER, m_OffscreenBuffer->FBO);
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		EE_CORE_ERROR("ERROR: Offscreen framebuffer not complete after resize! Status: {0}", status);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_OffscreenBuffer->width = width;
	m_OffscreenBuffer->height = height;

	glCheckError();

	ResizePickingBuffer(width, height);
}

/**
 * @brief Create optimized g-buffer for deferred rendering using scalar materials and emissive
 * RT0: RGBA16F (48 bits) - Albedo RGB
 * RT1: RGB16F (48 bits) - Normals XYZ
 * RT2: RGBA8 (32 bits) - Emissive RGB + Intensity
 * RT3: RGBA8 (32 bits) - Material properties (R: Roughness, G: Metallic, B: AO, A: Unused)
 * Total: 160 bits per pixel
 */
void Renderer::CreateGBuffer(const int& width, const int& height)
{
	// Clean up existing g-buffer if it exists
	CleanupGBuffer();

	GBuffer gBuffer{};
	gBuffer.width = width;
	gBuffer.height = height;

	// Validate dimensions
	if (width <= 0 || height <= 0)
	{
		EE_CORE_ERROR("ERROR: Invalid G-Buffer dimensions: {0}x{1}", width, height);
	}

	// If Light UBO doesn't exist, create it
	if (!m_LightsUBO)
	{
		glGenBuffers(1, &m_LightsUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, m_LightsUBO);
		const GLsizeiptr headerSize = static_cast<GLsizeiptr>(sizeof(glm::vec4));
		const GLsizeiptr bodySize = static_cast<GLsizeiptr>(MAX_LIGHTS * sizeof(LightGPU));
		glBufferData(GL_UNIFORM_BUFFER, headerSize + bodySize, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, LightsBindingPoint, m_LightsUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glCheckError();
	}

	// Create framebuffer
	glGenFramebuffers(1, &gBuffer.FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.FBO);

	// Create RT0 Texture: RGBA16F (48 bits) - Albedo RGB
	glGenTextures(1, &gBuffer.PackedTexture0);
	glBindTexture(GL_TEXTURE_2D, gBuffer.PackedTexture0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_HALF_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBuffer.PackedTexture0, 0);

	// Create RT1 Texture: RGB16F (48 bits) - Normals XYZ
	glGenTextures(1, &gBuffer.PackedTexture1);
	glBindTexture(GL_TEXTURE_2D, gBuffer.PackedTexture1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_HALF_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gBuffer.PackedTexture1, 0);

	// Create RT2 Texture: RGBA8 (32 bits) - Emissive RGB + Intensity
	glGenTextures(1, &gBuffer.PackedTexture2);
	glBindTexture(GL_TEXTURE_2D, gBuffer.PackedTexture2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gBuffer.PackedTexture2, 0);

	// Create RT3 Texture: RGBA8 (32 bits) - Material properties (R: Roughness, G: Metallic, B: AO, A: Unused)
	glGenTextures(1, &gBuffer.PackedTexture3);
	glBindTexture(GL_TEXTURE_2D, gBuffer.PackedTexture3);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gBuffer.PackedTexture3, 0);

	// Create depth texture for depth testing and reconstruction. 24 bits
	glGenTextures(1, &gBuffer.DepthTexture);
	glBindTexture(GL_TEXTURE_2D, gBuffer.DepthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gBuffer.DepthTexture, 0);

	// Set up MRTs - all 4 color attachments
	GLenum drawBuffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, drawBuffers);

	// Check framebuffer completeness
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		EE_CORE_ERROR("ERROR: G-Buffer framebuffer not complete! Status: {0}", status);
		CleanupGBuffer();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glCheckError();

	// Get the texture handles for the deferred lighting textures created above
	gBuffer.HandlePackedTexture0 = glGetTextureHandleARB(gBuffer.PackedTexture0);
	glMakeTextureHandleResidentARB(gBuffer.HandlePackedTexture0);

	gBuffer.HandlePackedTexture1 = glGetTextureHandleARB(gBuffer.PackedTexture1);
	glMakeTextureHandleResidentARB(gBuffer.HandlePackedTexture1);

	gBuffer.HandlePackedTexture2 = glGetTextureHandleARB(gBuffer.PackedTexture2);
	glMakeTextureHandleResidentARB(gBuffer.HandlePackedTexture2);

	gBuffer.HandlePackedTexture3 = glGetTextureHandleARB(gBuffer.PackedTexture3);
	glMakeTextureHandleResidentARB(gBuffer.HandlePackedTexture3);

	gBuffer.HandleDepthTexture = glGetTextureHandleARB(gBuffer.DepthTexture);
	glMakeTextureHandleResidentARB(gBuffer.HandleDepthTexture);

	m_GBuffer = std::make_shared<GBuffer>(gBuffer);
	EE_CORE_INFO("Created G-Buffer: {0}x{1}, 176 bits per pixel", width, height);
}

/**
 * @brief Create an offscreen buffer for viewport/scene rendering
 * @param width The width of the offscreen buffer
 * @param height The height of the offscreen buffer
 * @return OffscreenBuffer The offscreen buffer
 */
void Renderer::CreatePostProcessBuffer(const int& width, const int& height)
{
	PostProcessBuffer pPBuffer, bEBuffer, bBBuffer1, bBBuffer2, AABuffer;


	// If an  buffer already exists, delete its OpenGL resources before creating a new one.
	if (m_PostProcessBuffer)
	{
		glDeleteFramebuffers(1, &m_PostProcessBuffer->FBO);
		glDeleteTextures(1, &m_PostProcessBuffer->ColorTexture);
		if (m_PostProcessBuffer->DepthTexture != 0) {
			glDeleteTextures(1, &m_PostProcessBuffer->DepthTexture);
		}
		glDeleteFramebuffers(1, &m_BloomExtractBuffer->FBO);
		glDeleteTextures(1, &m_BloomExtractBuffer->ColorTexture);
		glDeleteFramebuffers(1, &m_BloomBlurBuffer1->FBO);
		glDeleteTextures(1, &m_BloomBlurBuffer1->ColorTexture);
		glDeleteFramebuffers(1, &m_BloomBlurBuffer2->FBO);
		glDeleteTextures(1, &m_BloomBlurBuffer2->ColorTexture);
	}

	// Create main post-process buffer with depth attachment for skybox rendering
	glGenFramebuffers(1, &pPBuffer.FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, pPBuffer.FBO);

	// Color texture
	glGenTextures(1, &pPBuffer.ColorTexture);
	glBindTexture(GL_TEXTURE_2D, pPBuffer.ColorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_HALF_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pPBuffer.ColorTexture, 0);

	// Depth texture for skybox rendering
	glGenTextures(1, &pPBuffer.DepthTexture);
	glBindTexture(GL_TEXTURE_2D, pPBuffer.DepthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pPBuffer.DepthTexture, 0);

	glCheckError();

	// Create other buffers without depth (they don't need it)
	glGenFramebuffers(1, &bEBuffer.FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, bEBuffer.FBO);
	glGenTextures(1, &bEBuffer.ColorTexture);
	glBindTexture(GL_TEXTURE_2D, bEBuffer.ColorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_HALF_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bEBuffer.ColorTexture, 0);
	bEBuffer.DepthTexture = 0; // No depth for bloom buffers
	glCheckError();

	glGenFramebuffers(1, &bBBuffer1.FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, bBBuffer1.FBO);
	glGenTextures(1, &bBBuffer1.ColorTexture);
	glBindTexture(GL_TEXTURE_2D, bBBuffer1.ColorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_HALF_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bBBuffer1.ColorTexture, 0);
	bBBuffer1.DepthTexture = 0;
	glCheckError();

	glGenFramebuffers(1, &bBBuffer2.FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, bBBuffer2.FBO);
	glGenTextures(1, &bBBuffer2.ColorTexture);
	glBindTexture(GL_TEXTURE_2D, bBBuffer2.ColorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_HALF_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bBBuffer2.ColorTexture, 0);
	bBBuffer2.DepthTexture = 0;
	glCheckError();

	glGenFramebuffers(1, &AABuffer.FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, AABuffer.FBO);
	glGenTextures(1, &AABuffer.ColorTexture);
	glBindTexture(GL_TEXTURE_2D, AABuffer.ColorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_HALF_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, AABuffer.ColorTexture, 0);
	AABuffer.DepthTexture = 0;
	glCheckError();

	// Making sure dimensions are non-zero
	if (width <= 0 || height <= 0)
	{
		EE_CORE_ERROR("ERROR: Invalid framebuffer dimensions: {0}x{1}", width, height);
	}

	// Explicitly specify draw buffer
	GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	// Check overall framebuffer completeness
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		switch (status)
		{
		case GL_FRAMEBUFFER_UNDEFINED:
			EE_CORE_ERROR("ERROR: Framebuffer is undefined!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			EE_CORE_ERROR("ERROR: Framebuffer incomplete attachment!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			EE_CORE_ERROR("ERROR: Framebuffer missing attachment!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			EE_CORE_ERROR("ERROR: Framebuffer incomplete draw buffer!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			EE_CORE_ERROR("ERROR: Framebuffer incomplete read buffer!");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			EE_CORE_ERROR("ERROR: Framebuffer unsupported!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			EE_CORE_ERROR("ERROR: Framebuffer incomplete multisample!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			EE_CORE_ERROR("ERROR: Framebuffer incomplete layer targets!");
			break;
		default:
			EE_CORE_ERROR("ERROR: Framebuffer unknown error!");
			break;
		}
		EE_CORE_FATAL("Framebuffer Failed!!!");
		assert(false && "Check logs");
	}

	pPBuffer.width = width;
	pPBuffer.height = height;
	m_PostProcessBuffer = std::make_shared<PostProcessBuffer>(pPBuffer);
	bEBuffer.width = width;
	bEBuffer.height = height;
	m_BloomExtractBuffer = std::make_shared<PostProcessBuffer>(bEBuffer);
	bBBuffer1.width = width;
	bBBuffer1.height = height;
	m_BloomBlurBuffer1 = std::make_shared<PostProcessBuffer>(bBBuffer1);
	bBBuffer2.width = width;
	bBBuffer2.height = height;
	m_BloomBlurBuffer2 = std::make_shared<PostProcessBuffer>(bBBuffer2);
	AABuffer.width = width;
	AABuffer.height = height;
	m_AntiAliasingBuffer = std::make_shared<PostProcessBuffer>(AABuffer);
}

/**
 * @brief Creates the G-buffer for deferred rendering.
 * @param width Buffer width.
 * @param height Buffer height.
 */
void Renderer::ResizeGBuffer(const int& width, const int& height)
{
	if (m_GBuffer && m_GBuffer->width == width && m_GBuffer->height == height)
		return;

	CreateGBuffer(width, height);

	CreatePostProcessBuffer(width, height);

	ResizePickingBuffer(width, height);
}

/**
 * @brief Begin geometry pass for deferred rendering
 */
void Renderer::BeginGeometryPass()
{
	if (!m_GBuffer)
	{
		EE_CORE_ERROR("G-Buffer not initialized!");
		return;
	}

	// Bind g-buffer framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, m_GBuffer->FBO);
	glViewport(0, 0, m_GBuffer->width, m_GBuffer->height);

	// Clear g-buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearDepth(1.0f);


	// Set up depth testing for geometry pass
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Disable blending for geometry pass
	glDisable(GL_BLEND);
}

/**
 * @brief End geometry pass and prepare for lighting pass
 */
void Renderer::EndGeometryPass()
{
	// Insert fence to track when GPU finishes reading bone data
	// This allows AnimationManager to wait before overwriting data in the next frame
	m_MeshManager.m_SkeletalSSBO.InsertFence();

	// Check for errors
	glCheckError();
}

/**
 * @brief Begin lighting pass for deferred rendering - render to texture for post-processing
 */
void Renderer::BeginLightingPass()
{
	// Always render lighting pass to post-process buffer for sampling
	if (!m_PostProcessBuffer)
	{
		EE_CORE_ERROR("Post-process buffer not initialized!");
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, m_PostProcessBuffer->FBO);
	glViewport(0, 0, m_PostProcessBuffer->width, m_PostProcessBuffer->height);

	// Clear the lighting pass output
	glClear(GL_COLOR_BUFFER_BIT);

	// Set up for lighting calculations
	glDisable(GL_DEPTH_TEST); // No depth testing needed for full-screen pass
	glDisable(GL_BLEND);       // No blending needed for lighting output
}

/**
 * @brief End lighting pass and finalize frame
 */
void Renderer::EndLightingPass()
{
	glDisable(GL_BLEND);
	glCheckError();
}

/**
 * @brief Binds material textures to the geometry shader.
 * @param material Pointer to the material.
 */
void Renderer::BindMaterialTextures(Ermine::graphics::Material* material)
{
	if (!material) return;

	int texUnit = 0;
	if (material->HasParameter("materialAlbedoMap")) {
		std::shared_ptr<Texture> albedo = material->GetParameter("materialAlbedoMap")->texture;
		if (albedo && albedo->IsValid()) {
			albedo->Bind(texUnit);
			m_GBufferShader->SetUniform1i("materialAlbedoMap", texUnit);
		}
	}
	texUnit = 1;
	if (material->HasParameter("materialNormalMap")) {
		std::shared_ptr<Texture> normal = material->GetParameter("materialNormalMap")->texture;
		if (normal && normal->IsValid()) {
			normal->Bind(texUnit);
			m_GBufferShader->SetUniform1i("materialNormalMap", texUnit);
		}
	}
	texUnit = 2;
	if (material->HasParameter("materialRoughnessMap")) {
		std::shared_ptr<Texture> roughness = material->GetParameter("materialRoughnessMap")->texture;
		if (roughness && roughness->IsValid()) {
			roughness->Bind(texUnit);
			m_GBufferShader->SetUniform1i("materialRoughnessMap", texUnit);
		}
	}
	texUnit = 3;
	if (material->HasParameter("materialMetallicMap")) {
		std::shared_ptr<Texture> metallic = material->GetParameter("materialMetallicMap")->texture;
		if (metallic && metallic->IsValid()) {
			metallic->Bind(texUnit);
			m_GBufferShader->SetUniform1i("materialMetallicMap", texUnit);
		}
	}
	texUnit = 4;
	if (material->HasParameter("materialAoMap")) {
		std::shared_ptr<Texture> ao = material->GetParameter("materialAoMap")->texture;
		if (ao && ao->IsValid()) {
			ao->Bind(texUnit);
			m_GBufferShader->SetUniform1i("materialAoMap", texUnit);
		}
	}
	texUnit = 5;
	if (material->HasParameter("materialEmissiveMap")) {
		std::shared_ptr<Texture> emissive = material->GetParameter("materialEmissiveMap")->texture;
		if (emissive && emissive->IsValid()) {
			emissive->Bind(texUnit);
			m_GBufferShader->SetUniform1i("materialEmissiveMap", texUnit);
		}
	}
}

/**
 * @brief Renders the geometry pass, collecting transparent objects.
 * @param view View matrix.
 * @param projection Projection matrix.
 */
void Renderer::RenderGeometryPass(const Mtx44& view, const Mtx44& projection)
{
	if (!m_GBuffer || !m_GBufferShader) {
		EE_CORE_ERROR("G-Buffer or geometry shader not initialized!");
		return;
	}

	BeginGeometryPass();

	// Bind geometry shader that writes to g-buffer
	m_GBufferShader->Bind();
	BindMaterialBlockIfPresent(m_GBufferShader);

	// Recompile materials if dirty
	if (m_MaterialsDirty) {
		CompileMaterials();
	}

	// Clear transparent objects from previous frame
	m_transparentObjects.clear();

	// Calculate camera position for transparent sorting
	glm::mat4 glmView = glm::mat4(
		view.m00, view.m01, view.m02, view.m03,
		view.m10, view.m11, view.m12, view.m13,
		view.m20, view.m21, view.m22, view.m23,
		view.m30, view.m31, view.m32, view.m33
	);
	glm::mat4 invView = glm::inverse(glmView);
	Vec3 cameraPos = Vec3(invView[3][0], invView[3][1], invView[3][2]);

	// Set view and projection uniforms (model matrix comes from DrawInfo SSBO per draw)
	m_GBufferShader->SetUniformMatrix4fv("view", &view.m2[0][0]);
	m_GBufferShader->SetUniformMatrix4fv("projection", &projection.m2[0][0]);
	glGetError(); // Clear any existing errors
	// Render standard (non-skinned) meshes with StandardVAO
	if (!m_StandardDrawCommands.empty() && m_MeshManager.GetStandardVAO() != 0) {
		// Upload standard draw commands to GPU
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_MeshManager.m_DrawCommandsSSBO);
		size_t commandsBufferSize = m_StandardDrawCommands.size() * sizeof(DrawElementsIndirectCommand);
		glBufferData(GL_SHADER_STORAGE_BUFFER, commandsBufferSize, m_StandardDrawCommands.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		// Upload standard draw infos to persistent mapped buffer at offset 0
		m_MeshManager.m_PersistentDrawInfoBuffer.WriteDrawInfos(m_StandardDrawInfos, 0);

		// Set baseDrawID = 0 for standard batch
		m_GBufferShader->SetUniform1ui("baseDrawID", 0);

		// Bind StandardVAO and issue draw call
		glBindVertexArray(m_MeshManager.GetStandardVAO());
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_MeshManager.m_DrawCommandsSSBO);
		glMultiDrawElementsIndirect(
			GL_TRIANGLES,
			GL_UNSIGNED_INT,
			nullptr,
			static_cast<GLsizei>(m_StandardDrawCommands.size()),
			0
		);

		// Unbind
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
		glBindVertexArray(0);
	}

	// Render skinned (animated) meshes with SkinnedVAO
	if (!m_SkinnedDrawCommands.empty() && m_MeshManager.GetSkinnedVAO() != 0) {
		// Upload skinned draw commands to GPU
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_MeshManager.m_DrawCommandsSSBO);
		size_t skinnedCommandsBufferSize = m_SkinnedDrawCommands.size() * sizeof(DrawElementsIndirectCommand);
		glBufferData(GL_SHADER_STORAGE_BUFFER, skinnedCommandsBufferSize, m_SkinnedDrawCommands.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		// Upload skinned draw infos to persistent mapped buffer at offset = number of standard draws
		size_t drawInfoOffset = m_StandardDrawCommands.size();
		m_MeshManager.m_PersistentDrawInfoBuffer.WriteDrawInfos(m_SkinnedDrawInfos, drawInfoOffset);

		// Set baseDrawID = offset so gl_DrawID in shader accesses correct DrawInfo indices
		m_GBufferShader->SetUniform1ui("baseDrawID", static_cast<uint32_t>(drawInfoOffset));

		// Bind SkinnedVAO and issue draw call
		glBindVertexArray(m_MeshManager.GetSkinnedVAO());
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_MeshManager.m_DrawCommandsSSBO);
		glMultiDrawElementsIndirect(
			GL_TRIANGLES,
			GL_UNSIGNED_INT,
			nullptr,
			static_cast<GLsizei>(m_SkinnedDrawCommands.size()),
			0
		);

		// Unbind
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
		glBindVertexArray(0);
	}

	// Sort transparent objects by distance from camera
	SortTransparentObjects(cameraPos);

	EndGeometryPass();
}

/**
 * @brief Compiles draw commands and draw info for all passes (geometry/shadow and forward).
 * Routes opaque meshes to geometry pass, transparent/custom shader meshes to forward pass.
 */
void Renderer::CompileDrawData()
{
	const auto& ecs = Ermine::ECS::GetInstance();

	// Clear previous frame's commands for both passes
	m_StandardDrawCommands.clear();
	m_StandardDrawInfos.clear();
	m_SkinnedDrawCommands.clear();
	m_SkinnedDrawInfos.clear();
	m_ForwardPassDrawCommands.clear();
	m_ForwardPassDrawInfos.clear();

	// Reserve space for geometry pass (opaque)
	m_StandardDrawCommands.reserve(m_Entities.size());
	m_StandardDrawInfos.reserve(m_Entities.size());
	m_SkinnedDrawCommands.reserve(m_Entities.size() / 4);
	m_SkinnedDrawInfos.reserve(m_Entities.size() / 4);

	// Reserve space for forward pass (transparent/custom shader)
	m_ForwardPassDrawCommands.reserve(m_Entities.size() / 4);
	m_ForwardPassDrawInfos.reserve(m_Entities.size() / 4);

	// ========== STANDARD (NON-SKINNED) MESHES ==========
	for (auto& entity : m_Entities) {
		// Skip entities with AnimationComponent (handled in skinned mesh section)
		if (ecs.HasComponent<AnimationComponent>(entity)) continue;

		// Process entities with Model component
		if (ecs.HasComponent<ModelComponent>(entity)) {
			auto& modelComp = ecs.GetComponent<ModelComponent>(entity);
			auto& trans = ecs.GetComponent<Transform>(entity);

			if (!modelComp.m_model) continue;

			// Check if entity has material component for transparency/custom shader check
			Ermine::graphics::Material* material = nullptr;
			if (ecs.HasComponent<Ermine::Material>(entity)) {
				auto& materialComponent = ecs.GetComponent<Ermine::Material>(entity);
				material = materialComponent.GetMaterial();
			}

			// Build entity transform
			// glm::mat4 modelMatrix = glm::mat4(1.0f);
			// modelMatrix = glm::translate(modelMatrix, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
			// glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
			// rotQuat = glm::normalize(rotQuat);
			// modelMatrix *= glm::mat4_cast(rotQuat);
			// modelMatrix = glm::scale(modelMatrix, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

			glm::mat4 entityModel = GetEntityWorldMatrix(entity);

			//glm::mat4 entityModel = glm::mat4(1.0f);
			//entityModel = glm::translate(entityModel, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
			//glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
			//rotQuat = glm::normalize(rotQuat);
			//entityModel *= glm::mat4_cast(rotQuat);
			//entityModel = glm::scale(entityModel, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

			// Determine which pass this entity belongs to
			bool isTransparent = material && IsTransparentMaterial(material);
			// TODO: Uncomment when use of custom shaders is supported
			//bool isCustomShader = HasCustomShader(material);
			bool isCustomShader = false;

			// Get material index
			uint32_t materialIndex = 0;
			auto it = m_EntityMaterialIndices.find(entity);
			if (it != m_EntityMaterialIndices.end()) {
				materialIndex = it->second;
			}

			// Process each mesh in the model
			for (const auto& mesh : modelComp.m_model->GetMeshes()) {
				// Get mesh handle from MeshManager
				MeshHandle meshHandle = m_MeshManager.GetMeshHandle(mesh.meshID);
				if (!meshHandle.isValid()) continue;

				const MeshSubset* meshData = m_MeshManager.GetMeshData(meshHandle);
				if (!meshData) continue;

				// Build draw command
				DrawElementsIndirectCommand cmd;
				cmd.count = meshData->indexCount;
				cmd.instanceCount = 1;
				cmd.firstIndex = meshData->indexOffset;
				cmd.baseVertex = meshData->baseVertex;
				cmd.baseInstance = 0;

				// Build draw info with AABB and model matrix
				DrawInfo info;
				info.modelMatrix = entityModel;
				info.aabbMin = mesh.aabbMin;
				info.materialIndex = materialIndex;
				info.aabbMax = mesh.aabbMax;
				info.entityID = entity;
				info.flags = 0; // No skinning for standard meshes
				info.boneTransformOffset = 0;
				info._pad[0] = 0;
				info._pad[1] = 0;

				// Route to appropriate pass
				if (isTransparent || isCustomShader) {
					// Forward pass (transparent/custom shader)
					m_ForwardPassDrawCommands.push_back(cmd);
					m_ForwardPassDrawInfos.push_back(info);
				}
				else {
					// Geometry pass (opaque, standard shader)
					m_StandardDrawCommands.push_back(cmd);
					m_StandardDrawInfos.push_back(info);
				}
			}
		}
		// Process entities with Mesh component (primitives)
		if (ecs.HasComponent<Mesh>(entity) && ecs.HasComponent<Ermine::Material>(entity)) {
			auto& trans = ecs.GetComponent<Transform>(entity);
			auto& mesh = ecs.GetComponent<Mesh>(entity);
			auto& materialComponent = ecs.GetComponent<Ermine::Material>(entity);

			if (!mesh.vertex_array || !mesh.index_buffer) continue;

			// Skip if no registered mesh ID
			if (mesh.registeredMeshID.empty()) continue;

			Ermine::graphics::Material* material = materialComponent.GetMaterial();
			if (!material) {
				continue;
			}

			// Build model matrix
			// glm::mat4 modelMatrix = glm::mat4(1.0f);
			// modelMatrix = glm::translate(modelMatrix, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
			// glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
			// rotQuat = glm::normalize(rotQuat);
			// modelMatrix *= glm::mat4_cast(rotQuat);
			// modelMatrix = glm::scale(modelMatrix, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

			glm::mat4 model = GetEntityWorldMatrix(entity);

			//glm::mat4 model = glm::mat4(1.0f);
			//model = glm::translate(model, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
			//glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
			//rotQuat = glm::normalize(rotQuat);
			//model *= glm::mat4_cast(rotQuat);
			//model = glm::scale(model, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

			// Determine which pass this entity belongs to
			bool isTransparent = IsTransparentMaterial(material);
			// TODO: Uncomment when use of custom shaders is supported
			//bool isCustomShader = HasCustomShader(material);
			bool isCustomShader = false;

			// Get material index
			uint32_t materialIndex = 0;
			auto it = m_EntityMaterialIndices.find(entity);
			if (it != m_EntityMaterialIndices.end()) {
				materialIndex = it->second;
			}

			// Get mesh handle from MeshManager using the stored registered mesh ID
			MeshHandle meshHandle = m_MeshManager.GetMeshHandle(mesh.registeredMeshID);
			if (!meshHandle.isValid()) continue;

			const MeshSubset* meshData = m_MeshManager.GetMeshData(meshHandle);
			if (!meshData) continue;

			// Build draw command
			DrawElementsIndirectCommand cmd;
			cmd.count = meshData->indexCount;
			cmd.instanceCount = 1;
			cmd.firstIndex = meshData->indexOffset;
			cmd.baseVertex = meshData->baseVertex;
			cmd.baseInstance = 0;

			// Build draw info
			DrawInfo info;
			info.modelMatrix = model;
			info.aabbMin = glm::vec3(-1.0f); // TODO: Calculate proper AABB for primitives
			info.materialIndex = materialIndex;
			info.aabbMax = glm::vec3(1.0f);
			info.entityID = entity;
			info.flags = 0; // Primitives never use skinning
			info.boneTransformOffset = 0;
			info._pad[0] = 0;
			info._pad[1] = 0;

			// Route to appropriate pass
			if (isTransparent || isCustomShader) {
				// Forward pass (transparent/custom shader)
				m_ForwardPassDrawCommands.push_back(cmd);
				m_ForwardPassDrawInfos.push_back(info);
			}
			else {
				// Geometry pass (opaque, standard shader)
				m_StandardDrawCommands.push_back(cmd);
				m_StandardDrawInfos.push_back(info);
			}
		}
	}

	// ========== SKINNED (ANIMATED) MESHES ==========
	// Iterate through AnimationManager's entities (entities with AnimationComponent)
	for (auto& entity : ecs.GetSystem<graphics::AnimationManager>()->m_Entities) {
		// All entities here have AnimationComponent, no need to check
		if (!ecs.HasComponent<ModelComponent>(entity)) continue;

		auto& modelComp = ecs.GetComponent<ModelComponent>(entity);
		auto& trans = ecs.GetComponent<Transform>(entity);
		auto& animComp = ecs.GetComponent<AnimationComponent>(entity);

		if (!modelComp.m_model) continue;
		if (animComp.boneTransformOffset < 0) continue; // Skip if no valid bone data

		// Check if entity has material component for transparency/custom shader check
		Ermine::graphics::Material* material = nullptr;
		if (ecs.HasComponent<Ermine::Material>(entity)) {
			auto& materialComponent = ecs.GetComponent<Ermine::Material>(entity);
			material = materialComponent.GetMaterial();
		}

		// Build entity transform
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
		glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
		rotQuat = glm::normalize(rotQuat);
		modelMatrix *= glm::mat4_cast(rotQuat);
		modelMatrix = glm::scale(modelMatrix, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

		// Determine which pass this entity belongs to
		bool isTransparent = material && IsTransparentMaterial(material);
		// TODO: Uncomment when use of custom shaders is supported
		//bool isCustomShader = HasCustomShader(material);
		bool isCustomShader = false;

		// Get material index
		uint32_t materialIndex = 0;
		auto it = m_EntityMaterialIndices.find(entity);
		if (it != m_EntityMaterialIndices.end()) {
			materialIndex = it->second;
		}

		// Get bone transform offset
		uint32_t boneOffset = static_cast<uint32_t>(animComp.boneTransformOffset);

		// Process each mesh in the model
		for (const auto& mesh : modelComp.m_model->GetMeshes()) {
			// Get mesh handle from MeshManager
			MeshHandle meshHandle = m_MeshManager.GetMeshHandle(mesh.meshID);
			if (!meshHandle.isValid()) continue;

			const MeshSubset* meshData = m_MeshManager.GetMeshData(meshHandle);
			if (!meshData) continue;

			// Build draw command
			DrawElementsIndirectCommand cmd;
			cmd.count = meshData->indexCount;
			cmd.instanceCount = 1;
			cmd.firstIndex = meshData->indexOffset;
			cmd.baseVertex = meshData->baseVertex;
			cmd.baseInstance = 0;

			// Build draw info with AABB and model matrix
			DrawInfo info;
			info.modelMatrix = modelMatrix;
			info.aabbMin = mesh.aabbMin;
			info.materialIndex = materialIndex;
			info.aabbMax = mesh.aabbMax;
			info.entityID = entity;
			info.flags = 1; // Skinning enabled
			info.boneTransformOffset = boneOffset;
			info._pad[0] = 0;
			info._pad[1] = 0;

			// Route to appropriate pass
			if (isTransparent || isCustomShader) {
				// Forward pass (transparent/custom shader)
				m_ForwardPassDrawCommands.push_back(cmd);
				m_ForwardPassDrawInfos.push_back(info);
			}
			else {
				// Geometry pass (opaque, standard shader)
				m_SkinnedDrawCommands.push_back(cmd);
				m_SkinnedDrawInfos.push_back(info);
			}
		}
	}
}

/**
 * @brief Renders the lighting pass for deferred rendering.
 * @param view View matrix.
 * @param projection Projection matrix.
 */
void Renderer::RenderLightingPass(const Mtx44& view, const Mtx44& projection)
{
	if (!m_GBuffer || !m_LightPassShader)
	{
		EE_CORE_ERROR("G-Buffer or lighting shader not initialized!");
		return;
	}

	BeginLightingPass();

	// Bind lighting shader
	m_LightPassShader->Bind();

	// Bind g-buffer textures for reading
	BindGBufferTextures();

	// Calculate and set inverse matrices for world position reconstruction
	glm::mat4 glmView = glm::mat4(
		view.m00, view.m01, view.m02, view.m03,
		view.m10, view.m11, view.m12, view.m13,
		view.m20, view.m21, view.m22, view.m23,
		view.m30, view.m31, view.m32, view.m33
	);
	glm::mat4 glmProjection = glm::mat4(
		projection.m00, projection.m01, projection.m02, projection.m03,
		projection.m10, projection.m11, projection.m12, projection.m13,
		projection.m20, projection.m21, projection.m22, projection.m23,
		projection.m30, projection.m31, projection.m32, projection.m33
	);

	glm::mat4 invView = glm::inverse(glmView);
	glm::mat4 invProjection = glm::inverse(glmProjection);
	m_LightPassShader->SetUniformMatrix4fv("view", glmView);
	m_LightPassShader->SetUniformMatrix4fv("invView", invView);
	m_LightPassShader->SetUniformMatrix4fv("invProjection", invProjection);
	m_LightPassShader->SetUniformMatrix4fv("projection", glmProjection);
	m_LightPassShader->SetUniform1i("u_SSAO", m_SSAOEnabled ? 1 : 0);
	m_LightPassShader->SetUniform1i("u_SSAOSamples", m_SSAOSamples);
	m_LightPassShader->SetUniform1f("u_SSAORadius", m_SSAORadius);
	m_LightPassShader->SetUniform1f("u_SSAOBias", m_SSAOBias);
	m_LightPassShader->SetUniform1f("u_SSAOIntensity", m_SSAOIntensity);
	m_LightPassShader->SetUniform1f("u_SSAOFadeout", m_SSAOFadeout);
	m_LightPassShader->SetUniform1f("u_SSAOMaxDistance", m_SSAOMaxDistance);

	// Set shading mode
	m_LightPassShader->SetUniform1i("u_ShadingMode", m_IsBlinnPhong ? 1 : 0);

	// Render fullscreen quad
	if (m_QuadMesh.vertex_array && m_QuadMesh.index_buffer)
	{
		Draw(m_QuadMesh.vertex_array, m_QuadMesh.index_buffer);
	}


	EndLightingPass();
}

/**
 * @brief Render post-processing effects using the lighting pass output
 */
void Renderer::RenderPostProcessPass()
{
	if (!m_PostProcessBuffer || !m_BloomShader || !m_PostProcessShader || !m_AAShader)
	{
		EE_CORE_ERROR("Post-process buffers or shaders not initialized!");
		return;
	}

	glDisable(GL_DEPTH_TEST);

	// Pass 1: Extract bright areas
	glBindFramebuffer(GL_FRAMEBUFFER, m_BloomExtractBuffer->FBO);
	m_BloomShader->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_PostProcessBuffer->ColorTexture);
	m_BloomShader->SetUniform1i("u_LightingTexture", 0);
	m_BloomShader->SetUniform1i("u_Pass", 1);

	// Set bloom extraction parameters
	m_BloomShader->SetUniform1f("u_BloomThreshold", m_BloomThreshold);
	m_BloomShader->SetUniform1f("u_BloomRadius", m_BloomRadius);

	Draw(m_QuadMesh.vertex_array, m_QuadMesh.index_buffer);

	// Pass 2: Horizontal blur
	glBindFramebuffer(GL_FRAMEBUFFER, m_BloomBlurBuffer1->FBO);
	m_BloomShader->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_BloomExtractBuffer->ColorTexture);
	m_BloomShader->SetUniform1i("u_Pass", 2);
	Draw(m_QuadMesh.vertex_array, m_QuadMesh.index_buffer);

	// Pass 3: Vertical blur
	glBindFramebuffer(GL_FRAMEBUFFER, m_BloomBlurBuffer2->FBO);
	m_BloomShader->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_BloomBlurBuffer1->ColorTexture);
	m_BloomShader->SetUniform1i("u_LightingTexture", 0);
	m_BloomShader->SetUniform1i("u_Pass", 3);
	Draw(m_QuadMesh.vertex_array, m_QuadMesh.index_buffer);

	// Pass 4: Post-processing (tone mapping, bloom combine, etc.)
	glBindFramebuffer(GL_FRAMEBUFFER, m_AntiAliasingBuffer->FBO);
	glClear(GL_COLOR_BUFFER_BIT);

	m_PostProcessShader->Bind();

	// Bind main scene texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_PostProcessBuffer->ColorTexture);
	m_PostProcessShader->SetUniform1i("u_LightingTexture", 0);

	// Bind bloom texture
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_BloomBlurBuffer2->ColorTexture);
	m_PostProcessShader->SetUniform1i("u_BloomTexture", 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_PostProcessBuffer->DepthTexture);
	m_PostProcessShader->SetUniform1i("u_SceneDepth", 2);

	// Set post-processing toggle parameters
	m_PostProcessShader->SetUniform1i("u_Vignette", m_VignetteEnabled ? 1 : 0);
	m_PostProcessShader->SetUniform1i("u_ToneMapping", m_ToneMappingEnabled ? 1 : 0);
	m_PostProcessShader->SetUniform1i("u_GammaCorrection", m_GammaCorrectionEnabled ? 1 : 0);
	m_PostProcessShader->SetUniform1i("u_Bloom", m_BloomEnabled ? 1 : 0);
	m_PostProcessShader->SetUniform1i("u_SkyboxIsHDR", m_SkyBoxisHDR ? 1 : 0);

	// Set post-processing value parameters
	m_PostProcessShader->SetUniform1f("u_Exposure", m_Exposure);
	m_PostProcessShader->SetUniform1f("u_Contrast", m_Contrast);
	m_PostProcessShader->SetUniform1f("u_Saturation", m_Saturation);
	m_PostProcessShader->SetUniform1f("u_Gamma", m_Gamma);
	m_PostProcessShader->SetUniform1f("u_VignetteIntensity", m_VignetteIntensity);
	m_PostProcessShader->SetUniform1f("u_VignetteRadius", m_VignetteRadius);
	m_PostProcessShader->SetUniform1f("u_BloomStrength", m_BloomStrength);


	Draw(m_QuadMesh.vertex_array, m_QuadMesh.index_buffer);

	// Final pass: FXAA
#if defined(EE_EDITOR)
	glBindFramebuffer(GL_FRAMEBUFFER, m_OffscreenBuffer->FBO);
	glViewport(0, 0, m_OffscreenBuffer->width, m_OffscreenBuffer->height);
#else
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

	glClear(GL_COLOR_BUFFER_BIT);

	m_AAShader->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_AntiAliasingBuffer->ColorTexture);
	m_AAShader->SetUniform1i("u_LightingTexture", 0);

	// Set FXAA parameters
	m_AAShader->SetUniform1i("u_FXAA", m_FXAAEnabled ? 1 : 0);
	m_AAShader->SetUniform1f("u_FXAASpanMax", m_FXAASpanMax);
	m_AAShader->SetUniform1f("u_FXAAReduceMin", m_FXAAReduceMin);
	m_AAShader->SetUniform1f("u_FXAAReduceMul", m_FXAAReduceMul);

	Draw(m_QuadMesh.vertex_array, m_QuadMesh.index_buffer);

	glEnable(GL_DEPTH_TEST);

#if defined(EE_EDITOR)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

// overload that forwards to the existing glm version
void Renderer::RenderDebugLines(const Mtx44& view, const Mtx44& proj)
{
	RenderDebugLines(ToGlm(view), ToGlm(proj));
}

/**
 * @brief Complete deferred rendering pipeline
 * @param view The view matrix
 * @param projection The projection matrix
 */
void Renderer::RenderDeferredPipeline(const Mtx44& view, const Mtx44& projection)
{
	// Compile draw data for all passes (routes opaque to geometry, transparent to forward)
	CompileDrawData();

	// Geometry pass - write opaque objects to g-buffer, collect transparent objects
	RenderGeometryPass(view, projection);

	// Shadow pass - render scene from light's perspective
	if (frameCounter % SHADOW_MAP_REFRESH_INTERVAL_IN_FRAMES == 0)
		RenderShadowPass();

	// Lighting pass - read from g-buffer and perform lighting on opaque objects
	RenderLightingPass(view, projection);

	// Render skybox after lighting but before transparent objects
	if (m_skybox && m_skybox->IsValid() && m_PostProcessBuffer && m_GBuffer) {
		glBindFramebuffer(GL_FRAMEBUFFER, m_PostProcessBuffer->FBO);
		glViewport(0, 0, m_PostProcessBuffer->width, m_PostProcessBuffer->height);

		// Copy depth buffer from g-buffer to post-process buffer
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_GBuffer->FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_PostProcessBuffer->FBO);
		glBlitFramebuffer(0, 0, m_GBuffer->width, m_GBuffer->height,
			0, 0, m_PostProcessBuffer->width, m_PostProcessBuffer->height,
			GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		glBindFramebuffer(GL_FRAMEBUFFER, m_PostProcessBuffer->FBO);

		// Enable depth testing but render only where depth = 1.0 (background)
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);

		// Render skybox
		m_skybox->Render(view, projection);

		// Restore depth state
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
	}

	// TRANSPARENCY PASS - render transparent objects using forward rendering
	RenderForwardPass(view, projection);

#if defined(EE_EDITOR)
	if (m_PostProcessBuffer && ECS::GetInstance().GetSystem<Physics>()->wireframe) {
		glBindFramebuffer(GL_FRAMEBUFFER, m_PostProcessBuffer->FBO);
		glViewport(0, 0, m_PostProcessBuffer->width, m_PostProcessBuffer->height);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_CULL_FACE);

		if (auto physics = ECS::GetInstance().GetSystem<Physics>()) {
			physics->DrawDebugPhysics();
		}
		RenderDebugLines(view, projection);
	}
#endif

	// Post-processing pass - read from lighting + transparency pass output
	RenderPostProcessPass();
}

/**
 * @brief Bind g-buffer textures to specified texture units
 */
void Renderer::BindGBufferTextures()
{
	if (!m_GBuffer)
	{
		EE_CORE_WARN("G-Buffer not initialized, cannot bind textures");
		return;
	}

	if (m_LightPassShader)
	{
		GLint loc0 = glGetUniformLocation(m_LightPassShader->GetRendererID(), "u_GBuffer0Handle");
		GLint loc1 = glGetUniformLocation(m_LightPassShader->GetRendererID(), "u_GBuffer1Handle");
		GLint loc2 = glGetUniformLocation(m_LightPassShader->GetRendererID(), "u_GBuffer2Handle");
		GLint loc3 = glGetUniformLocation(m_LightPassShader->GetRendererID(), "u_GBuffer3Handle");
		GLint locD = glGetUniformLocation(m_LightPassShader->GetRendererID(), "u_GBufferDepthHandle");
		GLint locS = glGetUniformLocation(m_LightPassShader->GetRendererID(), "u_ShadowMapArrayHandle");

		// Bind bindless texture handles
		if (loc0 != -1)
		{
			// Convert 64-bit handle to two 32-bit unsigned integers
			glUniform2ui(loc0, static_cast<GLuint>(m_GBuffer->HandlePackedTexture0),
				static_cast<GLuint>(m_GBuffer->HandlePackedTexture0 >> 32));
		}
		if (loc1 != -1)
		{
			glUniform2ui(loc1, static_cast<GLuint>(m_GBuffer->HandlePackedTexture1),
				static_cast<GLuint>(m_GBuffer->HandlePackedTexture1 >> 32));
		}
		if (loc2 != -1)
		{
			glUniform2ui(loc2, static_cast<GLuint>(m_GBuffer->HandlePackedTexture2),
				static_cast<GLuint>(m_GBuffer->HandlePackedTexture2 >> 32));
		}
		if (loc3 != -1)
		{
			glUniform2ui(loc3, static_cast<GLuint>(m_GBuffer->HandlePackedTexture3),
				static_cast<GLuint>(m_GBuffer->HandlePackedTexture3 >> 32));
		}
		if (locD != -1)
		{
			glUniform2ui(locD, static_cast<GLuint>(m_GBuffer->HandleDepthTexture),
				static_cast<GLuint>(m_GBuffer->HandleDepthTexture >> 32));
		}
		if (locS != -1 && m_ShadowMapArray)
		{
			glUniform2ui(locS, static_cast<GLuint>(m_ShadowMapArrayHandle),
				static_cast<GLuint>(m_ShadowMapArrayHandle >> 32));
		}
	}
}

/**
 * @brief Cleanup g-buffer resources
 */
void Renderer::CleanupGBuffer()
{
	if (m_GBuffer)
	{
		// Make bindless handles non-resident BEFORE deleting textures
		if (m_GBuffer->HandlePackedTexture0 != 0) {
			glMakeTextureHandleNonResidentARB(m_GBuffer->HandlePackedTexture0);
			m_GBuffer->HandlePackedTexture0 = 0;
		}
		if (m_GBuffer->HandlePackedTexture1 != 0) {
			glMakeTextureHandleNonResidentARB(m_GBuffer->HandlePackedTexture1);
			m_GBuffer->HandlePackedTexture1 = 0;
		}
		if (m_GBuffer->HandlePackedTexture2 != 0) {
			glMakeTextureHandleNonResidentARB(m_GBuffer->HandlePackedTexture2);
			m_GBuffer->HandlePackedTexture2 = 0;
		}
		if (m_GBuffer->HandlePackedTexture3 != 0) {
			glMakeTextureHandleNonResidentARB(m_GBuffer->HandlePackedTexture3);
			m_GBuffer->HandlePackedTexture3 = 0;
		}
		if (m_GBuffer->HandleDepthTexture != 0) {
			glMakeTextureHandleNonResidentARB(m_GBuffer->HandleDepthTexture);
			m_GBuffer->HandleDepthTexture = 0;
		}

		if (m_GBuffer->FBO != 0) {
			glDeleteFramebuffers(1, &m_GBuffer->FBO);
		}
		if (m_GBuffer->PackedTexture0 != 0)
		{
			glDeleteTextures(1, &m_GBuffer->PackedTexture0);
		}
		if (m_GBuffer->PackedTexture1 != 0)
		{
			glDeleteTextures(1, &m_GBuffer->PackedTexture1);
		}
		if (m_GBuffer->PackedTexture2 != 0)
		{
			glDeleteTextures(1, &m_GBuffer->PackedTexture2);
		}
		if (m_GBuffer->PackedTexture3 != 0)
		{
			glDeleteTextures(1, &m_GBuffer->PackedTexture3);
		}
		if (m_GBuffer->DepthTexture != 0)
		{
			glDeleteTextures(1, &m_GBuffer->DepthTexture);
		}
		m_GBuffer.reset();
	}
}

/**
 * @brief Cleanup post-processing buffer resources
 */
void Renderer::CleanupPostProcessBuffer()
{
	// Clean up main post-process buffer
	if (m_PostProcessBuffer)
	{
		if (m_PostProcessBuffer->FBO != 0)
		{
			glDeleteFramebuffers(1, &m_PostProcessBuffer->FBO);
			m_PostProcessBuffer->FBO = 0;
		}
		if (m_PostProcessBuffer->ColorTexture != 0)
		{
			glDeleteTextures(1, &m_PostProcessBuffer->ColorTexture);
			m_PostProcessBuffer->ColorTexture = 0;
		}
		if (m_PostProcessBuffer->DepthTexture != 0)
		{
			glDeleteTextures(1, &m_PostProcessBuffer->DepthTexture);
			m_PostProcessBuffer->DepthTexture = 0;
		}
		m_PostProcessBuffer.reset();
	}

	// Clean up bloom extract buffer
	if (m_BloomExtractBuffer)
	{
		if (m_BloomExtractBuffer->FBO != 0)
		{
			glDeleteFramebuffers(1, &m_BloomExtractBuffer->FBO);
			m_BloomExtractBuffer->FBO = 0;
		}
		if (m_BloomExtractBuffer->ColorTexture != 0)
		{
			glDeleteTextures(1, &m_BloomExtractBuffer->ColorTexture);
			m_BloomExtractBuffer->ColorTexture = 0;
		}
		m_BloomExtractBuffer.reset();
	}

	// Clean up bloom blur buffer 1
	if (m_BloomBlurBuffer1)
	{
		if (m_BloomBlurBuffer1->FBO != 0)
		{
			glDeleteFramebuffers(1, &m_BloomBlurBuffer1->FBO);
			m_BloomBlurBuffer1->FBO = 0;
		}
		if (m_BloomBlurBuffer1->ColorTexture != 0)
		{
			glDeleteTextures(1, &m_BloomBlurBuffer1->ColorTexture);
			m_BloomBlurBuffer1->ColorTexture = 0;
		}
		m_BloomBlurBuffer1.reset();
	}

	// Clean up bloom blur buffer 2
	if (m_BloomBlurBuffer2)
	{
		if (m_BloomBlurBuffer2->FBO != 0)
		{
			glDeleteFramebuffers(1, &m_BloomBlurBuffer2->FBO);
			m_BloomBlurBuffer2->FBO = 0;
		}
		if (m_BloomBlurBuffer2->ColorTexture != 0)
		{
			glDeleteTextures(1, &m_BloomBlurBuffer2->ColorTexture);
			m_BloomBlurBuffer2->ColorTexture = 0;
		}
		m_BloomBlurBuffer2.reset();
	}
	// Clean up anti-aliasing buffer
	if (m_AntiAliasingBuffer)
	{
		if (m_AntiAliasingBuffer->FBO != 0)
		{
			glDeleteFramebuffers(1, &m_AntiAliasingBuffer->FBO);
			m_AntiAliasingBuffer->FBO = 0;
		}
		if (m_AntiAliasingBuffer->ColorTexture != 0)
		{
			glDeleteTextures(1, &m_AntiAliasingBuffer->ColorTexture);
			m_AntiAliasingBuffer->ColorTexture = 0;
		}
		if (m_AntiAliasingBuffer->DepthTexture != 0)
		{
			glDeleteTextures(1, &m_AntiAliasingBuffer->DepthTexture);
			m_AntiAliasingBuffer->DepthTexture = 0;
		}
		m_AntiAliasingBuffer.reset();
	}

	// Check for OpenGL errors after cleanup
	glCheckError();
}

/**
 * @brief Updates the lights' uniform buffer object (UBO) with the current light and transform data from all living entities.
 * @param view The view matrix to transform the positions and directions of the lights into view space.
 */
void Renderer::UpdateLightsUBO(const Mtx44& view)
{
	std::vector<LightGPU> lights;
	lights.reserve(MAX_LIGHTS);

	const auto& ecs = Ermine::ECS::GetInstance();
	for (EntityID e : m_LightSystem->m_Entities)
	{
		const auto& trans = ecs.GetComponent<Transform>(e);
		const auto& light = ecs.GetComponent<Light>(e);

		// Keep position in WORLD SPACE instead of view space
		glm::vec4 posWorld(trans.position.x, trans.position.y, trans.position.z, 1.0f);

		// Build rotation from quaternion
		glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
		rotQuat = glm::normalize(rotQuat);

		// Keep direction in WORLD SPACE
		glm::vec3 fwd(0.0f, 0.0f, 1.0f);
		glm::vec3 dirWorld = glm::normalize(rotQuat * fwd);

		// Set spot angles
		float innerCos = 1.0f, outerCos = 1.0f;
		if (light.type == LightType::SPOT) {
			float innerAngle = glm::radians(light.innerAngle);
			float outerAngle = glm::radians(light.outerAngle);
			innerCos = glm::cos(innerAngle);
			outerCos = glm::cos(outerAngle);
		}

		// Convert to LightGPU structure - NOW IN WORLD SPACE
		LightGPU gpu{};
		gpu.position_type = glm::vec4(posWorld.x, posWorld.y, posWorld.z, static_cast<float>(light.type));
		gpu.color_intensity = glm::vec4(light.color.x, light.color.y, light.color.z, light.intensity);
		gpu.direction_range = glm::vec4(dirWorld.x, dirWorld.y, dirWorld.z, light.radius);
		gpu.spot_angles_castshadows_startOffset = glm::vec4(innerCos, outerCos, light.castsShadows, light.startOffset);

		for (int i = 0; i < NUM_CASCADES; ++i) {
			gpu.lightSpaceMatrix[i] = light.lightSpaceMatrices[i];
			gpu.splitDepths[i / 4][i % 4] = light.splitDepths[i];
		}
		lights.emplace_back(gpu);
	}

	// Upload to UBO
	glBindBuffer(GL_UNIFORM_BUFFER, m_LightsUBO);

	glm::vec4 count(static_cast<float>(lights.size()), 0.0f, 0.0f, 0.0f);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::vec4), &count);

	if (!lights.empty())
	{
		const GLsizeiptr bodyOffset = static_cast<GLsizeiptr>(sizeof(glm::vec4));
		const GLsizeiptr bodySize = static_cast<GLsizeiptr>(lights.size() * sizeof(LightGPU));
		glBufferSubData(GL_UNIFORM_BUFFER, bodyOffset, bodySize, lights.data());
	}

	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glCheckError();
}

/**
 * @brief Updates the material's SSBO with the specified material data.
 *
 * If the material SSBO does not exist, this function creates one. It then uploads the given material data
 * into the UBO, making it available to the shader for rendering.
 *
 * @param materialData The material data to be uploaded to the SSBO, including properties like color, texture, etc.
 */
void Renderer::UpdateMaterialSSBO(const graphics::MaterialSSBO& materialData, uint32_t materialIndex)
{
	if (!m_MaterialSSBO)
	{
		EE_CORE_ERROR("MaterialSSBO not initialized - call CompileMaterials() first");
		return;
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_MaterialSSBO);

	const size_t materialSize = sizeof(graphics::MaterialSSBO);
	const size_t offset = materialSize * materialIndex;

	// Upload to specific index in the array
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, materialSize, &materialData);

	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		EE_CORE_ERROR("Failed to update material at index {0}, error: {1}",
			materialIndex, error);
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

/**
* @brief Updates the material's SSBO at index 0 with the specified material data.
 *
 * This is a convenience overload that defaults to updating the first material (index 0).
 *
 * @param materialData The material data to be uploaded to the SSBO, including properties like color, texture, etc.
 */
void Renderer::UpdateMaterialSSBO(const graphics::MaterialSSBO& materialData)
{
	// This version is kept for backward compatibility
	// It updates index 0 by default
	UpdateMaterialSSBO(materialData, 0);
}

/**
 * @brief Update material color properties and upload to SSBO
 * @param entity The entity whose material to update
 * @param albedo The new albedo color
 * @param roughness The new roughness value
 * @param metallic The new metallic value
 * @param emissive The new emissive color
 */
void Renderer::UpdateMaterialColor(EntityID entity,
	const Vec3& albedo,
	float roughness,
	float metallic,
	const Vec3& emissive)
{
	auto& ecs = ECS::GetInstance();
	auto renderer = ecs.GetSystem<Renderer>();
	auto& materialComp = ecs.GetComponent<Ermine::Material>(entity);
	auto* material = materialComp.GetMaterial();

	if (!material) return;

	// Update all properties
	material->SetVec3("materialAlbedo", albedo);
	material->SetFloat("materialRoughness", roughness);
	material->SetFloat("materialMetallic", metallic);
	material->SetVec3("materialEmissive", emissive);

	// Single upload for all changes
	auto ssboData = material->GetSSBOData();
	uint32_t materialIndex = renderer->GetMaterialIndex(entity);
	renderer->UpdateMaterialSSBO(ssboData, materialIndex);
}

/**
 * @brief Retrieves the material index associated with the given entity.
 * @param entity The entity whose material index is to be retrieved.
 * @return The material index for the specified entity, or 0 if not found.
 */
uint32_t Renderer::GetMaterialIndex(EntityID entity) const
{
	auto it = m_EntityMaterialIndices.find(entity);
	if (it != m_EntityMaterialIndices.end()) {
		return it->second;
	}
	return 0; // Default to first material
}

/**
 * @brief Sets the material index uniform before drawing
 * Call this before each draw call to tell shader which material to use
 */
void Renderer::SetMaterialIndex(EntityID entity, const std::shared_ptr<Shader>& shader)
{
	if (!shader || !shader->IsValid()) return;

	auto it = m_EntityMaterialIndices.find(entity);
	if (it != m_EntityMaterialIndices.end())
	{
		shader->SetUniform1i("u_MaterialIndex", static_cast<int>(it->second));
	}
	else
	{
		EE_CORE_WARN("Entity {0} has no material index, using default 0", entity);
		shader->SetUniform1i("u_MaterialIndex", 0);
	}
}

/**
 * @brief Binds the MaterialBlock uniform block to the specified shader program if it has not been bound before.
 * @param shader The shader program to which the material block should be bound.
 */
void Renderer::BindMaterialBlockIfPresent(const std::shared_ptr<Shader>& shader)
{
	if (!shader || !shader->IsValid())
		return;

	const GLuint program = shader->GetRendererID();
	if (m_MaterialBlockBoundPrograms.find(program) != m_MaterialBlockBoundPrograms.end())
		return;

	// Get the shader storage block index instead of uniform block index
	GLuint blockIndex = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, "MaterialBlock");
	if (blockIndex != GL_INVALID_INDEX)
	{
		glShaderStorageBlockBinding(program, blockIndex, MaterialBindingPoint);
		m_MaterialBlockBoundPrograms.insert(program);
	}
}

/**
 * @brief Update all mesh entities and draw them
 */
void Renderer::Update(const Mtx44& view, const Mtx44& projection)
{
	// Update lights UBO
	UpdateLightsUBO(editor::EditorCamera::GetInstance().GetViewMatrix());


	// Check if new meshes have been registered and need uploading
	if (m_MeshManager.IsDirty() && m_MeshManager.HasStagedData())
	{
		m_MeshManager.UploadAndBuild();
		EE_CORE_INFO("MeshManager: Uploaded {} new meshes during runtime", m_MeshManager.GetMeshCount());
	}

	// Compile materials on first update when entities exist
	if (m_MaterialsDirty && !m_Entities.empty())
	{
		CompileMaterials();
	}
	if (m_UseDeferredRendering)
	{
		// Use deferred rendering pipeline (now includes transparency)
		RenderDeferredPipeline(view, projection);
	}
	else
	{
		// Forward rendering with transparency support
#if defined(EE_EDITOR)
		glBindFramebuffer(GL_FRAMEBUFFER, m_OffscreenBuffer->FBO);
		glViewport(0, 0, m_OffscreenBuffer->width, m_OffscreenBuffer->height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

		// Render skybox FIRST as the background
		if (m_skybox && m_skybox->IsValid()) {
			glDepthMask(GL_FALSE);
			m_skybox->Render(view, projection);
			glDepthMask(GL_TRUE);
		}

		// Clear transparent objects from previous frame
		m_transparentObjects.clear();

		// Calculate camera position for transparent sorting
		glm::mat4 glmView = glm::mat4(
			view.m00, view.m01, view.m02, view.m03,
			view.m10, view.m11, view.m12, view.m13,
			view.m20, view.m21, view.m22, view.m23,
			view.m30, view.m31, view.m32, view.m33
		);
		glm::mat4 invView = glm::inverse(glmView);
		Vec3 cameraPos = Vec3(invView[3][0], invView[3][1], invView[3][2]);

		auto& ecs = ECS::GetInstance();

		// First pass: Render opaque objects and collect transparent objects
		for (auto& entity : m_Entities
			)
		{
			// Model pipeline
			if (ecs.HasComponent<ModelComponent>(entity) && ecs.HasComponent<Ermine::Material>(entity))
			{
				auto& trans = ecs.GetComponent<Transform>(entity);
				auto& modelComp = ecs.GetComponent<ModelComponent>(entity);
				auto& materialComp = ecs.GetComponent<Ermine::Material>(entity);

				if (!modelComp.m_model) continue;

				// Build entity transform
				glm::mat4 entityModel = glm::mat4(1.0f);
				entityModel = glm::translate(entityModel, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
				glm::quat rotQuat = glm::quat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
				rotQuat = glm::normalize(rotQuat);
				entityModel *= glm::mat4_cast(rotQuat);
				entityModel = glm::scale(entityModel, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

				// Check if entity has material for transparency check
				Ermine::graphics::Material* material = nullptr;
				if (ecs.HasComponent<Ermine::Material>(entity)) {
					auto& materialComponent = ecs.GetComponent<Ermine::Material>(entity);
					material = materialComponent.GetMaterial();
				}

				// Check if transparent
				if (material && IsTransparentMaterial(material)) {
					TransparentObject transparentObj;
					transparentObj.entity = entity;
					transparentObj.modelMatrix = entityModel;
					transparentObj.distanceToCamera = 0.0f; // Will be calculated in SortTransparentObjects
					m_transparentObjects.push_back(transparentObj);
					continue; // Skip opaque rendering
				}


				RenderModelDeferred(*modelComp.m_model, materialComp.GetMaterial(), view, projection, entityModel);
			}
			// Mesh + material pipeline
			else if (ecs.HasComponent<Mesh>(entity) && ecs.HasComponent<Ermine::Material>(entity))
			{
				auto& trans = ecs.GetComponent<Transform>(entity);
				auto& mesh = ecs.GetComponent<Mesh>(entity);
				auto& materialComponent = ecs.GetComponent<Ermine::Material>(entity);

				if (!mesh.vertex_array || !mesh.index_buffer) continue;

				Ermine::graphics::Material* material = materialComponent.GetMaterial();
				if (!material) {
					EE_CORE_WARN("Entity {0} has null material", entity);
					continue;
				}

				// Build model matrix
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
				glm::quat rotQuat = glm::quat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
				rotQuat = glm::normalize(rotQuat);
				model *= glm::mat4_cast(rotQuat);
				model = glm::scale(model, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

				// Check if material is transparent
				if (IsTransparentMaterial(material)) {
					TransparentObject transparentObj;
					transparentObj.entity = entity;
					transparentObj.modelMatrix = model;
					transparentObj.distanceToCamera = 0.0f; // Will be calculated in SortTransparentObjects
					m_transparentObjects.push_back(transparentObj);
					continue; // Skip opaque rendering in geometry pass
				}

				// Render opaque object
				auto shader = material->GetShader();
				if (!shader || !shader->IsValid()) {
					EE_CORE_WARN("Entity {0} has invalid shader", entity);
					continue;
				}


				// Bind material (this handles shader binding and texture binding)
				material->Bind();

				// Bind uniform blocks
				BindMaterialBlockIfPresent(shader);

				// Disable skinning for primitive meshes
				shader->SetUniform1i("u_UseSkinning", 0);

				// Set transformation matrices
				shader->SetUniformMatrix4fv("model", model);
				shader->SetUniformMatrix4fv("view", &view.m2[0][0]);
				shader->SetUniformMatrix4fv("projection", &projection.m2[0][0]);

				glm::mat4 modelView = glmView * model;
				glm::mat3 normalMatrix = transpose(inverse(glm::mat3(modelView)));
				shader->SetUniformMatrix3fv("NormalMatrix", normalMatrix);

				// Set shading mode
				shader->SetUniform1i("isBlinnPhong", m_IsBlinnPhong ? 1 : 0);

				// Draw the mesh
				Draw(mesh.vertex_array, mesh.index_buffer);

				// Unbind material
				material->Unbind();
			}
		}

		// Sort transparent objects by distance from camera
		SortTransparentObjects(cameraPos);

		// Second pass: Render transparent objects in sorted order
		if (!m_transparentObjects.empty()) {
			// Enable alpha blending for transparency
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBlendEquation(GL_FUNC_ADD);

			// Enable depth testing but disable depth writing
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDepthMask(GL_FALSE);

			// Disable face culling for transparent objects
			glDisable(GL_CULL_FACE);

			// Render transparent objects back-to-front
			for (const auto& transparentObj : m_transparentObjects) {
				EntityID entity = transparentObj.entity;

				// Handle ModelComponent entities
				if (ecs.HasComponent<ModelComponent>(entity)) {
					auto& modelComp = ecs.GetComponent<ModelComponent>(entity);
					if (modelComp.m_model) {
						// Get material for proper transparency shader
						Ermine::graphics::Material* material = nullptr;
						if (ecs.HasComponent<Ermine::Material>(entity)) {
							auto& materialComponent = ecs.GetComponent<Ermine::Material>(entity);
							material = materialComponent.GetMaterial();
						}
					}
				}
				// Handle Mesh entities
				else if (ecs.HasComponent<Mesh>(entity) && ecs.HasComponent<Ermine::Material>(entity)) {
					auto& mesh = ecs.GetComponent<Mesh>(entity);
					auto& materialComponent = ecs.GetComponent<Ermine::Material>(entity);

					if (!mesh.vertex_array || !mesh.index_buffer) continue;

					Ermine::graphics::Material* material = materialComponent.GetMaterial();
					if (!material) continue;

					auto shader = material->GetShader();
					if (!shader || !shader->IsValid()) continue;

					// Bind material
					material->Bind();

					// Bind uniform blocks
					BindMaterialBlockIfPresent(shader);

					// Set transformation matrices
					shader->SetUniformMatrix4fv("model", transparentObj.modelMatrix);
					shader->SetUniformMatrix4fv("view", &view.m2[0][0]);
					shader->SetUniformMatrix4fv("projection", &projection.m2[0][0]);

					// Calculate normal matrix
					glm::mat4 glmView = glm::mat4(
						view.m00, view.m01, view.m02, view.m03,
						view.m10, view.m11, view.m12, view.m13,
						view.m20, view.m21, view.m22, view.m23,
						view.m30, view.m31, view.m32, view.m33
					);

					glm::mat4 modelView = glmView * transparentObj.modelMatrix;
					glm::mat3 normalMatrix = transpose(inverse(glm::mat3(modelView)));
					shader->SetUniformMatrix3fv("NormalMatrix", normalMatrix);

					// Set shading mode
					shader->SetUniform1i("isBlinnPhong", m_IsBlinnPhong ? 1 : 0);

					// Bind textures
					int texUnit = 0;
					if (material->HasParameter("materialAlbedoMap")) {
						std::shared_ptr<Texture> albedo = material->GetParameter("materialAlbedoMap")->texture;
						if (albedo && albedo->IsValid()) {
							albedo->Bind(texUnit);
						}
					}
					texUnit++;

					if (material->HasParameter("materialNormalMap")) {
						std::shared_ptr<Texture> normal = material->GetParameter("materialNormalMap")->texture;
						if (normal && normal->IsValid()) {
							normal->Bind(texUnit);
							shader->SetUniform1i("materialNormalMap", texUnit);
						}
					}
					texUnit++;

					if (material->HasParameter("materialRoughnessMap")) {
						std::shared_ptr<Texture> roughness = material->GetParameter("materialRoughnessMap")->texture;
						if (roughness && roughness->IsValid()) {
							roughness->Bind(texUnit);
							shader->SetUniform1i("materialRoughnessMap", texUnit);
						}
					}
					texUnit++;

					if (material->HasParameter("materialMetallicMap")) {
						std::shared_ptr<Texture> metallic = material->GetParameter("materialMetallicMap")->texture;
						if (metallic && metallic->IsValid()) {
							metallic->Bind(texUnit);
							shader->SetUniform1i("materialMetallicMap", texUnit);
						}
					}
					texUnit++;

					// Draw the mesh
					Draw(mesh.vertex_array, mesh.index_buffer);

					// Unbind material
					material->Unbind();
				}
			}

			// Restore render state after transparent rendering
			glDepthMask(GL_TRUE);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glDisable(GL_BLEND);
		}

#if defined(EE_EDITOR)
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
	}

	// Increment  frame counter at the end of the frame
	frameCounter++;
}

/**
 * @brief Draw the mesh
 * @param vao The vertex array object
 * @param ibo The index buffer object
 * @param shader The shader object
 */
void Renderer::Draw(const std::shared_ptr<VertexArray>& vao, const std::shared_ptr<IndexBuffer>& ibo) const
{
	vao->Bind();
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(ibo->GetCount()), GL_UNSIGNED_INT, 0);
	GPUProfiler::TrackDrawCall(
		static_cast<uint32_t>(vao->GetVertexCount()),
		ibo->GetCount()
	);
	vao->Unbind();
}

/**
 * @brief Draws a mesh using instanced rendering.
 * @param vao Vertex array object.
 * @param ibo Index buffer object.
 * @param instanceCount Number of instances.
 */
void Renderer::DrawInstanced(const std::shared_ptr<VertexArray>& vao, const std::shared_ptr<IndexBuffer>& ibo, int instanceCount) const
{
	vao->Bind();
	glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(ibo->GetCount()), GL_UNSIGNED_INT, 0, instanceCount);
	GPUProfiler::TrackDrawCall(
		static_cast<uint32_t>(vao->GetVertexCount()),
		ibo->GetCount()
	);
	vao->Unbind();
}

/**
 * @brief Clear the screen
 */
void Renderer::Clear() const
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/**
 * @brief Get the current performance metrics
 * @return CurrentGPU performance metrics
 */
const GPUProfiler::PerformanceMetrics& Renderer::GetPerformanceMetrics() const
{
	return GPUProfiler::GetMetrics();
}

/**
 * @brief Destructor for Renderer
 */
Renderer::~Renderer()
{
	// Check if we have a valid OpenGL context
	if (glfwGetCurrentContext() == nullptr)
	{
		// DO NOT LOG - logger may be destroyed during shutdown
		return;
	}

	// Make all bindless texture handles non-resident FIRST
	try
	{
		// Make texture array handles non-resident
		if (m_TextureArraySSBO != 0)
		{
			// Get all texture handles and make them non-resident
			for (GLuint textureID : m_TextureArray)
			{
				GLuint64 handle = glGetTextureHandleARB(textureID);
				if (handle != 0 && glIsTextureHandleResidentARB(handle))
				{
					glMakeTextureHandleNonResidentARB(handle);
				}
			}
			m_TextureArray.clear();
			m_TextureIDToIndex.clear();
			m_TexturePathToIndex.clear();
		}

		// Clean up shadow map handle
		if (m_ShadowMapArrayHandle != 0)
		{
			if (glIsTextureHandleResidentARB(m_ShadowMapArrayHandle))
			{
				glMakeTextureHandleNonResidentARB(m_ShadowMapArrayHandle);
			}
			m_ShadowMapArrayHandle = 0;
		}

		// Clean up g-buffer handles
		CleanupGBuffer();

		// Clean up post-process buffers
		CleanupPostProcessBuffer();

		// Now delete buffers
		if (m_LightsUBO)
		{
			glDeleteBuffers(1, &m_LightsUBO);
			m_LightsUBO = 0;
		}

		if (m_MaterialSSBO)
		{
			glDeleteBuffers(1, &m_MaterialSSBO);
			m_MaterialSSBO = 0;
		}

		if (m_TextureArraySSBO)
		{
			glDeleteBuffers(1, &m_TextureArraySSBO);
			m_TextureArraySSBO = 0;
		}

		// Delete shadow map resources
		if (m_ShadowMapArray)
		{
			glDeleteTextures(1, &m_ShadowMapArray);
			m_ShadowMapArray = 0;
		}

		if (m_ShadowMapFBO)
		{
			glDeleteFramebuffers(1, &m_ShadowMapFBO);
			m_ShadowMapFBO = 0;
		}

		// Clean up offscreen buffer
		if (m_OffscreenBuffer)
		{
			if (m_OffscreenBuffer->FBO != 0)
			{
				glDeleteFramebuffers(1, &m_OffscreenBuffer->FBO);
			}
			if (m_OffscreenBuffer->ColorTexture != 0)
			{
				glDeleteTextures(1, &m_OffscreenBuffer->ColorTexture);
			}
			if (m_OffscreenBuffer->RBO != 0)
			{
				glDeleteRenderbuffers(1, &m_OffscreenBuffer->RBO);
			}
			m_OffscreenBuffer.reset();
		}

		// Clean up picking buffer
		if (m_PickingBuffer)
		{
			if (m_PickingBuffer->FBO != 0)
			{
				glDeleteFramebuffers(1, &m_PickingBuffer->FBO);
			}
			if (m_PickingBuffer->ColorID != 0)
			{
				glDeleteTextures(1, &m_PickingBuffer->ColorID);
			}
			if (m_PickingBuffer->Depth != 0)
			{
				glDeleteRenderbuffers(1, &m_PickingBuffer->Depth);
			}
			m_PickingBuffer.reset();
		}

		// Clean up debug rendering resources
		if (m_DebugVAO != 0)
		{
			glDeleteVertexArrays(1, &m_DebugVAO);
			m_DebugVAO = 0;
		}

		if (m_DebugVBO != 0)
		{
			glDeleteBuffers(1, &m_DebugVBO);
			m_DebugVBO = 0;
		}

		// Don't call glCheckError here as context might be shutting down
		// Instead, just clear any pending errors silently
		while (glGetError() != GL_NO_ERROR);
	}
	catch (...)
	{
	}
}

/**
 * @brief Toggles between forward and deferred rendering pipelines.
 *
 * This function flips the internal flag @c m_UseDeferredRendering. When enabled,
 * all rendering will go through the deferred pipeline using a G-buffer and lighting pass.
 * When disabled, rendering falls back to forward shading, where each object is drawn directly
 * with its material and lighting applied in a single pass.
 *
 * It also logs a message indicating the current rendering mode.
 */
void Renderer::ToggleDeferredRendering()
{
	m_UseDeferredRendering = !m_UseDeferredRendering;
	if (m_UseDeferredRendering)
		EE_CORE_INFO("Switched to Deferred Rendering");
	else
		EE_CORE_INFO("Switched to Forward Rendering");

}

/**
 * @brief Renders a model using the deferred rendering pipeline.
 *
 * In this mode, the function uses the shared G-buffer shader (@c m_GBufferShader) to
 * write geometry data (position, normals, material properties) into the G-buffer.
 * Per-entity materials are not bound as shaders, but their UBO data (albedo, metallic,
 * roughness, emissive, etc.) is uploaded to the GPU so the G-buffer can store them.
 *
 * @param model The model to render, containing mesh geometry and local transforms.
 * @param material Pointer to the material providing UBO data (albedo, metallic, etc.).
 * @param view The view matrix representing the camera transform.
 * @param projection The projection matrix (perspective or orthographic).
 * @param rootTransform Root transform matrix for the entity (translation, rotation, scale).
 */
void Renderer::RenderModelDeferred(const Model& model, graphics::Material* material, const Mtx44& view, const Mtx44& projection, const glm::mat4& rootTransform)
{
	const auto& meshes = model.GetMeshes();
	if (meshes.empty() || !material) return;
	if (!m_GBufferShader || !m_GBufferShader->IsValid()) return;

	// Bind shared g-buffer shader used to write geometry information
	m_GBufferShader->Bind();

	// convert the Mtx44 view/projection into glm mats for convenience
	glm::mat4 glmView = ToGlm(view);
	glm::mat4 glmProj = ToGlm(projection);

	// Get bone transforms (may be empty for static meshes)
	const auto& boneTransforms = model.GetBoneTransforms();
	const bool hasBones = !boneTransforms.empty();
	m_GBufferShader->SetUniform1i("u_UseSkinning", hasBones ? 1 : 0); // Enable skinning for model

	// Upload bone matrices if present
	if (hasBones)
	{
		GLsizei count = std::min((int)boneTransforms.size(), MAX_BONE_UNIFORMS);
		GLint loc = glGetUniformLocation(m_GBufferShader->GetRendererID(), "u_BoneMatrices");
		glUniformMatrix4fv(loc, count, GL_FALSE, glm::value_ptr(boneTransforms[0]));
	}

	for (const auto& mesh : meshes)
	{
		if (!mesh.vao || !mesh.ibo) continue;

		glm::mat4 modelMat = rootTransform * mesh.localTransform;
		m_GBufferShader->SetUniformMatrix4fv("model", modelMat);
		m_GBufferShader->SetUniformMatrix4fv("view", glmView);
		m_GBufferShader->SetUniformMatrix4fv("projection", glmProj);

		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMat)));
		m_GBufferShader->SetUniformMatrix3fv("NormalMatrix", normalMatrix);

		Draw(mesh.vao, mesh.ibo);
	}
}

/**
 * @brief Renders a model using the forward rendering pipeline.
 *
 * In this mode, the function binds the entity's own material and its shader, then
 * issues draw calls for each mesh in the model. Lighting and material shading are
 * evaluated directly during rasterization (per-fragment).
 *
 * @param model The model to render, containing mesh geometry and local transforms.
 * @param material Pointer to the material to bind, providing textures and shader.
 * @param view The view matrix representing the camera transform.
 * @param projection The projection matrix (perspective or orthographic).
 * @param rootTransform Root transform matrix for the entity (translation, rotation, scale).
 */
void Renderer::RenderModelForward(const Model& model, graphics::Material* material, const Mtx44& view, const Mtx44& projection, const glm::mat4& rootTransform)
{
	const auto& meshes = model.GetMeshes();
	if (meshes.empty() || !material) return;

	auto shader = material->GetShader();
	if (!shader || !shader->IsValid()) return;


	glm::mat4 glmView = ToGlm(view);
	glm::mat4 glmProj = ToGlm(projection);

	const auto& boneTransforms = model.GetBoneTransforms();
	const bool hasBones = !boneTransforms.empty();
	shader->SetUniform1i("u_UseSkinning", hasBones ? 1 : 0); // Enable skinning for model

	// bind material (textures, shader)
	material->Bind();
	BindMaterialBlockIfPresent(shader);

	// Upload bone matrices if present
	if (hasBones)
	{
		GLsizei count = std::min((int)boneTransforms.size(), MAX_BONE_UNIFORMS);
		GLint loc = glGetUniformLocation(shader->GetRendererID(), "u_BoneMatrices");
		glUniformMatrix4fv(loc, count, GL_FALSE, glm::value_ptr(boneTransforms[0]));
	}

	for (const auto& mesh : meshes)
	{
		if (!mesh.vao || !mesh.ibo) continue;

		glm::mat4 modelMat = rootTransform * mesh.localTransform;
		shader->SetUniformMatrix4fv("model", modelMat);
		shader->SetUniformMatrix4fv("view", glmView);
		shader->SetUniformMatrix4fv("projection", glmProj);

		glm::mat4 modelView = glmView * modelMat;
		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelView)));
		shader->SetUniformMatrix3fv("NormalMatrix", normalMatrix);
		Draw(mesh.vao, mesh.ibo);
	}

	material->Unbind();
}

/**
 * @brief Checks if a material is transparent.
 * @param material Material pointer.
 * @return true if transparent, false otherwise.
 */
bool Renderer::IsTransparentMaterial(const Ermine::graphics::Material* material) const
{
	if (!material) return false;

	// Check if material has albedo with alpha for transparency
	if (auto albedoParam = material->GetParameter("materialAlbedo")) {
		if (albedoParam->type == MaterialParamType::VEC4 &&
			albedoParam->floatValues.size() >= 4) {
			float alpha = albedoParam->floatValues[3];
			return alpha < 0.99f; // Consider transparent if alpha < 99%
		}
		// If albedo is Vec3, check if material has an albedo texture with alpha
		else if (albedoParam->type == MaterialParamType::VEC3) {
			// Check if there's an albedo texture that might have alpha
			if (auto albedoTexParam = material->GetParameter("materialAlbedoMap")) {
				if (albedoTexParam->type == MaterialParamType::TEXTURE_2D &&
					albedoTexParam->texture && albedoTexParam->texture->IsValid()) {
					// For texture-based materials, we can't easily check alpha without loading the texture
					// For now, assume opaque unless explicitly marked as transparent
					return false;
				}
			}
		}
	}

	// Default to opaque if no transparency information is found
	return false;
}

bool Renderer::HasCustomShader(const Ermine::graphics::Material* material) const
{
	if (!material) return false;

	// Check if material has a custom shader (not using standard deferred pipeline)
	auto shader = material->GetShader();
	return shader && shader != m_GBufferShader;
}

/**
 * @brief Sorts transparent objects by distance to camera.
 * @param cameraPos Camera position.
 */
void Renderer::SortTransparentObjects(const Vec3& cameraPos)
{
	if (m_ForwardPassDrawInfos.empty()) return;

	glm::vec3 camPos = glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z);

	// Create indices for sorting
	std::vector<size_t> indices(m_ForwardPassDrawInfos.size());
	std::iota(indices.begin(), indices.end(), 0);

	// Calculate distances and sort indices back-to-front
	std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
		// Extract position from model matrix
		glm::vec3 posA = glm::vec3(m_ForwardPassDrawInfos[a].modelMatrix[3]);
		glm::vec3 posB = glm::vec3(m_ForwardPassDrawInfos[b].modelMatrix[3]);

		float distA = glm::distance(posA, camPos);
		float distB = glm::distance(posB, camPos);

		return distA > distB; // Back-to-front for alpha blending
		});

	// Reorder draw commands and draw infos based on sorted indices
	std::vector<DrawElementsIndirectCommand> sortedCommands;
	std::vector<DrawInfo> sortedInfos;
	sortedCommands.reserve(m_ForwardPassDrawCommands.size());
	sortedInfos.reserve(m_ForwardPassDrawInfos.size());

	for (size_t idx : indices) {
		sortedCommands.push_back(m_ForwardPassDrawCommands[idx]);
		sortedInfos.push_back(m_ForwardPassDrawInfos[idx]);
	}

	m_ForwardPassDrawCommands = std::move(sortedCommands);
	m_ForwardPassDrawInfos = std::move(sortedInfos);
}

/**
 * @brief Renders all transparent objects using forward rendering.
 * @param view View matrix.
 * @param projection Projection matrix.
 */
void Renderer::RenderForwardPass(const Mtx44& view, const Mtx44& projection)
{
	if (m_ForwardPassDrawCommands.empty()) {
		return;
	}

	// Bind the post-process buffer where the opaque scene was rendered
	if (!m_PostProcessBuffer) {
		EE_CORE_ERROR("Post-process buffer not initialized for transparent pass!");
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, m_PostProcessBuffer->FBO);
	glViewport(0, 0, m_PostProcessBuffer->width, m_PostProcessBuffer->height);

	// Enable alpha blending for transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	// Enable depth testing but disable depth writing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE); // Don't write to depth buffer

	// Disable face culling for transparent objects (they might be viewed from inside)
	glDisable(GL_CULL_FACE);

	// Bind forward shader
	if (!m_ForwardShader || !m_ForwardShader->IsValid()) {
		EE_CORE_ERROR("Forward shader not initialized!");
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		return;
	}

	m_ForwardShader->Bind();

	// Set view and projection uniforms
	m_ForwardShader->SetUniformMatrix4fv("view", &view.m2[0][0]);
	m_ForwardShader->SetUniformMatrix4fv("projection", &projection.m2[0][0]);

	// Upload forward pass draw commands to GPU
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_MeshManager.m_DrawCommandsSSBO);
	size_t commandsBufferSize = m_ForwardPassDrawCommands.size() * sizeof(DrawElementsIndirectCommand);
	glBufferData(GL_SHADER_STORAGE_BUFFER, commandsBufferSize, m_ForwardPassDrawCommands.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Upload forward pass draw infos to persistent mapped buffer after geometry pass data
	size_t drawInfoOffset = m_StandardDrawCommands.size() + m_SkinnedDrawCommands.size();
	m_MeshManager.m_PersistentDrawInfoBuffer.WriteDrawInfos(m_ForwardPassDrawInfos, drawInfoOffset);

	// Set baseDrawID = offset so gl_DrawID in shader accesses correct DrawInfo indices
	m_ForwardShader->SetUniform1ui("baseDrawID", static_cast<uint32_t>(drawInfoOffset));

	// Determine which VAO to use (check if any meshes are skinned)
	bool hasSkinned = false;
	for (const auto& info : m_ForwardPassDrawInfos) {
		if (info.flags & 1) {
			hasSkinned = true;
			break;
		}
	}

	// For now, use StandardVAO for all (TODO: separate skinned/standard batches)
	GLuint vaoToUse = hasSkinned ? m_MeshManager.GetSkinnedVAO() : m_MeshManager.GetStandardVAO();
	if (vaoToUse == 0) {
		EE_CORE_ERROR("VAO not initialized for forward pass!");
		glDepthMask(GL_TRUE);
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		return;
	}

	// Bind VAO and issue draw call
	glBindVertexArray(vaoToUse);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_MeshManager.m_DrawCommandsSSBO);
	glMultiDrawElementsIndirect(
		GL_TRIANGLES,
		GL_UNSIGNED_INT,
		nullptr,
		static_cast<GLsizei>(m_ForwardPassDrawCommands.size()),
		0
	);

	// Unbind
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	glBindVertexArray(0);

	// Restore render state
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDisable(GL_BLEND);

	GPUProfiler::EndEvent();
}

#pragma region Shadow Mapping
/**
 * @brief Initializes the shadow map framebuffer object (FBO).
 * Creates and binds the FBO for shadow mapping. If a depth texture array exists, attaches it.
 * Does not validate completeness unless a depth attachment is present.
 * @return True if the FBO was successfully created, false otherwise.
 */
bool Renderer::InitializeShadowMap()
{
	if (m_ShadowMapFBO != 0)
	{
		glDeleteFramebuffers(1, &m_ShadowMapFBO);
		m_ShadowMapFBO = 0;
	}

	glGenFramebuffers(1, &m_ShadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMapFBO);

	// If a depth texture already exists attach it. Otherwise we create the FBO now
	// and defer attachment until CreateShadowMap is called.
	if (m_ShadowMapArray != 0)
	{
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_ShadowMapArray, 0);
	}

	// No color buffer is drawn
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	// Only check completeness if we already have a depth attachment.
	if (m_ShadowMapArray != 0)
	{
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			EE_CORE_ERROR("Shadow map FBO incomplete: {0}", status);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return false;
		}
	}
	else
	{
		EE_CORE_INFO("Initialized shadow FBO (no depth texture attached yet).");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return m_ShadowMapFBO != 0;
}

/**
 * @brief Creates a shadow map texture array for cascaded shadow mapping.
 * Attempts to allocate a depth texture array with as many layers as possible, falling back if allocation fails.
 * Attaches the texture array to the shadow map FBO and sets up bindless texture handle.
 * @return True if the texture array was successfully created and attached, false otherwise.
 */
bool Renderer::CreateShadowMapArray()
{
	// Query hardware limit for texture array layers
	GLint maxLayers;
	glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxLayers);

	// Clean up previous array if it exists
	if (m_ShadowMapArray)
	{
		if (m_ShadowMapArrayHandle != 0) {
			glMakeTextureHandleNonResidentARB(m_ShadowMapArrayHandle);
			m_ShadowMapArrayHandle = 0;
		}
		glDeleteTextures(1, &m_ShadowMapArray);
		m_ShadowMapArray = 0;
	}

	// Create depth texture array with fallback logic
	glGenTextures(1, &m_ShadowMapArray);
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_ShadowMapArray);

	bool allocationSuccessful = false;
	unsigned int attemptedLayers = SHADOW_MAX_LAYERS;

	// Try allocating with progressively fewer layers until successful
	while (!allocationSuccessful && attemptedLayers >= 4) {
		glTexImage3D(
			GL_TEXTURE_2D_ARRAY,
			0,
			GL_DEPTH_COMPONENT16,
			SHADOW_MAP_RESOLUTION,
			SHADOW_MAP_RESOLUTION,
			attemptedLayers,
			0,
			GL_DEPTH_COMPONENT,
			GL_FLOAT,
			nullptr
		);

		GLenum error = glGetError();
		if (error == GL_NO_ERROR) {
			allocationSuccessful = true;
			SHADOW_MAX_LAYERS = attemptedLayers;
			EE_CORE_WARN("Successfully allocated {0} shadow map layers", SHADOW_MAX_LAYERS);
		}
		else {
			EE_CORE_WARN("Failed to allocate {0} layers (error: {1}), trying {2}",
				attemptedLayers, error, attemptedLayers / 2);
			attemptedLayers /= 2;
		}
	}

	if (!allocationSuccessful) {
		EE_CORE_ERROR("Failed to allocate shadow map array even with minimum layers");
		glDeleteTextures(1, &m_ShadowMapArray);
		m_ShadowMapArray = 0;
		return false;
	}

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

	// Create FBO if needed
	if (m_ShadowMapFBO == 0)
		glGenFramebuffers(1, &m_ShadowMapFBO);

	// FIXED: Attach entire texture array for layered rendering
	glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMapFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_ShadowMapArray, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		EE_CORE_ERROR("CreateShadowMapArray: FBO incomplete after attaching depth array: {0}", status);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteTextures(1, &m_ShadowMapArray);
		m_ShadowMapArray = 0;
		return false;
	}

	// Bindless handle for array
	m_ShadowMapArrayHandle = glGetTextureHandleARB(m_ShadowMapArray);
	glMakeTextureHandleResidentARB(m_ShadowMapArrayHandle);

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glCheckError();

	return m_ShadowMapArray != 0;
}

/**
 * @brief Computes the eight frustum corners in world space for a given cascade split.
 * Unprojects normalized device coordinates (NDC) to world space using the inverse projection-view matrix.
 * @param invPV Inverse projection-view matrix.
 * @param nearSplit NDC Z value for the near plane of the cascade.
 * @param farSplit NDC Z value for the far plane of the cascade.
 * @return Array of eight world-space frustum corners.
 */
std::array<glm::vec3, 8> Renderer::createCascadeFrustum(const glm::mat4& invPV, float nearSplit, float farSplit) {
	std::array<glm::vec3, 8> frustumCorners;

	// Lambda function to unproject NDC to world space
	auto UnprojectNDC = [&](float ndcX, float ndcY, float ndcZ) -> glm::vec3 {
		glm::vec4 ndc(ndcX, ndcY, ndcZ, 1.0f);
		glm::vec4 world = invPV * ndc;
		if (world.w != 0.0f) world /= world.w;
		return glm::vec3(world);
		};

	const float xs[2] = { -1.0f, 1.0f };
	const float ys[2] = { -1.0f, 1.0f };

	int idx = 0;
	for (int iz = 0; iz < 2; ++iz) {
		float zndc = (iz == 0) ? nearSplit : farSplit;
		for (int iy = 0; iy < 2; ++iy) {
			for (int ix = 0; ix < 2; ++ix) {
				frustumCorners[idx++] = UnprojectNDC(xs[ix], ys[iy], zndc);
			}
		}
	}

	return frustumCorners;
}

/**
 * @brief Tests whether a spotlight's cone intersects a given frustum.
 * Checks if any frustum corner is inside the spotlight cone or if the cone intersects the frustum's AABB.
 * @param lightPos Position of the spotlight.
 * @param spotDir Direction vector of the spotlight.
 * @param outerAngleRad Outer angle of the spotlight cone in radians.
 * @param lightRadius Maximum range of the spotlight.
 * @param frustumCorners Array of eight frustum corners in world space.
 * @return True if the spotlight cone intersects the frustum, false otherwise.
 */
bool Renderer::testSpotlightFrustumIntersection(const glm::vec3& lightPos, const glm::vec3& spotDir,
	float outerAngleRad, float lightRadius,
	const std::array<glm::vec3, 8>& frustumCorners) {

	// Test if any frustum corner is inside spotlight cone. If so, they intersect.
	for (const auto& corner : frustumCorners) {
		glm::vec3 toCorner = corner - lightPos;
		float distanceToCorner = glm::length(toCorner);

		// Check if corner is within spotlight range
		if (distanceToCorner <= lightRadius && distanceToCorner > 0.01f) {
			glm::vec3 dirToCorner = toCorner / distanceToCorner;
			float angleToCorner = std::acos(glm::clamp(glm::dot(spotDir, dirToCorner), -1.0f, 1.0f));

			// Check if corner is within spotlight cone
			if (angleToCorner <= outerAngleRad) {
				return true;
			}
		}
	}

	// Test if spotlight cone intersects frustum planes
	// Create cone vertices at far plane
	const int numSamples = 8;
	float coneRadius = lightRadius * std::tan(outerAngleRad);

	// Create cone basis vectors
	glm::vec3 right = glm::normalize(glm::cross(spotDir, glm::vec3(0.0f, 1.0f, 0.0f)));
	if (glm::length(right) < 0.1f) {
		right = glm::normalize(glm::cross(spotDir, glm::vec3(1.0f, 0.0f, 0.0f)));
	}
	glm::vec3 up = glm::normalize(glm::cross(right, spotDir));

	// Test cone apex and circumference points
	glm::vec3 coneCenter = lightPos + spotDir * lightRadius;

	// Create AABB from frustum corners for quick rejection test
	glm::vec3 frustumMin = frustumCorners[0];
	glm::vec3 frustumMax = frustumCorners[0];
	for (const auto& corner : frustumCorners) {
		frustumMin = glm::min(frustumMin, corner);
		frustumMax = glm::max(frustumMax, corner);
	}
	frustumMin -= glm::vec3(0.1f);
	frustumMax += glm::vec3(0.1f);

	// Test cone apex
	if (lightPos.x >= frustumMin.x && lightPos.x <= frustumMax.x &&
		lightPos.y >= frustumMin.y && lightPos.y <= frustumMax.y &&
		lightPos.z >= frustumMin.z && lightPos.z <= frustumMax.z) {
		return true;
	}

	// Test points around cone circumference
	for (int i = 0; i < numSamples; ++i) {
		float angle = (2.0f * static_cast<float>(M_PI) * static_cast<float>(i)) / static_cast<float>(numSamples);
		glm::vec3 offset = right * (coneRadius * std::cos(angle)) + up * (coneRadius * std::sin(angle));
		glm::vec3 conePoint = coneCenter + offset;

		if (conePoint.x >= frustumMin.x && conePoint.x <= frustumMax.x &&
			conePoint.y >= frustumMin.y && conePoint.y <= frustumMax.y &&
			conePoint.z >= frustumMin.z && conePoint.z <= frustumMax.z) {
			return true;
		}
	}

	return false;
}

/**
 * @brief Calculates the shadow matrix for a spotlight.
 * Computes a view and orthographic projection matrix that tightly fits the cascade frustum in light space.
 * Applies texel snapping and margin adjustments for stable shadows.
 * @param lightPos Position of the spotlight.
 * @param spotDir Direction vector of the spotlight.
 * @param outerAngleRad Outer angle of the spotlight cone in radians.
 * @param lightRadius Maximum range of the spotlight.
*/
glm::mat4 Renderer::calculateSpotlightShadowMatrix(const glm::vec3& lightPos,
	const glm::vec3& spotDir,
	float outerAngleRad,
	float lightRadius) {

	// View matrix
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	if (glm::abs(glm::dot(spotDir, up)) > 0.999f)
		up = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::mat4 view = glm::lookAt(lightPos, lightPos + spotDir, up);

	// Perspective projection matching cone
	float fov = 2.0f * outerAngleRad;
	float nearPlane = 0.1f;
	float farPlane = lightRadius;
	glm::mat4 proj = glm::perspective(fov, 1.0f, nearPlane, farPlane);

	return proj * view;
}

/**
 * @brief Calculates light-space matrices for all shadow-casting lights.
 * Computes cascade splits and shadow matrices for directional and spot lights based on the camera's view and projection.
 * Updates each light's shadow matrix and split depth for use in shadow mapping.
 * @param editorCamera Reference to the editor camera providing view and projection matrices.
 */
void Renderer::CalculateLightMatrix(const editor::EditorCamera& editorCamera)
{
	// Convert camera projection/view to glm
	const Mtx44 proj = editorCamera.GetProjectionMatrix();
	const Mtx44 view = editorCamera.GetViewMatrix();

	glm::mat4 glmProj = glm::mat4(
		proj.m00, proj.m01, proj.m02, proj.m03,
		proj.m10, proj.m11, proj.m12, proj.m13,
		proj.m20, proj.m21, proj.m22, proj.m23,
		proj.m30, proj.m31, proj.m32, proj.m33
	);
	glm::mat4 glmView = glm::mat4(
		view.m00, view.m01, view.m02, view.m03,
		view.m10, view.m11, view.m12, view.m13,
		view.m20, view.m21, view.m22, view.m23,
		view.m30, view.m31, view.m32, view.m33
	);

	// Inverse PV used for unprojecting NDC to world
	glm::mat4 invPV = glm::inverse(glmProj * glmView);

	auto UnprojectNDC = [&](float ndcX, float ndcY, float ndcZ) -> glm::vec3 {
		glm::vec4 ndc(ndcX, ndcY, ndcZ, 1.0f);
		glm::vec4 world = invPV * ndc;
		if (world.w != 0.0f) world /= world.w;
		return glm::vec3(world);
		};

	// Derive near and far world positions along the view center ray
	glm::vec3 nearPos = UnprojectNDC(0.0f, 0.0f, -1.0f);
	glm::vec3 farPos = UnprojectNDC(0.0f, 0.0f, 1.0f);

	// Compute view-space distances for robust split calculation
	glm::vec4 nearPosView4 = glmView * glm::vec4(nearPos, 1.0f);
	glm::vec4 farPosView4 = glmView * glm::vec4(farPos, 1.0f);
	float nearDist = -nearPosView4.z; // positive distance from camera along view dir
	float farDist = -farPosView4.z;

	if (nearDist <= 1e-6f || farDist <= nearDist) {
		EE_CORE_WARN("calculatedirectionalmatrix: invalid camera near/far ({0},{1})", nearDist, farDist);
		return;
	}

	// Direction along camera center ray (world space)
	glm::vec3 viewDir = glm::normalize(farPos - nearPos);

	const auto& ecs = Ermine::ECS::GetInstance();
	unsigned int currentLayer = 0;

	for (auto e : m_LightSystem->m_Entities) {
		if (!ecs.HasComponent<Light>(e) || !ecs.HasComponent<Transform>(e)) continue;
		auto& light = ecs.GetComponent<Light>(e);
		if (light.castsShadows == 0) continue;

		// Get transform data
		const auto& trans = ecs.GetComponent<Transform>(e);
		glm::quat rotQuat = glm::normalize(glm::quat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z));
		glm::vec3 lightPos = glm::vec3(trans.position.x, trans.position.y, trans.position.z);

		if (light.type == LightType::DIRECTIONAL) {
			// DIRECTIONAL LIGHT PROCESSING (existing code)
			if (currentLayer + NUM_CASCADES > SHADOW_MAX_LAYERS) {
				EE_CORE_WARN("Not enough layers in shadow map array for directional light entity {0}. Skipping.", e);
				continue;
			}

			light.startOffset = currentLayer;

			// Get light direction
			glm::vec3 fwd = glm::normalize(rotQuat * glm::vec3(0.0f, 0.0f, 1.0f));
			glm::vec3 lightDir = glm::normalize(-fwd); // from scene to light

			// Setup up vector
			glm::vec3 up(0.0f, 1.0f, 0.0f);
			if (glm::abs(glm::dot(up, lightDir)) > 0.999f)
				up = glm::vec3(1.0f, 0.0f, 0.0f);

			// Compute cascade splits
			std::vector<float> splits(NUM_CASCADES + 1);
			for (int i = 0; i <= NUM_CASCADES; ++i) {
				float si = static_cast<float>(i) / static_cast<float>(NUM_CASCADES);
				float logSplit = nearDist * std::pow(farDist / nearDist, si);
				float linSplit = nearDist + (farDist - nearDist) * si;
				splits[i] = SHADOW_MAP_ARRAY_LAMBDA * logSplit + (1.0f - SHADOW_MAP_ARRAY_LAMBDA) * linSplit;
			}

			// Get shadow map resolution
			int shadowRes = 1024;
			if (m_ShadowMapArray != 0) {
				glBindTexture(GL_TEXTURE_2D_ARRAY, m_ShadowMapArray);
				GLint w = 0;
				glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_WIDTH, &w);
				if (w > 0) shadowRes = w;
				glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
				glCheckError();
			}

			// Process each cascade
			for (int split = 0; split < NUM_CASCADES; ++split) {
				float splitNearDist = splits[split];
				float splitFarDist = splits[split + 1];

				// Compute world positions for cascade near/far
				glm::vec3 splitNearWorld = nearPos + viewDir * (splitNearDist - nearDist);
				glm::vec3 splitFarWorld = nearPos + viewDir * (splitFarDist - nearDist);

				// Compute depth for depth buffer
				glm::vec4 clipFar = glmProj * glmView * glm::vec4(splitFarWorld, 1.0f);
				float ndcZ_splitFar = (clipFar.w == 0.0f) ? 1.0f : (clipFar.z / clipFar.w);
				float depthBufferFar = ndcZ_splitFar * 0.5f + 0.5f;

				// Build frustum corners
				std::array<glm::vec3, 8> frustumCornersWorld;
				const float xs[2] = { -1.0f, 1.0f };
				const float ys[2] = { -1.0f, 1.0f };
				float ndcZ_splitNear = (split == 0) ? -1.0f : (glmProj * glmView * glm::vec4(splitNearWorld, 1.0f)).z / (glmProj * glmView * glm::vec4(splitNearWorld, 1.0f)).w;

				int idx = 0;
				for (int iz = 0; iz < 2; ++iz) {
					float zndc = (iz == 0) ? ndcZ_splitNear : ndcZ_splitFar;
					for (int iy = 0; iy < 2; ++iy)
						for (int ix = 0; ix < 2; ++ix)
							frustumCornersWorld[idx++] = UnprojectNDC(xs[ix], ys[iy], zndc);
				}

				// Get AABB and center
				glm::vec3 minCorner = frustumCornersWorld[0];
				glm::vec3 maxCorner = frustumCornersWorld[0];
				for (const auto& c : frustumCornersWorld) {
					minCorner = glm::min(minCorner, c);
					maxCorner = glm::max(maxCorner, c);
				}
				glm::vec3 worldCenter = (minCorner + maxCorner) * 0.5f;

				// Place light far enough away with safety margin
				float diagonal = glm::length(maxCorner - minCorner);
				float lightDistance = glm::max(diagonal * 3.0f, (splitFarDist - splitNearDist) * 2.0f);
				glm::vec3 lightPosCalc = worldCenter - lightDir * lightDistance;

				// Create light view matrix
				glm::mat4 lightView = glm::lookAt(lightPosCalc, worldCenter, up);

				// Transform frustum to light space
				std::vector<glm::vec3> cornersLS;
				cornersLS.reserve(8);
				for (const auto& c : frustumCornersWorld) {
					glm::vec4 p = lightView * glm::vec4(c, 1.0f);
					cornersLS.emplace_back(glm::vec3(p));
				}

				// 2D PCA for optimal shadow map orientation
				glm::vec2 centroid2D(0.0f);
				for (const auto& p : cornersLS)
					centroid2D += glm::vec2(p.x, p.y);
				centroid2D /= static_cast<float>(cornersLS.size());

				// Compute 2x2 covariance matrix
				float cov_xx = 0.0f, cov_xy = 0.0f, cov_yy = 0.0f;
				for (const auto& p : cornersLS) {
					glm::vec2 d = glm::vec2(p.x, p.y) - centroid2D;
					cov_xx += d.x * d.x;
					cov_xy += d.x * d.y;
					cov_yy += d.y * d.y;
				}
				cov_xx /= static_cast<float>(cornersLS.size());
				cov_xy /= static_cast<float>(cornersLS.size());
				cov_yy /= static_cast<float>(cornersLS.size());

				// Find principal axis
				float trace = cov_xx + cov_yy;
				float det = cov_xx * cov_yy - cov_xy * cov_xy;
				float lambda1 = 0.5f * (trace + std::sqrt(trace * trace - 4.0f * det));

				glm::vec2 eigenVec;
				if (std::abs(cov_xy) > 1e-6f) {
					eigenVec = glm::normalize(glm::vec2(lambda1 - cov_yy, cov_xy));
				}
				else {
					eigenVec = (cov_xx > cov_yy) ? glm::vec2(1.0f, 0.0f) : glm::vec2(0.0f, 1.0f);
				}

				// Rotate around Z-axis only
				float angle = std::atan2(eigenVec.y, eigenVec.x);
				glm::mat4 zRotation = glm::rotate(glm::mat4(1.0f), -angle, glm::vec3(0.0f, 0.0f, 1.0f));
				glm::mat4 rotatedLightView = zRotation * lightView;

				// Find bounds in rotated space
				glm::vec3 lsMin(FLT_MAX), lsMax(-FLT_MAX);
				for (const auto& c : frustumCornersWorld) {
					glm::vec4 p = rotatedLightView * glm::vec4(c, 1.0f);
					glm::vec3 pp = glm::vec3(p);
					lsMin = glm::min(lsMin, pp);
					lsMax = glm::max(lsMax, pp);
				}

				// Add margins to prevent clipping
				const float xyMargin = 0.1f;
				const float zMargin = 0.5f;
				lsMin -= glm::vec3(xyMargin, xyMargin, zMargin);
				lsMax += glm::vec3(xyMargin, xyMargin, zMargin);

				// Compute ortho extents
				float nearPlane = -lsMax.z;
				float farPlane = -lsMin.z;
				glm::mat4 lightProj = glm::ortho(lsMin.x, lsMax.x, lsMin.y, lsMax.y, nearPlane, farPlane);

				// Store final matrix and split depth
				light.lightSpaceMatrices[split] = lightProj * rotatedLightView;
				light.splitDepths[split] = splitFarDist;
			}

			currentLayer += NUM_CASCADES;
		}
		else if (light.type == LightType::SPOT) {
			// Check if we have a shadow map layer available
			if (currentLayer >= SHADOW_MAX_LAYERS) {
				continue;
			}

			// Get spotlight direction and parameters
			glm::vec3 spotDir = glm::normalize(rotQuat * glm::vec3(0.0f, 0.0f, 1.0f));
			float outerAngleRad = glm::radians(light.outerAngle);

			// Calculate single shadow matrix for the spotlight
			light.lightSpaceMatrices[0] = calculateSpotlightShadowMatrix(
				lightPos, spotDir, outerAngleRad, light.radius);

			// Assign shadow map layer
			light.startOffset = currentLayer;
			currentLayer += 1;
		}
	}

	// Store total layers for instanced shadow rendering
	m_TotalShadowLayers = currentLayer;
}

/**
 * @brief Renders shadow map using indirect rendering and instancing across all shadow layers.
 *
 * Uses indirect rendering (glMultiDrawElementsIndirect) to render all meshes with a single draw call.
 * Reads pre-skinned positions from geometry pass (binding 8) to eliminate redundant bone calculations.
 * Each mesh is instanced across all shadow-casting light cascades for optimal performance.
 *
 * @note Requires geometry pass to have completed and written pre-skinned positions to SSBO.
 * @note Memory barrier is issued in RenderShadowPass() before calling this function.
 */
void Renderer::RenderShadowMapInstanced()
{
	UpdateLightsUBO(editor::EditorCamera::GetInstance().GetViewMatrix());

	// Validate resources
	if (!m_ShadowMapFBO || !m_ShadowMapArray || !m_ShadowMapInstancedShader)
	{
		EE_CORE_WARN("RenderShadowMapInstanced: missing shadow FBO/texture/shader");
		return;
	}

	// Query current viewport so we can restore it later
	GLint prevViewport[4];
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	// Query shadow map resolution from the texture
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_ShadowMapArray);
	GLint shadowWidth = 1024, shadowHeight = 1024; // fallback
	glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_WIDTH, &shadowWidth);
	glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_HEIGHT, &shadowHeight);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	// Bind shadow FBO and set viewport to shadow resolution
	glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMapFBO);
	glViewport(0, 0, shadowWidth, shadowHeight);

	// Clear depth
	glClearDepth(1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);

	// Setup render state for depth-only pass
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Bind shadow shader
	m_ShadowMapInstancedShader->Bind();

	// Gather shadow casting lights (both directional and spotlight)
	const auto& ecs = Ermine::ECS::GetInstance();
	std::vector<int> activeShadowLights;
	int lightIndex = 0;

	for (EntityID entity : m_LightSystem->m_Entities)
	{
		if (!ecs.HasComponent<Light>(entity)) continue;
		auto& light = ecs.GetComponent<Light>(entity);
		// Include both directional and spotlight shadow casters
		if ((light.type == LightType::DIRECTIONAL || light.type == LightType::SPOT) && light.castsShadows) {
			activeShadowLights.push_back(lightIndex);
		}
		lightIndex++;
	}

	// Use only shadowcaster lights for instancing
	unsigned int maxLights = std::min(static_cast<unsigned int>(activeShadowLights.size()), static_cast<unsigned int>(MAX_LIGHTS));
	int totalInstances = maxLights * NUM_CASCADES;

	// Set up per-frame uniforms
	for (unsigned int i = 0; i < maxLights; ++i) {
		std::string uniformName = "u_ActiveShadowLights[" + std::to_string(i) + "]";
		m_ShadowMapInstancedShader->SetUniform1i(uniformName, activeShadowLights[i]);
	}

	// Prepare draw commands with instanceCount = total shadow layers
	size_t totalDrawCount = m_StandardDrawCommands.size() + m_SkinnedDrawCommands.size();
	std::vector<DrawElementsIndirectCommand> shadowCommands;
	shadowCommands.reserve(totalDrawCount);

	// Copy standard commands and set instanceCount
	for (const auto& cmd : m_StandardDrawCommands) {
		shadowCommands.push_back(cmd);
		shadowCommands.back().instanceCount = totalInstances;
	}

	// Copy skinned commands and set instanceCount
	for (const auto& cmd : m_SkinnedDrawCommands) {
		shadowCommands.push_back(cmd);
		shadowCommands.back().instanceCount = totalInstances;
	}

	// Upload combined commands to SSBO
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_MeshManager.m_DrawCommandsSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, totalDrawCount * sizeof(DrawElementsIndirectCommand),
		shadowCommands.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Upload DrawInfo data (reuse from geometry pass - already contains both standard and skinned)
	m_MeshManager.m_PersistentDrawInfoBuffer.WriteDrawInfos(m_StandardDrawInfos, 0);
	m_MeshManager.m_PersistentDrawInfoBuffer.WriteDrawInfos(m_SkinnedDrawInfos, m_StandardDrawCommands.size());

	// Render standard meshes using indirect rendering with shadow VAO
	// Shadow VAO includes pre-skinned position attribute (location 6) for hardware vertex fetching
	if (!m_StandardDrawCommands.empty() && m_MeshManager.GetStandardShadowVAO() != 0) {
		m_ShadowMapInstancedShader->SetUniform1ui("baseDrawID", 0);
		glBindVertexArray(m_MeshManager.GetStandardShadowVAO());
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_MeshManager.m_DrawCommandsSSBO);
		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
			static_cast<GLsizei>(m_StandardDrawCommands.size()), 0);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
		glBindVertexArray(0);
	}

	// Render skinned meshes using indirect rendering with shadow VAO
	// Shadow VAO includes pre-skinned position attribute (location 6) for hardware vertex fetching
	if (!m_SkinnedDrawCommands.empty() && m_MeshManager.GetSkinnedShadowVAO() != 0) {
		m_ShadowMapInstancedShader->SetUniform1ui("baseDrawID", static_cast<uint32_t>(m_StandardDrawCommands.size()));
		glBindVertexArray(m_MeshManager.GetSkinnedShadowVAO());
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_MeshManager.m_DrawCommandsSSBO);
		size_t offset = m_StandardDrawCommands.size() * sizeof(DrawElementsIndirectCommand);
		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, reinterpret_cast<const void*>(offset),
			static_cast<GLsizei>(m_SkinnedDrawCommands.size()), 0);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
		glBindVertexArray(0);
	}

	// Unbind framebuffer and restore viewport
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

	glCheckError();
}

/**
 * @brief Executes the full shadow pass for all shadow-casting lights.
 * Calculates cascade splits and shadow matrices for directional and spot lights based on the camera's view and projection.
 * Updates each light's shadow matrix and split depth for use in shadow mapping.
 */
void Renderer::RenderShadowPass()
{
	// Calculate directional light matrices
	CalculateLightMatrix(editor::EditorCamera::GetInstance());

	// Memory barrier to ensure pre-skinned positions from geometry pass are visible to shadow pass
	// The geometry pass writes to preSkinnedPositions buffer (binding 8) which shadow pass can read
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// Render shadows using instanced rendering
	RenderShadowMapInstanced();
}

#pragma endregion

/**
 * @brief Creates the picking buffer for entity selection.
 * @param width Buffer width.
 * @param height Buffer height.
 */
void Renderer::CreatePickingBuffer(const int& width, const int& height)
{
	if (m_PickingBuffer)
	{
		glDeleteFramebuffers(1, &m_PickingBuffer->FBO);
		glDeleteTextures(1, &m_PickingBuffer->ColorID);
		glDeleteRenderbuffers(1, &m_PickingBuffer->Depth);
		m_PickingBuffer.reset();
	}

	auto pb = std::make_shared<PickingBuffer>();
	pb->width = width;
	pb->height = height;

	glGenFramebuffers(1, &pb->FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, pb->FBO);

	// Color: 32-bit unsigned int
	glGenTextures(1, &pb->ColorID);
	glBindTexture(GL_TEXTURE_2D, pb->ColorID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pb->ColorID, 0);

	// Depth
	glGenRenderbuffers(1, &pb->Depth);
	glBindRenderbuffer(GL_RENDERBUFFER, pb->Depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pb->Depth);

	GLenum db[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, db);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		EE_CORE_ERROR("Picking FBO incomplete: {0}", status);
		assert(false && "Picking FBO failed");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	m_PickingBuffer = pb;
	glCheckError();
}

/**
 * @brief Resizes the picking buffer.
 * @param width New width.
 * @param height New height.
 */
void Renderer::ResizePickingBuffer(const int& width, const int& height)
{
	if (!m_PickingBuffer)
	{
		CreatePickingBuffer(width, height);
		return;
	}
	if (m_PickingBuffer->width == width && m_PickingBuffer->height == height)
		return;

	glBindTexture(GL_TEXTURE_2D, m_PickingBuffer->ColorID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);

	glBindRenderbuffer(GL_RENDERBUFFER, m_PickingBuffer->Depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, m_PickingBuffer->FBO);
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		EE_CORE_ERROR("Picking FBO not complete after resize! Status: {0}", status);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_PickingBuffer->width = width;
	m_PickingBuffer->height = height;
	glCheckError();
}

/**
 * @brief Renders the picking pass for entity selection.
 * @param view View matrix.
 * @param projection Projection matrix.
 */
void Renderer::RenderPickingPass(const Mtx44& view, const Mtx44& projection)
{
#if defined(EE_EDITOR)
	if (!m_PickingBuffer || !m_PickingShader)
		return;

	// 1) Prime depth: copy scene depth into picking FBO (source depends on path)
	if (m_UseDeferredRendering && m_GBuffer)
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_GBuffer->FBO);
	}
	else if (m_OffscreenBuffer)
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_OffscreenBuffer->FBO);
	}
	else
	{
		return;
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_PickingBuffer->FBO);
	glBlitFramebuffer(0, 0, m_PickingBuffer->width, m_PickingBuffer->height,
		0, 0, m_PickingBuffer->width, m_PickingBuffer->height,
		GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	// 2) Render IDs
	glBindFramebuffer(GL_FRAMEBUFFER, m_PickingBuffer->FBO);
	glViewport(0, 0, m_PickingBuffer->width, m_PickingBuffer->height);

	// Clear IDs to 0
	GLuint clearVal[1] = { 0u };
	glClearBufferuiv(GL_COLOR, 0, clearVal);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glDisable(GL_BLEND);

	m_PickingShader->Bind();

	// Use same transforms as other passes
	glm::mat4 glmView = glm::mat4(
		view.m00, view.m01, view.m02, view.m03,
		view.m10, view.m11, view.m12, view.m13,
		view.m20, view.m21, view.m22, view.m23,
		view.m30, view.m31, view.m32, view.m33
	);
	glm::mat4 glmProjection = glm::mat4(
		projection.m00, projection.m01, projection.m02, projection.m03,
		projection.m10, projection.m11, projection.m12, projection.m13,
		projection.m20, projection.m21, projection.m22, projection.m23,
		projection.m30, projection.m31, projection.m32, projection.m33
	);

	glm::mat4 vp = glmProjection * glmView;
	// Shader uses u_LightViewProj + model, same as shadow/depth style
	m_PickingShader->SetUniformMatrix4fv("u_LightViewProj", vp);

	auto& ecs = ECS::GetInstance();

	// Model pipeline
	for (EntityID entity : m_Entities)
	{
		if (!ecs.HasComponent<ModelComponent>(entity))
			continue;

		auto& modelComp = ecs.GetComponent<ModelComponent>(entity);
		if (!modelComp.m_model) continue;

		auto& trans = ecs.GetComponent<Transform>(entity);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
		glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
		rotQuat = glm::normalize(rotQuat);
		model *= glm::mat4_cast(rotQuat);
		model = glm::scale(model, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

		// Encode as (EntityID + 1) so 0 stays as "no hit"
		uint32_t encoded = static_cast<uint32_t>(entity) + 1u;
		m_PickingShader->SetUniform1ui("u_EntityId", encoded);

		const auto& meshes = modelComp.m_model->GetMeshes();
		for (const auto& mesh : meshes)
		{
			if (!mesh.vao || !mesh.ibo) continue;
			glm::mat4 modelMat = model * mesh.localTransform;
			m_PickingShader->SetUniformMatrix4fv("model", modelMat);
			Draw(mesh.vao, mesh.ibo);
		}
	}

	// Mesh + material pipeline
	for (EntityID entity : m_Entities)
	{
		if (!(ecs.HasComponent<Mesh>(entity) && ecs.HasComponent<Ermine::Material>(entity)))
			continue;

		auto& mesh = ecs.GetComponent<Mesh>(entity);
		if (!mesh.vertex_array || !mesh.index_buffer) continue;

		auto& trans = ecs.GetComponent<Transform>(entity);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
		glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
		rotQuat = glm::normalize(rotQuat);
		model *= glm::mat4_cast(rotQuat);
		model = glm::scale(model, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

		uint32_t encoded = static_cast<uint32_t>(entity) + 1u;
		m_PickingShader->SetUniform1ui("u_EntityId", encoded);
		m_PickingShader->SetUniformMatrix4fv("model", model);

		Draw(mesh.vertex_array, mesh.index_buffer);
	}

	// Restore
	m_PickingShader->Unbind();
	glDepthMask(GL_TRUE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glCheckError();
#endif
}

/**
 * @brief Gets the world transform matrix for an entity using GlobalTransform component
 * @param entity The entity to get the world matrix for
 * @return glm::mat4 The world transform matrix
 */
glm::mat4 Renderer::GetEntityWorldMatrix(EntityID entity) const
{
	const auto& ecs = Ermine::ECS::GetInstance();

	if (ecs.HasComponent<GlobalTransform>(entity)) {
		auto& globalTransform = ecs.GetComponent<GlobalTransform>(entity);

		// CONVERT your custom matrix to GLM format
		Mtx44& m = globalTransform.worldMatrix;
		return glm::mat4(
			m.m00, m.m10, m.m20, m.m30,  // Column 0
			m.m01, m.m11, m.m21, m.m31,  // Column 1  
			m.m02, m.m12, m.m22, m.m32,  // Column 2
			m.m03, m.m13, m.m23, m.m33   // Column 3
		);
	}

	// Fallback to local transform using GLM
	if (ecs.HasComponent<Transform>(entity)) {
		auto& trans = ecs.GetComponent<Transform>(entity);
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
		glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
		rotQuat = glm::normalize(rotQuat);
		model *= glm::mat4_cast(rotQuat);
		model = glm::scale(model, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));
		return model;
	}

	return glm::mat4(1.0f);
}

/**
 * @brief Picks the entity at the given screen coordinates.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param view View matrix.
 * @param projection Projection matrix.
 * @return Pair of (success, entity ID).
 */
std::pair<bool, Ermine::EntityID> Renderer::PickEntityAt(const int& x, const int& y, const Mtx44& view, const Mtx44& projection)
{

#if defined(EE_EDITOR)
	if (!m_PickingBuffer)
		return { false, EntityID{} };

	RenderPickingPass(view, projection);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_PickingBuffer->FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0); // color read buffer doesn't matter since we only read stencil
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	uint32_t id = 0u;
	glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &id);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	if (id == 0u) return { false, EntityID{} };

	EntityID picked = id - 1u;
	if (!ECS::GetInstance().IsEntityValid(picked))
		return { false , EntityID{} };

	return { true, picked };
#else
	return { false, EntityID{} };
#endif
}

/**
 * @brief Handle window resize events to adjust buffers and viewports
 * @param width New window width
 * @param height New window height
 */
void Renderer::OnWindowResize(const int& width, const int& height)
{
	if (m_UseDeferredRendering)
		ResizeGBuffer(width, height);
}

/**
 * @brief Compiles all materials from entities into a single SSBO.
 * This collects material data, uploads to GPU, and assigns indices.
 */
void Renderer::CompileMaterials()
{
	if (!m_MaterialsDirty) return;

	EE_CORE_INFO("Compiling materials for GPU upload...");

	// Clear previous compiled data
	m_CompiledMaterials.clear();
	m_EntityMaterialIndices.clear();

	// Map to track unique materials and avoid duplicates
	std::map<const graphics::Material*, uint32_t> materialToIndex;

	const auto& ecs = Ermine::ECS::GetInstance();

	// First pass: Collect unique materials
	for (auto entity : m_Entities)
	{
		if (!ecs.HasComponent<Ermine::Material>(entity)) continue;

		auto& materialComponent = ecs.GetComponent<Ermine::Material>(entity);
		graphics::Material* material = materialComponent.GetMaterial();

		if (!material) {
			EE_CORE_WARN("Entity {0} has null material", entity);
			continue;
		}

		// Check if we've already seen this material
		if (materialToIndex.find(material) != materialToIndex.end()) {
			// Reuse existing index
			m_EntityMaterialIndices[entity] = materialToIndex[material];
			continue;
		}

		// New material - register textures and assign indices
		uint32_t materialIndex = static_cast<uint32_t>(m_CompiledMaterials.size());

		// Register all textures used by this material
		const std::vector<std::string> textureTypes = {
			"materialAlbedoMap",
			"materialNormalMap",
			"materialRoughnessMap",
			"materialMetallicMap",
			"materialAoMap",
			"materialEmissiveMap"
		};

		for (const auto& texName : textureTypes)
		{
			if (auto texture = material->GetTexture(texName))
			{
				int textureIndex = RegisterTexture(texture);
				if (textureIndex >= 0)
				{
					material->SetTextureArrayIndex(texName, textureIndex);
				}
			}
		}

		// Add material data to compiled list
		m_CompiledMaterials.push_back(material->GetSSBOData());

		// Store the mapping
		materialToIndex[material] = materialIndex;
		m_EntityMaterialIndices[entity] = materialIndex;

		// Update the material with its index
		material->SetMaterialIndex(static_cast<int>(materialIndex));
	}

	// Build the texture array
	BuildTextureArray();

	// Upload all materials to GPU
	UploadMaterialsToGPU();

	m_MaterialsDirty = false;

	EE_CORE_INFO("Compiled {0} unique materials for {1} entities",
		m_CompiledMaterials.size(), m_EntityMaterialIndices.size());
}


/**
 * @brief Uploads all compiled materials to the GPU SSBO in one batch.
 */
void Renderer::UploadMaterialsToGPU()
{
	if (m_CompiledMaterials.empty()) {
		EE_CORE_WARN("No materials to upload to GPU");
		return;
	}

	// Create Material SSBO if it doesn't exist
	if (!m_MaterialSSBO)
	{
		glGenBuffers(1, &m_MaterialSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_MaterialSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MaterialBindingPoint, m_MaterialSSBO);
		EE_CORE_INFO("Created MaterialSSBO");
	}
	else
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_MaterialSSBO);
	}

	// Calculate total size needed
	const size_t materialSize = sizeof(graphics::MaterialSSBO);
	const size_t totalSize = materialSize * m_CompiledMaterials.size();

	// Reallocate buffer to fit all materials
	glBufferData(GL_SHADER_STORAGE_BUFFER, totalSize, nullptr, GL_DYNAMIC_DRAW);

	// Check for allocation errors
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		EE_CORE_ERROR("Failed to allocate MaterialSSBO: {0} bytes, error: {1}",
			totalSize, error);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		return;
	}

	// Upload all materials at once
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, totalSize, m_CompiledMaterials.data());

	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		EE_CORE_ERROR("Failed to upload materials to GPU, error: {0}", error);
	}
	else
	{
		EE_CORE_INFO("Uploaded {0} materials ({1} bytes) to GPU",
			m_CompiledMaterials.size(), totalSize);
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glCheckError();
}

/**
 * @brief Registers a texture in the global texture array.
 * @param texture Shared pointer to the texture.
 * @return The index of the texture in the array, or -1 if registration failed.
 */
int Renderer::RegisterTexture(std::shared_ptr<Texture> texture)
{
	if (!texture || !texture->IsValid())
	{
		return -1;
	}

	GLuint textureID = texture->GetRendererID();
	std::string filePath = texture->GetFilePath();

	// Check if texture is already registered by ID
	auto idIt = m_TextureIDToIndex.find(textureID);
	if (idIt != m_TextureIDToIndex.end())
	{
		return idIt->second;
	}

	// Check if texture is already registered by path
	auto pathIt = m_TexturePathToIndex.find(filePath);
	if (pathIt != m_TexturePathToIndex.end())
	{
		return pathIt->second;
	}

	// Register new texture
	int index = static_cast<int>(m_TextureArray.size());
	m_TextureArray.push_back(textureID);
	m_TextureIDToIndex[textureID] = index;
	if (!filePath.empty())
	{
		m_TexturePathToIndex[filePath] = index;
	}

	m_TextureArrayDirty = true;

	EE_CORE_INFO("Registered texture '{0}' at index {1}", filePath, index);
	return index;
}

/**
 * @brief Gets the texture array index for a given texture ID.
 * @param textureID The OpenGL texture ID.
 * @return The array index, or -1 if not found.
 */
int Renderer::GetTextureArrayIndex(GLuint textureID) const
{
	auto it = m_TextureIDToIndex.find(textureID);
	return it != m_TextureIDToIndex.end() ? it->second : -1;
}

/**
 * @brief Builds the bindless texture array SSBO.
 * This should be called after all textures are registered and before rendering.
 */
void Renderer::BuildTextureArray()
{
	if (!m_TextureArrayDirty || m_TextureArray.empty())
	{
		return;
	}

	EE_CORE_INFO("Building bindless texture array with {0} textures...", m_TextureArray.size());

	// Create texture array SSBO if it doesn't exist
	if (!m_TextureArraySSBO)
	{
		glGenBuffers(1, &m_TextureArraySSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_TextureArraySSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, TextureArrayBindingPoint, m_TextureArraySSBO);
		EE_CORE_INFO("Created Texture Array SSBO at binding point {0}", TextureArrayBindingPoint);
	}
	else
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_TextureArraySSBO);
	}

	// Create bindless texture handles for all textures
	std::vector<GLuint64> textureHandles;
	textureHandles.reserve(m_TextureArray.size());

	for (GLuint textureID : m_TextureArray)
	{
		// Get bindless handle for this texture
		GLuint64 handle = glGetTextureHandleARB(textureID);
		if (handle == 0)
		{
			EE_CORE_ERROR("Failed to get bindless texture handle for texture ID {0}", textureID);
			textureHandles.push_back(0);
			continue;
		}

		// Make the handle resident (accessible in shaders)
		if (!glIsTextureHandleResidentARB(handle))
		{
			glMakeTextureHandleResidentARB(handle);
		}

		textureHandles.push_back(handle);
	}

	// Upload texture handles to SSBO
	const size_t totalSize = textureHandles.size() * sizeof(GLuint64);
	glBufferData(GL_SHADER_STORAGE_BUFFER, totalSize, textureHandles.data(), GL_STATIC_DRAW);

	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		EE_CORE_ERROR("Failed to upload texture array to GPU, error: {0}", error);
	}
	else
	{
		EE_CORE_INFO("Uploaded {0} texture handles ({1} bytes) to GPU",
			textureHandles.size(), totalSize);
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	m_TextureArrayDirty = false;
}