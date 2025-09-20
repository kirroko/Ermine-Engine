/* Start Header ************************************************************************/
/*!
\file       Renderer.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\co-author  Jeremy Lim Ting Jie, jeremytingjie.lim, 2301370, jeremytingjie.lim\@digipen.edu
\co-author  Ridhwan
\date       09/03/2025
\brief      This file contains the definition of the Renderer system.
			This file is used to render the game objects.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "Renderer.h"
#include "Material.h"

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

#include <GLFW/glfw3.h>

using namespace Ermine::graphics;


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

void Renderer::Init(const int& screenWidth, const int& screenHeight)
{
	// Check for ARB_bindless_texture support
	if (!glfwExtensionSupported("GL_ARB_bindless_texture") || !GL_ARB_bindless_texture)
	{
		EE_CORE_WARN("GL_ARB_bindless_texture not supported. Deferred rendering will be disabled.");
		m_UseDeferredRendering = false;
	}
	// Create a fullscreen quad for rendering the offscreen buffer to the screen


	// Add light system reference
	m_LightSystem = Ermine::ECS::GetInstance().GetSystem<LightSystem>();

	m_QuadMesh = GeometryFactory::CreateQuad(2.0f, 2.0f);

	// Load deferred shading shaders
	m_GBufferShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/gBuffer_vertex.glsl", "../Resources/Shaders/gBuffer_fragment.glsl");
	m_LightPassShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/lighting_vertex.glsl", "../Resources/Shaders/lighting_fragment.glsl");
	m_BloomShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/bloom_vertex.glsl", "../Resources/Shaders/bloom_fragment.glsl");
	m_PostProcessShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/postprocess_vertex.glsl", "../Resources/Shaders/postprocess_fragment.glsl");
	m_ShadowMapShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/shadowmap_vertex.glsl", "../Resources/Shaders/shadowmap_geometry.glsl", "../Resources/Shaders/shadowmap_fragment.glsl");
	// Create initial g-buffer
	CreateGBuffer(screenWidth, screenHeight);
	CreatePostProcessBuffer(screenWidth, screenHeight);

	// Create shadow map FBO and texture
	InitializeShadowMap();
	for (EntityID entity : m_LightSystem->m_Entities)
	{
		auto& light = Ermine::ECS::GetInstance().GetComponent<Light>(entity);
		if (light.castsShadows)
			CreateShadowMap(light.resolution);
	}

	tempTexture = AssetManager::GetInstance().LoadTexture("../Resources/Textures/greybox_grey_grid.png");
}

//Renderer::~Renderer()
//{
//	//if (m_OffscreenBuffer)
//	//{
//	//	glDeleteFramebuffers(1, &m_OffscreenBuffer->FBO);
//	//	glDeleteTextures(1, &m_OffscreenBuffer->ColorTexture);
//	//	glDeleteRenderbuffers(1, &m_OffscreenBuffer->RBO);
//	//}
//}

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

	// If Light UBO doesn't exist, create it
	if (!m_LightsUBO)
	{
		glGenBuffers(1, &m_LightsUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, m_LightsUBO);
		const GLsizeiptr headerSize = static_cast<GLsizeiptr>(sizeof(glm::vec4));
		const GLsizeiptr bodySize = static_cast<GLsizeiptr>(MaxLights * sizeof(LightGPU));
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
}

/**
 * @brief Create optimized g-buffer for deferred rendering using scalar materials and emissive
 * RT0: RGB16F (48 bits) - Albedo RGB
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
		const GLsizeiptr bodySize = static_cast<GLsizeiptr>(MaxLights * sizeof(LightGPU));
		glBufferData(GL_UNIFORM_BUFFER, headerSize + bodySize, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, LightsBindingPoint, m_LightsUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glCheckError();
	}

	// Create framebuffer
	glGenFramebuffers(1, &gBuffer.FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.FBO);

	// Create RT0 Texture: RGB16F (48 bits) - Albedo RGB
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
	EE_CORE_INFO("Created G-Buffer: {0}x{1}, 160 bits per pixel", width, height);
}

/**
 * @brief Create an offscreen buffer for viewport/scene rendering
 * @param width The width of the offscreen buffer
 * @param height The height of the offscreen buffer
 * @return OffscreenBuffer The offscreen buffer
 */
void Renderer::CreatePostProcessBuffer(const int& width, const int& height)
{
	PostProcessBuffer pPBuffer, bEBuffer, bBBuffer1, bBBuffer2;


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
}

/**
 * @brief Resize the g-buffer to new dimensions
 */
void Renderer::ResizeGBuffer(const int& width, const int& height)
{
	if (m_GBuffer && m_GBuffer->width == width && m_GBuffer->height == height)
		return; 

	CreateGBuffer(width, height);
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
 * @brief Render geometry pass for deferred rendering
 * CPU sets up the rendering state, GPU does the actual g-buffer writing
 */
void Renderer::RenderGeometryPass(const Mtx44& view, const Mtx44& projection)
{
	if (!m_GBuffer || !m_GBufferShader)
	{
		EE_CORE_ERROR("G-Buffer or geometry shader not initialized!");
		return;
	}

	// Begin geometry pass
	BeginGeometryPass();


	// Bind geometry shader that writes to g-buffer
	m_GBufferShader->Bind();

	// Bind material blocks
	BindMaterialBlockIfPresent(m_GBufferShader);

	// Gather all renderable entities and draw them
	const auto& ecs = Ermine::ECS::GetInstance();
	const unsigned long int maxId = ecs.GetLivingEntityCount();

	for (auto& entity:m_Entities)
	{
		// Model pipeline
		if (ecs.HasComponent<ModelComponent>(entity))
		{
			auto& modelComp = ecs.GetComponent<ModelComponent>(entity);
			auto& trans = ecs.GetComponent<Transform>(entity);
			auto& materialComponent = ecs.GetComponent<Ermine::Material>(entity);

			if (modelComp.m_model)
			{
				Ermine::graphics::Material* material = materialComponent.GetMaterial();

				if (!material) {
					EE_CORE_WARN("Entity {0} has null material", entity);
					continue;
				}
				UpdateMaterialUBO(material->GetUBOData());

				// Apply entity's transform as root
				glm::mat4 entityModel = glm::mat4(1.0f);
				entityModel = glm::translate(entityModel, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
				glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
				rotQuat = glm::normalize(rotQuat);
				entityModel *= glm::mat4_cast(rotQuat);
				entityModel = glm::scale(entityModel, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

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
				texUnit = 6;
				if (material->HasParameter("materialEnvironmentMap")) {
					std::shared_ptr<Texture> env = material->GetParameter("materialEnvironmentMap")->texture;
					if (env && env->IsValid()) {
						env->Bind(texUnit);
						m_GBufferShader->SetUniform1i("materialEnvironmentMap", texUnit);
					}
				}
				texUnit = 7;
				if (material->HasParameter("materialIrradianceMap")) {
					std::shared_ptr<Texture> irradiance = material->GetParameter("materialIrradianceMap")->texture;
					if (irradiance && irradiance->IsValid()) {
						irradiance->Bind(texUnit);
						m_GBufferShader->SetUniform1i("materialIrradianceMap", texUnit);
					}
				}



				// Render model
				RenderModel(*modelComp.m_model, view, projection, entityModel);
			}
		}
		// Mesh + material pipeline
		else if (ecs.HasComponent<Mesh>(entity) && ecs.HasComponent<Ermine::Material>(entity))
		{
			auto& trans = ecs.GetComponent<Transform>(entity);
			auto& mesh = ecs.GetComponent<Mesh>(entity);
			auto& materialComponent = ecs.GetComponent<Ermine::Material>(entity);

			if (!mesh.vertex_array || !mesh.index_buffer) continue;

			// Get the modular material
			Ermine::graphics::Material* material = materialComponent.GetMaterial();

			if (!material) {
				EE_CORE_WARN("Entity {0} has null material", entity);
				continue;
			}

			// Build model matrix
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
			glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
			rotQuat = glm::normalize(rotQuat);
			model *= glm::mat4_cast(rotQuat);
			model = glm::scale(model, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

			// Set transformation matrices for g-buffer shader
			m_GBufferShader->SetUniformMatrix4fv("model", model);
			m_GBufferShader->SetUniformMatrix4fv("view", &view.m2[0][0]);
			m_GBufferShader->SetUniformMatrix4fv("projection", &projection.m2[0][0]);

			// Calculate and set normal matrix
			glm::mat4 glmView = glm::mat4(
				view.m00, view.m01, view.m02, view.m03,
				view.m10, view.m11, view.m12, view.m13,
				view.m20, view.m21, view.m22, view.m23,
				view.m30, view.m31, view.m32, view.m33
			);
			glm::mat4 modelView = glmView * model;
			glm::mat3 normalMatrix = transpose(inverse(glm::mat3(model)));
			m_GBufferShader->SetUniformMatrix3fv("NormalMatrix", normalMatrix);

			UpdateMaterialUBO(material->GetUBOData());

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
			texUnit = 6;
			if (material->HasParameter("materialEnvironmentMap")) {
				std::shared_ptr<Texture> env = material->GetParameter("materialEnvironmentMap")->texture;
				if (env && env->IsValid()) {
					env->Bind(texUnit);
					m_GBufferShader->SetUniform1i("materialEnvironmentMap", texUnit);
				}
			}
			texUnit = 7;
			if (material->HasParameter("materialIrradianceMap")) {
				std::shared_ptr<Texture> irradiance = material->GetParameter("materialIrradianceMap")->texture;
				if (irradiance && irradiance->IsValid()) {
					irradiance->Bind(texUnit);
					m_GBufferShader->SetUniform1i("materialIrradianceMap", texUnit);
				}
			}

			// Draw the mesh
			Draw(mesh.vertex_array, mesh.index_buffer, m_GBufferShader);
		}
	}

	EndGeometryPass();
}

/**
 * @brief Render lighting pass for deferred rendering
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
	m_LightPassShader->SetUniformMatrix4fv("lightSpaceMatrix", m_LightSpaceMatrix);
	m_LightPassShader->SetUniform1i("u_VBAO", m_SSAOEnabled ? 1 : 0);

	// Set shading mode
	m_LightPassShader->SetUniform1i("u_ShadingMode", m_IsBlinnPhong ? 1 : 0);

	// Update and bind lights UBO
	UpdateLightsUBO(view);
	BindLightsBlockIfPresent(m_LightPassShader);

	// Render fullscreen quad 
	if (m_QuadMesh.vertex_array && m_QuadMesh.index_buffer)
	{
		Draw(m_QuadMesh.vertex_array, m_QuadMesh.index_buffer, m_LightPassShader);
	}

	EndLightingPass();
}

/**
 * @brief Render post-processing effects using the lighting pass output
 */
void Renderer::RenderPostProcessPass()
{
	if (!m_PostProcessBuffer || !m_BloomShader || !m_PostProcessShader)
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
	m_BloomShader->SetUniform1f("u_BloomIntensity", m_BloomIntensity);
	m_BloomShader->SetUniform1f("u_BloomRadius", m_BloomRadius);

	Draw(m_QuadMesh.vertex_array, m_QuadMesh.index_buffer, m_BloomShader);

	// Pass 2: Horizontal blur
	glBindFramebuffer(GL_FRAMEBUFFER, m_BloomBlurBuffer1->FBO);
	m_BloomShader->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_BloomExtractBuffer->ColorTexture);
	m_BloomShader->SetUniform1i("u_Pass", 2);
	Draw(m_QuadMesh.vertex_array, m_QuadMesh.index_buffer, m_BloomShader);

	// Pass 3: Vertical blur
	glBindFramebuffer(GL_FRAMEBUFFER, m_BloomBlurBuffer2->FBO);
	m_BloomShader->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_BloomBlurBuffer1->ColorTexture);
	m_BloomShader->SetUniform1i("u_LightingTexture", 0);
	m_BloomShader->SetUniform1i("u_Pass", 3);
	Draw(m_QuadMesh.vertex_array, m_QuadMesh.index_buffer, m_BloomShader);

	// Final pass: Combine with post-processing
#ifdef _DEBUG
	glBindFramebuffer(GL_FRAMEBUFFER, m_OffscreenBuffer->FBO);
	glViewport(0, 0, m_OffscreenBuffer->width, m_OffscreenBuffer->height);
#else
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glViewport(0, 0, viewport[2], viewport[3]);
#endif

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
	m_PostProcessShader->SetUniform1i("u_FXAA", m_FXAAEnabled ? 1 : 0);
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

	// Set FXAA parameters
	m_PostProcessShader->SetUniform1f("u_FXAASpanMax", m_FXAASpanMax);
	m_PostProcessShader->SetUniform1f("u_FXAAReduceMin", m_FXAAReduceMin);
	m_PostProcessShader->SetUniform1f("u_FXAAReduceMul", m_FXAAReduceMul);

	Draw(m_QuadMesh.vertex_array, m_QuadMesh.index_buffer, m_PostProcessShader);

	glEnable(GL_DEPTH_TEST);

#ifdef _DEBUG
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

/**
 * @brief Complete deferred rendering pipeline
 * @param view The view matrix
 * @param projection The projection matrix
 */
void Renderer::RenderDeferredPipeline(const Mtx44& view, const Mtx44& projection)
{
	// Shadow pass - render scene from light's perspective to create shadow map
	RenderShadowPass();

	// Geometry pass - write to g-buffer
	RenderGeometryPass(view, projection);

	// Lighting pass - read from g-buffer and perform lighting
	RenderLightingPass(view, projection);

	// Render skybox after lighting but before post-processing
	// This ensures the skybox appears behind all geometry using the depth buffer
	if (m_skybox && m_skybox->IsValid() && m_PostProcessBuffer && m_GBuffer) {
		// Bind the post-process buffer where the lighting pass output is stored
		glBindFramebuffer(GL_FRAMEBUFFER, m_PostProcessBuffer->FBO);
		glViewport(0, 0, m_PostProcessBuffer->width, m_PostProcessBuffer->height);
		
		// Copy depth buffer from g-buffer to post-process buffer for proper depth testing
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_GBuffer->FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_PostProcessBuffer->FBO);
		glBlitFramebuffer(0, 0, m_GBuffer->width, m_GBuffer->height,
						  0, 0, m_PostProcessBuffer->width, m_PostProcessBuffer->height,
						  GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		
		// Bind back to post-process buffer
		glBindFramebuffer(GL_FRAMEBUFFER, m_PostProcessBuffer->FBO);

		// Bind depth texture for depth testing
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_PostProcessBuffer->DepthTexture);

		
		// Enable depth testing but set to render only where depth = 1.0 (background)
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		
		// Render skybox
		m_skybox->Render(view, projection);
		
		// Restore depth state
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
	}

	// Post-processing pass - read from lighting pass output
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
		GLint locS = glGetUniformLocation(m_LightPassShader->GetRendererID(), "u_ShadowMapHandle");

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
		if (locS != -1 && m_ShadowMap)
		{
			glUniform2ui(locS, static_cast<GLuint>(m_ShadowMapHandle),
				static_cast<GLuint>(m_ShadowMapHandle >> 32));
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
		if (m_GBuffer->FBO != 0)
		{
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
	lights.reserve(MaxLights);

	// Convert view matrix to glm once for better performance
	glm::mat4 glmView = glm::mat4(
		view.m00, view.m01, view.m02, view.m03,
		view.m10, view.m11, view.m12, view.m13,
		view.m20, view.m21, view.m22, view.m23,
		view.m30, view.m31, view.m32, view.m33
	);

	// Gather Light and Transform across all alive entities
	const auto& ecs = Ermine::ECS::GetInstance();
	for(EntityID e : m_LightSystem->m_Entities)
	{

		const auto& trans = ecs.GetComponent<Transform>(e);
		const auto& light = ecs.GetComponent<Light>(e);

		// View-space position using GLM
		glm::vec4 posWorld(trans.position.x, trans.position.y, trans.position.z, 1.0f);
		glm::vec4 posView = glmView * posWorld;

		// Build rotation from Euler angles using GLM
		glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
		rotQuat = glm::normalize(rotQuat);
		//rotationMatrix = glm::rotate(rotationMatrix, glm::radians(trans.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		//rotationMatrix = glm::rotate(rotationMatrix, glm::radians(trans.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		//rotationMatrix = glm::rotate(rotationMatrix, glm::radians(trans.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));

		// World-space direction using GLM
		glm::vec3 fwd(0.0f, 0.0f, 1.0f); // Light coming from +Z when unrotated
		glm::vec3 dirWorld = glm::normalize(rotQuat * fwd);

		// View-space direction using GLM
		glm::vec4 dirWorldH(dirWorld, 0.0f); // Homogeneous coordinate with w=0 for directions
		glm::vec4 dirViewH = glmView * dirWorldH;
		glm::vec3 dirView = glm::normalize(glm::vec3(dirViewH));

		// Set spot angles
		float innerCos = 1.0f, outerCos = 1.0f;
		if (light.type == LightType::SPOT) {
			float innerAngle = glm::radians(10.0f);
			float outerAngle = glm::radians(10.0f);
			innerCos = glm::cos(innerAngle);
			outerCos = glm::cos(outerAngle);
		}

		// Convert back to glm::vec4 for LightGPU structure (maintaining compatibility)
		LightGPU gpu{};
		gpu.position_type = glm::vec4(posView.x, posView.y, posView.z, static_cast<float>(light.type));
		gpu.color_intensity = glm::vec4(light.color.x, light.color.y, light.color.z, light.intensity);
		gpu.direction_range = glm::vec4(dirView.x, dirView.y, dirView.z, 100.0f);
		gpu.spot_angles_castshadows_resolution = glm::vec4(innerCos, outerCos, light.castsShadows, light.resolution);
		lights.emplace_back(gpu);
	}

	// Upload
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
 * @brief Binds the Lights uniform block to the specified shader program if it has not been bound before.
 * @param shader The shader program to which the lights block should be bound.
 */
void Renderer::BindLightsBlockIfPresent(const std::shared_ptr<Shader>& shader)
{
	if (!shader || !shader->IsValid())
		return;

	const GLuint program = shader->GetRendererID();
	if (m_LightBlockBoundPrograms.find(program) != m_LightBlockBoundPrograms.end())
		return;

	GLuint blockIndex = glGetUniformBlockIndex(program, "Lights");
	if (blockIndex != GL_INVALID_INDEX)
	{
		glUniformBlockBinding(program, blockIndex, LightsBindingPoint);
		m_LightBlockBoundPrograms.insert(program);
	}
}

/**
 * @brief Updates the material's uniform buffer object (UBO) with the specified material data.
 *
 * If the material UBO does not exist, this function creates one. It then uploads the given material data
 * into the UBO, making it available to the shader for rendering.
 *
 * @param materialData The material data to be uploaded to the UBO, including properties like color, texture, etc.
 */
void Renderer::UpdateMaterialUBO(const graphics::MaterialUBO& materialData)
{
	// Create Material UBO if it doesn't exist
	if (!m_MaterialUBO)
	{
		glGenBuffers(1, &m_MaterialUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, m_MaterialUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(graphics::MaterialUBO), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, MaterialBindingPoint, m_MaterialUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		GLenum error = glGetError();
		if (error != GL_NO_ERROR)
		{
			EE_CORE_ERROR("OpenGL error during MaterialUBO creation: {0}", error);
			return;
		}
	}

	// Validate material data size
	if (sizeof(graphics::MaterialUBO) == 0)
	{
		EE_CORE_ERROR("Invalid MaterialUBO size");
		return;
	}

	// Upload material data with comprehensive error checking
	glBindBuffer(GL_UNIFORM_BUFFER, m_MaterialUBO);
	
	// Check if buffer is properly bound
	GLint boundBuffer;
	glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &boundBuffer);
	if (static_cast<GLuint>(boundBuffer) != m_MaterialUBO)
	{
		EE_CORE_ERROR("Failed to bind MaterialUBO for update");
		return;
	}

	// Perform the buffer update
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(graphics::MaterialUBO), &materialData);
	
	// Check for errors immediately after the critical operation
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		const char* errorString = "";
		switch (error)
		{
		case GL_INVALID_ENUM: errorString = "GL_INVALID_ENUM"; break;
		case GL_INVALID_VALUE: errorString = "GL_INVALID_VALUE"; break;
		case GL_INVALID_OPERATION: errorString = "GL_INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY: errorString = "GL_OUT_OF_MEMORY"; break;
		default: errorString = "UNKNOWN_ERROR"; break;
		}
		EE_CORE_ERROR("OpenGL error in UpdateMaterialUBO during glBufferSubData: {0} ({1})", error, errorString);
		
		// Try to get more diagnostic information
		GLint bufferSize;
		glGetBufferParameteriv(GL_UNIFORM_BUFFER, GL_BUFFER_SIZE, &bufferSize);
		EE_CORE_ERROR("Buffer size: {0}, MaterialUBO size: {1}", bufferSize, sizeof(graphics::MaterialUBO));
	}
	
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
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

	GLuint blockIndex = glGetUniformBlockIndex(program, "MaterialBlock");
	if (blockIndex != GL_INVALID_INDEX)
	{
		glUniformBlockBinding(program, blockIndex, MaterialBindingPoint);
		m_MaterialBlockBoundPrograms.insert(program);
	}
}

/**
 * @brief Update all mesh entities and draw them
 */
void Renderer::Update(const Mtx44& view, const Mtx44& projection)
{
	if (m_UseDeferredRendering)
	{
		// Use deferred rendering pipeline
		RenderDeferredPipeline(view, projection);
	}
	else
	{
#ifdef _DEBUG
		glBindFramebuffer(GL_FRAMEBUFFER, m_OffscreenBuffer->FBO);
		glViewport(0, 0, m_OffscreenBuffer->width, m_OffscreenBuffer->height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

		// Render skybox FIRST as the background
		if (m_skybox && m_skybox->IsValid()) {
			// Disable depth writing for skybox so it appears behind everything
			glDepthMask(GL_FALSE);
			m_skybox->Render(view, projection);
			glDepthMask(GL_TRUE);
		}

		// Update lights UBO for this frame
		UpdateLightsUBO(view);

		auto& ecs = ECS::GetInstance();

		for (auto& entity : m_Entities)
		{
			// Model pipeline
			if (ecs.HasComponent<ModelComponent>(entity))
			{	
				auto& modelComp = ecs.GetComponent<ModelComponent>(entity);
				auto& trans = ecs.GetComponent<Transform>(entity);

				if (modelComp.m_model)
				{
					// Apply entity's transform as root
					glm::mat4 entityModel = glm::mat4(1.0f);
					entityModel = glm::translate(entityModel, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
					glm::quat rotQuat = glm::quat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
					rotQuat = glm::normalize(rotQuat);
					entityModel *= glm::mat4_cast(rotQuat);
					//entityModel = glm::rotate(entityModel, glm::radians(trans.rotation.x), glm::vec3(1, 0, 0));
					//entityModel = glm::rotate(entityModel, glm::radians(trans.rotation.y), glm::vec3(0, 1, 0));
					//entityModel = glm::rotate(entityModel, glm::radians(trans.rotation.z), glm::vec3(0, 0, 1));
					entityModel = glm::scale(entityModel, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

					// Render model
					RenderModel(*modelComp.m_model, view, projection, entityModel);
				}
			}
			// Mesh + material pipeline
			else if (ecs.HasComponent<Mesh>(entity) && ecs.HasComponent<Ermine::Material>(entity))
			{
				auto& trans = ecs.GetComponent<Transform>(entity);
				auto& mesh = ecs.GetComponent<Mesh>(entity);
				auto& materialComponent = ecs.GetComponent<Ermine::Material>(entity);

				// Get the modular material
				Ermine::graphics::Material* material = materialComponent.GetMaterial();

				if (!material) {
					EE_CORE_WARN("Entity {0} has null material", entity);
					continue;
				}

				auto shader = material->GetShader();
				if (!shader || !shader->IsValid()) {
					EE_CORE_WARN("Entity {0} has invalid shader", entity);
					continue;
				}

				// Build model matrix
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
				glm::quat rotQuat = glm::quat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
				rotQuat = glm::normalize(rotQuat);
				model *= glm::mat4_cast(rotQuat);
				//model = glm::rotate(model, glm::radians(trans.rotation.x), glm::vec3(1, 0, 0));
				//model = glm::rotate(model, glm::radians(trans.rotation.y), glm::vec3(0, 1, 0));
				//model = glm::rotate(model, glm::radians(trans.rotation.z), glm::vec3(0, 0, 1));
				model = glm::scale(model, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

				// Update Material UBO with current material data
				UpdateMaterialUBO(material->GetUBOData());

				// Bind material (this handles shader binding and texture binding)
				material->Bind();

				// Bind uniform blocks
				BindLightsBlockIfPresent(shader);
				BindMaterialBlockIfPresent(shader);

				// Set transformation matrices
				shader->SetUniformMatrix4fv("model", model);
				shader->SetUniformMatrix4fv("view", &view.m2[0][0]);
				shader->SetUniformMatrix4fv("projection", &projection.m2[0][0]);

				// Calculate and set normal matrix
				glm::mat4 glmView = glm::mat4(
					view.m00, view.m01, view.m02, view.m03,
					view.m10, view.m11, view.m12, view.m13,
					view.m20, view.m21, view.m22, view.m23,
					view.m30, view.m31, view.m32, view.m33
				);
				glm::mat4 modelView = glmView * model;
				glm::mat3 normalMatrix = transpose(inverse(glm::mat3(modelView)));
				shader->SetUniformMatrix3fv("NormalMatrix", normalMatrix);

				// Set shading mode
				shader->SetUniform1i("isBlinnPhong", m_IsBlinnPhong ? 1 : 0);

				// Handle material properties based on shading mode
				if (m_IsBlinnPhong)
				{
					// Set Blinn-Phong material properties for ALL entities
					auto uboData = material->GetUBOData();

					// Convert PBR properties to Blinn-Phong equivalents
					glm::vec3 albedo = glm::vec3(uboData.albedo.x, uboData.albedo.y, uboData.albedo.z);

					// Set material properties
					shader->SetUniform3f("materialKa", glm::vec3(0.2f) * albedo); // Ambient = 20% of albedo
					shader->SetUniform3f("materialKd", albedo); // Diffuse = albedo
					shader->SetUniform3f("materialKs", glm::vec3(1.0f)); // Specular = white
					shader->SetUniform1f("materialShininess", (1.0f - uboData.roughness) * 128.0f); // Convert roughness to shininess

					// Handle special case for light entities
					if (ECS::GetInstance().HasComponent<Light>(entity))
					{
						auto& light = ECS::GetInstance().GetComponent<Light>(entity);
						// Override for pure emission
						shader->SetUniform3f("materialKe", glm::vec3(light.color.x * light.intensity,
							light.color.y * light.intensity,
							light.color.z * light.intensity));
						shader->SetUniform3f("materialKa", glm::vec3(0.0f));
						shader->SetUniform3f("materialKd", glm::vec3(0.0f));
						shader->SetUniform3f("materialKs", glm::vec3(0.0f));
					}
					else
					{
						// Non-light entities should have no emission
						shader->SetUniform3f("materialKe", glm::vec3(0.0f));
					}
				}
				else
				{
					// PBR mode - handle light entities with emissive materials
					if (ECS::GetInstance().HasComponent<Light>(entity))
					{
						auto& light = ECS::GetInstance().GetComponent<Light>(entity);

						// Create temporary material data for emissive lighting
						MaterialUBO lightMaterialData = material->GetUBOData();
						lightMaterialData.emissive = Vec3(light.color.x, light.color.y, light.color.z);
						lightMaterialData.emissiveIntensity = light.intensity;
						lightMaterialData.albedo = Vec3(0.0f, 0.0f, 0.0f);
						lightMaterialData.metallic = 0.0f;
						lightMaterialData.roughness = 1.0f;

						// Update UBO with light-specific data
						UpdateMaterialUBO(lightMaterialData);
					}
				}

				// Draw the mesh
				Draw(mesh.vertex_array, mesh.index_buffer, shader);

				// Unbind material
				material->Unbind();
			}
		}

#ifdef _DEBUG
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
	}
}

/**
* @brief Draw the mesh
* @param vao The vertex array object
* @param ibo The index buffer object
* @param shader The shader object
*/
void Renderer::Draw(const std::shared_ptr<VertexArray>& vao, const std::shared_ptr<IndexBuffer>& ibo, const std::shared_ptr<Shader>& shader) const
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
	if (m_LightsUBO) {
		glDeleteBuffers(1, &m_LightsUBO);
		m_LightsUBO = 0;
	}

	if (m_MaterialUBO) {
		glDeleteBuffers(1, &m_MaterialUBO);
		m_MaterialUBO = 0;
	}

	CleanupGBuffer();
	CleanupPostProcessBuffer();
}

void Renderer::ToggleDeferredRendering()
{
	m_UseDeferredRendering = !m_UseDeferredRendering;
	if (m_UseDeferredRendering)
		EE_CORE_INFO("Switched to Deferred Rendering");
	else
		EE_CORE_INFO("Switched to Forward Rendering");

}

void Renderer::RenderModel(const Model& model, const Mtx44& view, const Mtx44& projection, const glm::mat4& rootTransform)
{
	const auto& meshes = model.GetMeshes();
	if (meshes.empty())
		return;

	for (const auto& mesh : meshes)
	{
		if (!mesh.vao || !mesh.ibo) continue;

		auto shader = m_GBufferShader ? m_GBufferShader : AssetManager::GetInstance().GetShader("default");
		if (!shader || !shader->IsValid()) continue;

		shader->Bind();
		BindLightsBlockIfPresent(shader);
		BindMaterialBlockIfPresent(shader);

		glm::mat4 modelMat = rootTransform * mesh.localTransform;

		shader->SetUniformMatrix4fv("model", modelMat);
		shader->SetUniformMatrix4fv("view", &view.m2[0][0]);
		shader->SetUniformMatrix4fv("projection", &projection.m2[0][0]);

		glm::mat4 glmView = glm::mat4(
			view.m00, view.m01, view.m02, view.m03,
			view.m10, view.m11, view.m12, view.m13,
			view.m20, view.m21, view.m22, view.m23,
			view.m30, view.m31, view.m32, view.m33
		);
		glm::mat4 modelView = glmView * modelMat;
		glm::mat3 normalMatrix = transpose(inverse(glm::mat3(modelView)));
		shader->SetUniformMatrix3fv("NormalMatrix", normalMatrix);

		if (mesh.texture && mesh.texture->IsValid())
		{
			// Bind texture to texture unit 0
			mesh.texture->Bind(0);

			// Tell the shader which texture unit the sampler uses
			if (shader->HasUniform("materialAlbedoMap"))
				shader->SetUniform1i("materialAlbedoMap", 0);
		}

		Draw(mesh.vao, mesh.ibo, shader);
	}
}

bool Renderer::InitializeShadowMap()
{
	glGenFramebuffers(1, &m_ShadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMapFBO);

	// If a depth texture already exists attach it. Otherwise we create the FBO now
	// and defer attachment until CreateShadowMap is called.
	if (m_ShadowMap != 0)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_ShadowMap, 0);
	}

	// No color buffer is drawn
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	// Only check completeness if we already have a depth attachment.
	if (m_ShadowMap != 0)
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
		// We intentionally do not validate completeness here because texture creation
		// happens after InitializeShadowMap in the Init sequence. The FBO exists and
		// will be validated after the texture is attached in CreateShadowMap.
		EE_CORE_INFO("Initialized shadow FBO (no depth texture attached yet).");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return m_ShadowMapFBO != 0;
}

bool Renderer::CreateShadowMap(const unsigned int resolution)
{
	unsigned int width = resolution, height = resolution;

	// Create depth texture
	glGenTextures(1, &m_ShadowMap);
	glBindTexture(GL_TEXTURE_2D, m_ShadowMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// Ensure we have an FBO; InitializeShadowMap may have created it earlier.
	if (m_ShadowMapFBO == 0)
	{
		glGenFramebuffers(1, &m_ShadowMapFBO);
	}

	// Bind FBO and attach the newly created depth texture
	glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_ShadowMap, 0);

	// No color buffer is drawn
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	// Validate framebuffer completeness now that the texture is attached
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		EE_CORE_ERROR("CreateShadowMap: Shadow map FBO incomplete after attaching depth texture: {0}", status);

		// clean up created resources on failure
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteTextures(1, &m_ShadowMap);
		m_ShadowMap = 0;
		return false;
	}

	// Create bindless handle and make resident immediately so shaders can sample it via handle
	if (m_ShadowMap != 0)
	{
		m_ShadowMapHandle = glGetTextureHandleARB(m_ShadowMap);
		glMakeTextureHandleResidentARB(m_ShadowMapHandle);
	}
	else
	{
		m_ShadowMapHandle = 0;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glCheckError();

	EE_CORE_INFO("Created shadow map with resolution {0}x{1}", width, height);
	return m_ShadowMap != 0;
}
 
bool Renderer::CreateShadowMapCube(const unsigned int resolution)
{
	glGenTextures(1, &m_ShadowMapCube);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_ShadowMapCube);
	for (unsigned int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24,
			resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glCheckError();
	EE_CORE_INFO("Created cube shadow map with resolution {0}x{1}", resolution, resolution);
	return m_ShadowMapCube != 0;
}

void Renderer::CalculateDirectionalMatrix(const editor::EditorCamera & editorCamera)
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

	// Derive near and far world positions along the view center ray (NDC z = -1 and +1)
	glm::vec3 nearPos = UnprojectNDC(0.0f, 0.0f, -1.0f);
	glm::vec3 farPos = UnprojectNDC(0.0f, 0.0f, 1.0f);

	// Compute view-space distances for robust split calculation
	glm::vec4 nearPosView4 = glmView * glm::vec4(nearPos, 1.0f);
	glm::vec4 farPosView4 = glmView * glm::vec4(farPos, 1.0f);
	float nearDist = -nearPosView4.z; // positive distance from camera along view dir
	float farDist = -farPosView4.z;

	if (nearDist <= 1e-6f || farDist <= nearDist)
	{
		EE_CORE_WARN("calculatedirectionalmatrix: invalid camera near/far ({0},{1})", nearDist, farDist);
		return;
	}

	// Direction along camera center ray (world space)
	glm::vec3 viewDir = glm::normalize(farPos - nearPos);

	// Determine directional light vector (use first shadow-casting directional light found)
	glm::vec3 lightDir(0.0f, -1.0f, 0.0f);
	if (m_LightSystem && !m_LightSystem->m_Entities.empty())
	{
		const auto& ecs = Ermine::ECS::GetInstance();
		for (auto e : m_LightSystem->m_Entities)
		{
			if (!ecs.HasComponent<Light>(e) || !ecs.HasComponent<Transform>(e)) continue;
			const auto& light = ecs.GetComponent<Light>(e);
			if (light.type != LightType::DIRECTIONAL) continue;
			if (light.castsShadows == 0) continue;

			const auto& trans = ecs.GetComponent<Transform>(e);
			glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
			rotQuat = glm::normalize(rotQuat);
			glm::vec3 fwd = glm::normalize(rotQuat * glm::vec3(0.0f, 0.0f, 1.0f));
			lightDir = glm::normalize(-fwd); // light direction points from scene to light
			break; // use first suitable directional light
		}
	}

	// Avoid degenerate up vector
	glm::vec3 up(0.0f, 1.0f, 0.0f);
	if (glm::abs(glm::dot(up, lightDir)) > 0.999f)
		up = glm::vec3(1.0f, 0.0f, 0.0f);

	// Number of cascades
	const int numSplits = 3;

	// Blend factor lambda controls how "logarithmic" the split is. 1.0 => pure log, 0.0 => linear.
	const float lambda = 0.95f;

	// Compute split distances in view-space
	std::vector<float> splits;
	splits.resize(numSplits + 1);
	for (int i = 0; i <= numSplits; ++i)
	{
		float si = static_cast<float>(i) / static_cast<float>(numSplits);
		float logSplit = nearDist * std::pow(farDist / nearDist, si);
		float linSplit = nearDist + (farDist - nearDist) * si;
		splits[i] = lambda * logSplit + (1.0f - lambda) * linSplit;
	}

	// Compute light matrices per split
	glm::mat4 splitLightMatrices[3];

	for (int split = 0; split < numSplits; ++split)
	{
		// Compute world-space near and far for this split from distances along center ray.
		float splitNearDist = splits[split];
		float splitFarDist = splits[split + 1];

		glm::vec3 splitNearWorld = nearPos + viewDir * (splitNearDist - nearDist);
		glm::vec3 splitFarWorld = nearPos + viewDir * (splitFarDist - nearDist);

		// Compute clip-space (post projection) z for split near/far to allow unprojection via NDC
		glm::vec4 clipNear = glmProj * glmView * glm::vec4(splitNearWorld, 1.0f);
		glm::vec4 clipFar = glmProj * glmView * glm::vec4(splitFarWorld, 1.0f);
		float ndcZ_splitNear = (clipNear.w == 0.0f) ? -1.0f : (clipNear.z / clipNear.w);
		float ndcZ_splitFar = (clipFar.w == 0.0f) ? 1.0f : (clipFar.z / clipFar.w);

		// Build the 8 world-space corners of the frustum slice [splitNear, splitFar]
		std::array<glm::vec3, 8> frustumCornersWorld;
		const float xs[2] = { -1.0f, 1.0f };
		const float ys[2] = { -1.0f, 1.0f };
		int idx = 0;
		for (int iz = 0; iz < 2; ++iz)
		{
			float zndc = (iz == 0) ? ndcZ_splitNear : ndcZ_splitFar;
			for (int iy = 0; iy < 2; ++iy)
				for (int ix = 0; ix < 2; ++ix)
					frustumCornersWorld[idx++] = UnprojectNDC(xs[ix], ys[iy], zndc);
		}

		// Compute world-space AABB center (used to position the light)
		glm::vec3 minCorner = frustumCornersWorld[0];
		glm::vec3 maxCorner = frustumCornersWorld[0];
		for (const auto& c : frustumCornersWorld)
		{
			minCorner = glm::min(minCorner, c);
			maxCorner = glm::max(maxCorner, c);
		}
		glm::vec3 worldCenter = (minCorner + maxCorner) * 0.5f;

		// Place light at a distance so the frustum slice sits between near/far planes of the ortho projection
		float diagonal = glm::length(maxCorner - minCorner);
		float lightDistance = glm::max(diagonal * 2.0f, (splitFarDist - splitNearDist) + 1.0f);
		glm::vec3 lightPos = worldCenter - lightDir * lightDistance;

		// Initial light view (unrotated)
		glm::mat4 lightView = glm::lookAt(lightPos, worldCenter, up);

		// Transform frustum corners into light space (for PCA / rotating to tight OBB)
		std::vector<glm::vec3> cornersLS;
		cornersLS.reserve(8);
		for (const auto& c : frustumCornersWorld)
		{
			glm::vec4 p = lightView * glm::vec4(c, 1.0f);
			cornersLS.emplace_back(glm::vec3(p));
		}

		// PCA on 2D (x,y) 
		// Compute centroid
		glm::vec2 centroid(0.0f);
		for (const auto& p : cornersLS) centroid += glm::vec2(p.x, p.y);
		centroid /= static_cast<float>(cornersLS.size());

		// Covariance 2x2
		float cov_xx = 0.0f, cov_xy = 0.0f, cov_yy = 0.0f;
		for (const auto& p : cornersLS)
		{
			glm::vec2 d = glm::vec2(p.x, p.y) - centroid;
			cov_xx += d.x * d.x;
			cov_xy += d.x * d.y;
			cov_yy += d.y * d.y;
		}
		cov_xx /= static_cast<float>(cornersLS.size());
		cov_xy /= static_cast<float>(cornersLS.size());
		cov_yy /= static_cast<float>(cornersLS.size());

		// Solve eigenvector for largest eigenvalue of symmetric matrix [cov_xx cov_xy; cov_xy cov_yy]
		float trace = cov_xx + cov_yy;
		float det = cov_xx * cov_yy - cov_xy * cov_xy;
		float disc = std::sqrt(glm::max(0.0f, trace * trace * 0.25f - det));
		float lambda1 = trace * 0.5f + disc;

		glm::vec2 principalAxis;
		if (std::abs(cov_xy) > 1e-6f || std::abs(cov_xx - lambda1) > 1e-6f)
		{
			// (cov_xx - lambda) * vx + cov_xy * vy = 0  => choose vx = cov_xy, vy = lambda - cov_xx (or the symmetric)
			principalAxis = glm::vec2(cov_xy, lambda1 - cov_xx);
			float len = glm::length(principalAxis);
			if (len * len < 1e-12f)
				principalAxis = glm::vec2(1.0f, 0.0f);
			else
				principalAxis = glm::normalize(principalAxis);
		}
		else
		{
			// Degenerate: pick world X
			principalAxis = glm::vec2(1.0f, 0.0f);
		}

		// Rotation angle to align principalAxis to +X
		float angle = std::atan2(principalAxis.y, principalAxis.x);

		// Build rotation around light-space Z to align principal axis to X
		glm::mat4 rot = glm::rotate(glm::mat4(1.0f), -angle, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 rotatedLightView = rot * lightView;

		// Transform corners again with rotated view to compute tight AABB in rotated frame
		glm::vec3 lsMin(FLT_MAX), lsMax(-FLT_MAX);
		for (const auto& c : frustumCornersWorld)
		{
			glm::vec4 p = rotatedLightView * glm::vec4(c, 1.0f);
			glm::vec3 pp = glm::vec3(p);
			lsMin = glm::min(lsMin, pp);
			lsMax = glm::max(lsMax, pp);
		}

		// Small margin to avoid precision clipping
		const float margin = 0.5f;
		lsMin -= glm::vec3(margin);
		lsMax += glm::vec3(margin);

		// Compute ortho extents
		float orthoWidth = lsMax.x - lsMin.x;
		float orthoHeight = lsMax.y - lsMin.y;
		float orthoDepth = lsMax.z - lsMin.z;

		// Clamp extents to avoid degenerate projection
		const float minExtent = 0.1f;
		orthoWidth = glm::max(orthoWidth, minExtent);
		orthoHeight = glm::max(orthoHeight, minExtent);
		orthoDepth = glm::max(orthoDepth, minExtent);

		// Texel-snapping to stabilize shadows and make full use of shadow map resolution
		int shadowRes = 1024;
		if (m_ShadowMap != 0)
		{
			glBindTexture(GL_TEXTURE_2D, m_ShadowMap);
			GLint w = 0;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
			if (w > 0) shadowRes = w;
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// Use a single texel size (square texels) based on the larger ortho extent
		float texelSize = glm::max(orthoWidth, orthoHeight) / static_cast<float>(shadowRes);

		// Compute center in rotated light space and snap to texel grid
		glm::vec4 centerLS4 = rotatedLightView * glm::vec4(worldCenter, 1.0f);
		glm::vec3 centerLS = glm::vec3(centerLS4);
		centerLS.x = floor(centerLS.x / texelSize) * texelSize;
		centerLS.y = floor(centerLS.y / texelSize) * texelSize;

		// Recompute lsMin/lsMax around the snapped center to form tight axes-aligned extents
		float halfW = orthoWidth * 0.5f;
		float halfH = orthoHeight * 0.5f;
		lsMin.x = centerLS.x - halfW;
		lsMax.x = centerLS.x + halfW;
		lsMin.y = centerLS.y - halfH;
		lsMax.y = centerLS.y + halfH;

		// Build orthographic projection with tight near/far based on rotated light-space z extents
		float nearPlane = -lsMax.z;
		float farPlane = -lsMin.z;

		glm::mat4 lightProj = glm::ortho(lsMin.x, lsMax.x, lsMin.y, lsMax.y, nearPlane, farPlane);

		// Store computed light-space matrix for this split (rotated for minimal-area rectangle)
		splitLightMatrices[split] = lightProj * rotatedLightView;
	}

	// For testing: assign the nearest split (split 0 = closest to camera) to the active light view-proj matrix.
	m_LightSpaceMatrix = splitLightMatrices[0];
}

void Renderer::RenderShadowMap(const glm::mat4& lightSpaceMatrix)
{
	// Validate resources
	if (!m_ShadowMapFBO || !m_ShadowMap || !m_ShadowMapShader)
	{
		EE_CORE_WARN("RenderShadowMap: missing shadow FBO/texture/shader");
		return;
	}

	// Query current viewport so we can restore it later
	GLint prevViewport[4];
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	// Query shadow map resolution from the texture
	glBindTexture(GL_TEXTURE_2D, m_ShadowMap);
	GLint shadowWidth = 1024, shadowHeight = 1024; // fallback
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &shadowWidth);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &shadowHeight);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Bind shadow FBO and set viewport to shadow resolution
	glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMapFBO);
	glViewport(0, 0, shadowWidth, shadowHeight);

	// Clear depth
	glClearDepth(1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);

	// Setup render state for depth-only pass
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Cull front faces to reduce shadow acne (common technique)
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	// Bind shadow shader and set light-space matrix/uniforms
	m_ShadowMapShader->Bind();
	m_ShadowMapShader->SetUniformMatrix4fv("u_LightViewProj", lightSpaceMatrix);

	// Try to find a directional light and send its direction / color if present
	if (m_LightSystem)
	{
		const auto& ecs = Ermine::ECS::GetInstance();
		for (EntityID le : m_LightSystem->m_Entities)
		{
			if (!ecs.HasComponent<Light>(le) || !ecs.HasComponent<Transform>(le)) continue;
			const auto& light = ecs.GetComponent<Light>(le);
			if (light.type != LightType::DIRECTIONAL) continue;

			const auto& trans = ecs.GetComponent<Transform>(le);
			glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
			rotQuat = glm::normalize(rotQuat);
			glm::vec3 fwd = glm::normalize(rotQuat * glm::vec3(0.0f, 0.0f, 1.0f));
			glm::vec3 lightDir = glm::normalize(-fwd); // direction from scene to light
			// Use first directional light found
			break;
		}
	}

	// Render all geometry into the depth map
	const auto& ecs = Ermine::ECS::GetInstance();

	// First draw ModelComponent pipeline (models with multiple meshes)
	for (EntityID entity : m_Entities)
	{
		if (!ecs.HasComponent<ModelComponent>(entity)) continue;
		auto& modelComp = ecs.GetComponent<ModelComponent>(entity);
		if (!modelComp.m_model) continue;

		const auto& trans = ecs.GetComponent<Transform>(entity);

		// Build root transform
		glm::mat4 root = glm::mat4(1.0f);
		root = glm::translate(root, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
		glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
		rotQuat = glm::normalize(rotQuat);
		root *= glm::mat4_cast(rotQuat);
		root = glm::scale(root, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

		const auto& meshes = modelComp.m_model->GetMeshes();
		for (const auto& mesh : meshes)
		{
			if (!mesh.vao || !mesh.ibo) continue;

			glm::mat4 modelMat = root * mesh.localTransform;
			m_ShadowMapShader->SetUniformMatrix4fv("model", modelMat);

			// Draw
			Draw(mesh.vao, mesh.ibo, m_ShadowMapShader);
		}
	}

	// Then draw simple mesh+material entities (we only need transform + mesh)
	for (EntityID entity : m_Entities)
	{
		if (!(ecs.HasComponent<Mesh>(entity) && ecs.HasComponent<Ermine::Material>(entity)))
			continue;

		auto& mesh = ecs.GetComponent<Mesh>(entity);
		auto& trans = ecs.GetComponent<Transform>(entity);

		if (!mesh.vertex_array || !mesh.index_buffer) continue;

		// Build model matrix
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
		glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
		rotQuat = glm::normalize(rotQuat);
		model *= glm::mat4_cast(rotQuat);
		model = glm::scale(model, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

		m_ShadowMapShader->SetUniformMatrix4fv("model", model);

		// Draw
		Draw(mesh.vertex_array, mesh.index_buffer, m_ShadowMapShader);
	}

	// Cleanup / restore GL state
	m_ShadowMapShader->Unbind();
	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);

	// Unbind framebuffer and restore viewport
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

	glCheckError();
}

void Renderer::RenderShadowPass()
{
	// Get nearest chunk for directional light
	CalculateDirectionalMatrix(editor::EditorCamera::GetInstance());

	RenderShadowMap(m_LightSpaceMatrix);
}