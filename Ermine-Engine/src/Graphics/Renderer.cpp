/* Start Header ************************************************************************/
/*!
\file       Renderer.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\co-author  Jeremy Lim Ting Jie, jeremytingjie.lim, 2301370, jeremytingjie.lim\@digipen.edu
\co-author  Ridhwan Afandi, moahamedridhwan.b, 2301367, moahamedridhwan.b\@digipen.edu
\date       09/27/2025
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

unsigned int SHADOW_MAX_LAYERS = SHADOW_MAX_LAYERS_DESIRED;

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
	m_ShadowMapInstancedShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/shadowmap_instanced_vertex.glsl", "../Resources/Shaders/shadowmap_instanced_geometry.glsl", "../Resources/Shaders/shadowmap_fragment.glsl");
	m_GBufferShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/gBuffer_vertex.glsl", "../Resources/Shaders/gBuffer_fragment.glsl");
	m_LightPassShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/lighting_vertex.glsl", "../Resources/Shaders/lighting_fragment.glsl");
	m_BloomShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/bloom_vertex.glsl", "../Resources/Shaders/bloom_fragment.glsl");
	m_PostProcessShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/postprocess_vertex.glsl", "../Resources/Shaders/postprocess_fragment.glsl");
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
	InitializeShadowMap();
	for (EntityID entity : m_LightSystem->m_Entities)
	{
		auto& light = Ermine::ECS::GetInstance().GetComponent<Light>(entity);
		if (light.castsShadows)
			CreateShadowMapArray();
	}
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

	// If Light SSBO doesn't exist, create it
	if (!m_LightsSSBO)
	{
		glGenBuffers(1, &m_LightsSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_LightsSSBO);
		const GLsizeiptr headerSize = static_cast<GLsizeiptr>(sizeof(glm::vec4));
		const GLsizeiptr bodySize = static_cast<GLsizeiptr>(MAX_LIGHTS * sizeof(LightGPU));
		glBufferData(GL_SHADER_STORAGE_BUFFER, headerSize + bodySize, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, LightsBindingPoint, m_LightsSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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

	// If Light SSBO doesn't exist, create it
	if (!m_LightsSSBO)
	{
		glGenBuffers(1, &m_LightsSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_LightsSSBO);
		const GLsizeiptr headerSize = static_cast<GLsizeiptr>(sizeof(glm::vec4));
		const GLsizeiptr bodySize = static_cast<GLsizeiptr>(MAX_LIGHTS * sizeof(LightGPU));
		glBufferData(GL_SHADER_STORAGE_BUFFER, headerSize + bodySize, nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, LightsBindingPoint, m_LightsSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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

	const auto& ecs = Ermine::ECS::GetInstance();

	for (auto& entity : m_Entities) {
		// Model pipeline
		if (ecs.HasComponent<ModelComponent>(entity)) {
			auto& modelComp = ecs.GetComponent<ModelComponent>(entity);
			auto& trans = ecs.GetComponent<Transform>(entity);

			if (!modelComp.m_model) continue;

			// Check if entity has material component for transparency check
			Ermine::graphics::Material* material = nullptr;
			if (ecs.HasComponent<Ermine::Material>(entity)) {
				auto& materialComponent = ecs.GetComponent<Ermine::Material>(entity);
				material = materialComponent.GetMaterial();
			}

			// Build entity transform
			glm::mat4 entityModel = glm::mat4(1.0f);
			entityModel = glm::translate(entityModel, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
			glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
			rotQuat = glm::normalize(rotQuat);
			entityModel *= glm::mat4_cast(rotQuat);
			entityModel = glm::scale(entityModel, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

			// Check if material is transparent
			if (material && IsTransparentMaterial(material)) {
				// Add to transparent objects list
				TransparentObject transparentObj;
				transparentObj.entity = entity;
				transparentObj.modelMatrix = entityModel;
				transparentObj.distanceToCamera = 0.0f; // Will be calculated in SortTransparentObjects
				m_transparentObjects.push_back(transparentObj);
				continue; // Skip rendering in geometry pass
			}

			// Render opaque model in geometry pass
			if (material) {
				UpdateMaterialUBO(material->GetUBOData());
				BindMaterialTextures(material);
			}
			RenderModel(*modelComp.m_model, view, projection, entityModel);
		}
		// Mesh + material pipeline
		else if (ecs.HasComponent<Mesh>(entity) && ecs.HasComponent<Ermine::Material>(entity)) {
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
			glm::quat rotQuat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
			rotQuat = glm::normalize(rotQuat);
			model *= glm::mat4_cast(rotQuat);
			model = glm::scale(model, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

			// Check if material is transparent
			if (IsTransparentMaterial(material)) {
				// Add to transparent objects list
				TransparentObject transparentObj;
				transparentObj.entity = entity;
				transparentObj.modelMatrix = model;
				transparentObj.distanceToCamera = 0.0f; // Will be calculated in SortTransparentObjects
				m_transparentObjects.push_back(transparentObj);
				continue; // Skip opaque rendering in geometry pass
			}

			// Render opaque object in geometry pass
			m_GBufferShader->SetUniformMatrix4fv("model", model);
			m_GBufferShader->SetUniformMatrix4fv("view", &view.m2[0][0]);
			m_GBufferShader->SetUniformMatrix4fv("projection", &projection.m2[0][0]);

			// Calculate and set normal matrix
			glm::mat4 modelView = glmView * model;
			glm::mat3 normalMatrix = transpose(inverse(glm::mat3(model)));
			m_GBufferShader->SetUniformMatrix3fv("NormalMatrix", normalMatrix);

			UpdateMaterialUBO(material->GetUBOData());
			BindMaterialTextures(material);

			// Draw the mesh
			Draw(mesh.vertex_array, mesh.index_buffer, m_GBufferShader);
		}
	}

	// Sort transparent objects by distance from camera
	SortTransparentObjects(cameraPos);

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
	m_LightPassShader->SetUniform1i("u_VBAO", m_SSAOEnabled ? 1 : 0);

	// Set shading mode
	m_LightPassShader->SetUniform1i("u_ShadingMode", m_IsBlinnPhong ? 1 : 0);

	// Update and bind lights UBO
	// UpdateLightsUBO(view); // updated in shadow pass
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
	// Shadow pass - render scene from light's perspective
	if (frameCounter % SHADOW_MAP_REFRESH_INTERVAL_IN_FRAMES == 0) // Update shadows every 4 frames for performance
		RenderShadowPass();
	// Geometry pass - write opaque objects to g-buffer, collect transparent objects
	RenderGeometryPass(view, projection);

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
	RenderTransparentPass(view, projection);

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

	// Check for OpenGL errors after cleanup
	glCheckError();
}

/**
 * @brief Updates the lights' uniform buffer object (UBO) with the current light and transform data from all living entities.
 * @param view The view matrix to transform the positions and directions of the lights into view space.
 */
void Renderer::UpdateLightsSSBO(const Mtx44& view)
{
	std::vector<LightGPU> lights;
	lights.reserve(MAX_LIGHTS);

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
			float innerAngle = glm::radians(light.innerAngle);
			float outerAngle = glm::radians(light.outerAngle);
			innerCos = glm::cos(innerAngle);
			outerCos = glm::cos(outerAngle);
		}

		// Convert back to glm::vec4 for LightGPU structure (maintaining compatibility)
		LightGPU gpu{};
		gpu.position_type = glm::vec4(posView.x, posView.y, posView.z, static_cast<float>(light.type));
		gpu.color_intensity = glm::vec4(light.color.x, light.color.y, light.color.z, light.intensity);
		gpu.direction_range = glm::vec4(dirView.x, dirView.y, dirView.z, light.radius);
		gpu.spot_angles_castshadows_startOffset = glm::vec4(innerCos, outerCos, light.castsShadows, light.startOffset);
		for (int i = 0; i < NUM_CASCADES; ++i) {
			gpu.lightSpaceMatrix[i] = light.lightSpaceMatrices[i];
			gpu.splitDepths[i / 4][i % 4] = light.splitDepths[i];
		}
		lights.emplace_back(gpu);
	}

	// Upload to SSBO
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_LightsSSBO);

	glm::vec4 count(static_cast<float>(lights.size()), 0.0f, 0.0f, 0.0f);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4), &count);

	if (!lights.empty())
	{
		const GLsizeiptr bodyOffset = static_cast<GLsizeiptr>(sizeof(glm::vec4));
		const GLsizeiptr bodySize = static_cast<GLsizeiptr>(lights.size() * sizeof(LightGPU));
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, bodyOffset, bodySize, lights.data());
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glCheckError();
}

/**
 * @brief Binds the Lights SSBO to the specified shader program if it has not been bound before.
 * @param shader The shader program to which the lights SSBO should be bound.
 */
void Renderer::BindLightsBlockIfPresent(const std::shared_ptr<Shader>& shader)
{
	// Since we use explicit binding in shaders (binding = 1),
	// we don't need to manually bind SSBO blocks like we did with UBOs
	// The shader binding layout handles this automatically
	return;
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
	// Validate material data size first
	constexpr size_t expectedSize = sizeof(graphics::MaterialUBO);
	if (expectedSize == 0)
	{
		EE_CORE_ERROR("Invalid MaterialUBO size: {0}", expectedSize);
		return;
	}
	
	// Ensure size is reasonable (MaterialUBO should be 128 bytes with proper alignment)
	if (expectedSize < 64 || expectedSize > 512)
	{
		EE_CORE_ERROR("MaterialUBO size out of expected range: {0} bytes (expected ~128 bytes)", expectedSize);
		return;
	}

	// Create Material UBO if it doesn't exist
	if (!m_MaterialUBO)
	{
		glGenBuffers(1, &m_MaterialUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, m_MaterialUBO);
		glBufferData(GL_UNIFORM_BUFFER, expectedSize, nullptr, GL_DYNAMIC_DRAW);
		
		// Check for errors during buffer creation
		GLenum error = glGetError();
		if (error != GL_NO_ERROR)
		{
			EE_CORE_ERROR("OpenGL error during MaterialUBO creation: {0}", error);
			return;
		}
		
		glBindBufferBase(GL_UNIFORM_BUFFER, MaterialBindingPoint, m_MaterialUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		
		EE_CORE_INFO("Created MaterialUBO with size: {0} bytes", expectedSize);
	}

	// Upload material data with comprehensive error checking
	glBindBuffer(GL_UNIFORM_BUFFER, m_MaterialUBO);
	
	// Check if buffer is properly bound
	GLint boundBuffer;
	glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &boundBuffer);
	if (static_cast<GLuint>(boundBuffer) != m_MaterialUBO)
	{
		EE_CORE_ERROR("Failed to bind MaterialUBO for update");
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		return;
	}

	// Check buffer size matches expectation
	GLint bufferSize;
	glGetBufferParameteriv(GL_UNIFORM_BUFFER, GL_BUFFER_SIZE, &bufferSize);
	if (static_cast<size_t>(bufferSize) != expectedSize)
	{
		EE_CORE_ERROR("MaterialUBO buffer size mismatch. Expected: {0}, Got: {1}", expectedSize, bufferSize);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		return;
	}

	// Perform the buffer update
	glBufferSubData(GL_UNIFORM_BUFFER, 0, expectedSize, &materialData);
	
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
		EE_CORE_ERROR("Buffer size: {0}, MaterialUBO size: {1}", bufferSize, expectedSize);
		
		// Additional debug information
		EE_CORE_ERROR("MaterialUBO contents preview:");
		EE_CORE_ERROR("  albedo: [{0}, {1}, {2}, {3}]", materialData.albedo.x, materialData.albedo.y, materialData.albedo.z, materialData.albedo.w);
		EE_CORE_ERROR("  metallic: {0}, roughness: {1}, ao: {2}", materialData.metallic, materialData.roughness, materialData.ao);
		EE_CORE_ERROR("  normalStrength: {0}, shadingModel: {1}", materialData.normalStrength, materialData.shadingModel);
	}
	
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glCheckError();
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
		// Use deferred rendering pipeline (now includes transparency)
		RenderDeferredPipeline(view, projection);
	}
	else
	{
		// Forward rendering with transparency support
#ifdef _DEBUG
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

		// Update lights UBO for forward rendering
		UpdateLightsSSBO(view);

		auto& ecs = ECS::GetInstance();

		// First pass: Render opaque objects and collect transparent objects
		for (auto& entity : m_Entities
		)
		{
			// Model pipeline
			if (ecs.HasComponent<ModelComponent>(entity))
			{
				auto& modelComp = ecs.GetComponent<ModelComponent>(entity);
				auto& trans = ecs.GetComponent<Transform>(entity);

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

				// Render opaque model
				if (material) {
					UpdateMaterialUBO(material->GetUBOData());
				}
				RenderModel(*modelComp.m_model, view, projection, entityModel);
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

				// Calculate normal matrix
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

				// Draw the mesh
				Draw(mesh.vertex_array, mesh.index_buffer, shader);

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

						if (material) {
							UpdateMaterialUBO(material->GetUBOData());
						}

						RenderModel(*modelComp.m_model, view, projection, transparentObj.modelMatrix);
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

					// Update Material UBO
					UpdateMaterialUBO(material->GetUBOData());

					// Bind material
					material->Bind();

					// Bind uniform blocks
					BindLightsBlockIfPresent(shader);
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

					// Draw the mesh
					Draw(mesh.vertex_array, mesh.index_buffer, shader);

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

#ifdef _DEBUG
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

void Renderer::DrawInstanced(const std::shared_ptr<VertexArray>& vao, const std::shared_ptr<IndexBuffer>& ibo, const std::shared_ptr<Shader>& shader, int instanceCount) const
{
   vao->Bind();
   glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(ibo->GetCount()), GL_UNSIGNED_INT, 0, instanceCount);
   GPUProfiler::TrackDrawCall(
       static_cast<uint32_t>(vao->GetVertexCount()) ,
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
	if (m_LightsSSBO) {
		glDeleteBuffers(1, &m_LightsSSBO);
		m_LightsSSBO = 0;
	}

	if (m_MaterialUBO) {
		glDeleteBuffers(1, &m_MaterialUBO);
		m_MaterialUBO = 0;
	}

	if (m_ShadowMapArrayHandle != 0) {
		glMakeTextureHandleNonResidentARB(m_ShadowMapArrayHandle);
		m_ShadowMapArrayHandle = 0;
	}

	if (m_ShadowMapArray) {
		glDeleteTextures(1, &m_ShadowMapArray);
		m_ShadowMapArray = 0;
	}
	if (m_ShadowMapFBO) {
		glDeleteFramebuffers(1, &m_ShadowMapFBO);
		m_ShadowMapFBO = 0;
	}

	CleanupGBuffer();
	CleanupPostProcessBuffer();

	// Clean up offscreen buffer
	if (m_OffscreenBuffer) {
		if (m_OffscreenBuffer->FBO != 0) {
			glDeleteFramebuffers(1, &m_OffscreenBuffer->FBO);
		}
		if (m_OffscreenBuffer->ColorTexture != 0) {
			glDeleteTextures(1, &m_OffscreenBuffer->ColorTexture);
		}
		if (m_OffscreenBuffer->RBO != 0) {
			glDeleteRenderbuffers(1, &m_OffscreenBuffer->RBO);
		}
		m_OffscreenBuffer.reset();
	}
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

void Renderer::SortTransparentObjects(const Vec3& cameraPos)
{
	// Calculate distances and sort transparent objects back-to-front
	for (auto& obj : m_transparentObjects) {
		// Extract position from model matrix
		glm::vec3 objPos = glm::vec3(obj.modelMatrix[3]);
		glm::vec3 camPos = glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z);

		obj.distanceToCamera = glm::distance(objPos, camPos);
	}

	// Sort back-to-front for proper alpha blending
	std::sort(m_transparentObjects.begin(), m_transparentObjects.end());
}

void Renderer::RenderTransparentPass(const Mtx44& view, const Mtx44& projection)
{
	if (m_transparentObjects.empty()) return;

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

	// Get ECS reference
	const auto& ecs = Ermine::ECS::GetInstance();

	// Render all transparent objects in sorted order
	for (const auto& transparentObj : m_transparentObjects) {
		EntityID entity = transparentObj.entity;

		if (!ecs.HasComponent<Ermine::Material>(entity)) continue;

		auto& materialComponent = ecs.GetComponent<Ermine::Material>(entity);
		Ermine::graphics::Material* material = materialComponent.GetMaterial();

		if (!material || !IsTransparentMaterial(material)) continue;

		// Use forward shader for transparent objects (enhanced fragment shader)
		auto shader = m_ForwardShader ? m_ForwardShader : material->GetShader();
		if (!shader || !shader->IsValid()) continue;

		shader->Bind();

		// Bind uniform blocks
		BindLightsBlockIfPresent(shader);
		BindMaterialBlockIfPresent(shader);

		// Update material UBO
		UpdateMaterialUBO(material->GetUBOData());

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
				shader->SetUniform1i("materialAlbedoMap", texUnit);
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

		// Render the mesh
		if (ecs.HasComponent<ModelComponent>(entity)) {
			// Handle model component
			auto& modelComp = ecs.GetComponent<ModelComponent>(entity);
			if (modelComp.m_model) {
				RenderModel(*modelComp.m_model, view, projection, transparentObj.modelMatrix);
			}
		}
		else if (ecs.HasComponent<Mesh>(entity)) {
			// Handle regular mesh component
			auto& mesh = ecs.GetComponent<Mesh>(entity);
			if (mesh.vertex_array && mesh.index_buffer) {
				Draw(mesh.vertex_array, mesh.index_buffer, shader);
			}
		}

		// Unbind material
		material->Unbind();
	}

	// Restore render state
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDisable(GL_BLEND);

	// Clear transparent objects list for next frame
	m_transparentObjects.clear();
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
	glGenFramebuffers(1, &m_ShadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMapFBO);

	// If a depth texture already exists attach it. Otherwise we create the FBO now
	// and defer attachment until CreateShadowMap is called.
	if (m_ShadowMapArray != 0)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_ShadowMapArray, 0);
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
		float angle = (2.0f * M_PI * i) / numSamples;
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
 * @brief Calculates the shadow matrix for a spotlight cascade.
 * Computes a view and orthographic projection matrix that tightly fits the cascade frustum in light space.
 * Applies texel snapping and margin adjustments for stable shadows.
 * @param lightPos Position of the spotlight.
 * @param spotDir Direction vector of the spotlight.
 * @param outerAngleRad Outer angle of the spotlight cone in radians.
 * @param lightRadius Maximum range of the spotlight.
 * @param cascadeFrustum Array of eight frustum corners for the cascade.
 * @param shadowRes Shadow map resolution.
 * @return The spotlight's light-space matrix for the cascade.
 */
glm::mat4 Renderer::calculateSpotlightCascadeMatrix(const glm::vec3& lightPos, const glm::vec3& spotDir,
	float outerAngleRad, float lightRadius,
	const std::array<glm::vec3, 8>& cascadeFrustum,
	int shadowRes) {

	// Create spotlight's base view matrix
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	if (glm::abs(glm::dot(spotDir, up)) > 0.999f)
		up = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::mat4 baseView = glm::lookAt(lightPos, lightPos + spotDir, up);

	// Transform cascade frustum to light space
	std::vector<glm::vec3> frustumLS;
	frustumLS.reserve(8);
	for (const auto& corner : cascadeFrustum) {
		glm::vec4 p = baseView * glm::vec4(corner, 1.0f);
		frustumLS.emplace_back(glm::vec3(p));
	}

	// Find AABB in light space
	glm::vec3 lsMin = frustumLS[0];
	glm::vec3 lsMax = frustumLS[0];
	for (const auto& p : frustumLS) {
		lsMin = glm::min(lsMin, p);
		lsMax = glm::max(lsMax, p);
	}

	// Apply margins to prevent clipping
	const float xyMargin = 0.2f;
	const float zMargin = 5.0f;
	lsMin -= glm::vec3(xyMargin, xyMargin, zMargin);
	lsMax += glm::vec3(xyMargin, xyMargin, zMargin);

	// Ensure we don't go beyond the spotlight's natural cone
	float maxConeRadius = lightRadius * std::tan(outerAngleRad);
	float maxExtent = glm::max(glm::abs(lsMin.x), glm::abs(lsMax.x));
	maxExtent = glm::max(maxExtent, glm::max(glm::abs(lsMin.y), glm::abs(lsMax.y)));

	if (maxExtent > maxConeRadius) {
		float scale = maxConeRadius / maxExtent;
		lsMin.x *= scale;
		lsMin.y *= scale;
		lsMax.x *= scale;
		lsMax.y *= scale;
	}

	// Create orthographic projection (tighter fit than perspective for cascaded shadows)
	float nearPlane = -lsMax.z;
	float farPlane = -lsMin.z;

	// Ensure valid near/far planes
	if (nearPlane <= 0.0f) nearPlane = 0.1f;
	if (farPlane <= nearPlane) farPlane = nearPlane + lightRadius;

	glm::mat4 lightProj = glm::ortho(lsMin.x, lsMax.x, lsMin.y, lsMax.y, nearPlane, farPlane);

	return lightProj * baseView;
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
	int currentLayer = 0;

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
				const float xyMargin = 0.5f;
				const float zMargin = 10.0f;
				lsMin -= glm::vec3(xyMargin, xyMargin, zMargin);
				lsMax += glm::vec3(xyMargin, xyMargin, zMargin);

				// Compute ortho extents
				float nearPlane = -lsMax.z;
				float farPlane = -lsMin.z;
				glm::mat4 lightProj = glm::ortho(lsMin.x, lsMax.x, lsMin.y, lsMax.y, nearPlane, farPlane);

				// Store final matrix and split depth
				light.lightSpaceMatrices[split] = lightProj * rotatedLightView;
				light.splitDepths[split] = depthBufferFar;
			}

			currentLayer += NUM_CASCADES;
		}
		else if (light.type == LightType::SPOT) {
			// CASCADED SPOTLIGHT PROCESSING

			// Get spotlight direction and parameters
			glm::vec3 spotDir = glm::normalize(rotQuat * glm::vec3(0.0f, 0.0f, 1.0f));
			float outerAngleRad = glm::radians(light.outerAngle);

			// Compute cascade splits (same as directional lights)
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

			// Test intersection with each cascade and build matrices
			std::vector<bool> intersectingCascades(NUM_CASCADES, false);
			std::vector<std::array<glm::vec3, 8>> cascadeFrustums(NUM_CASCADES);
			int validCascades = 0;

			for (int cascade = 0; cascade < NUM_CASCADES; ++cascade) {
				float splitNearDist = splits[cascade];
				float splitFarDist = splits[cascade + 1];

				// Convert split distances to NDC Z values
				glm::vec3 splitNearWorld = nearPos + viewDir * (splitNearDist - nearDist);
				glm::vec3 splitFarWorld = nearPos + viewDir * (splitFarDist - nearDist);

				glm::vec4 clipNear = glmProj * glmView * glm::vec4(splitNearWorld, 1.0f);
				glm::vec4 clipFar = glmProj * glmView * glm::vec4(splitFarWorld, 1.0f);

				float ndcZ_splitNear = (cascade == 0) ? -1.0f :
					(clipNear.w == 0.0f) ? -1.0f : (clipNear.z / clipNear.w);
				float ndcZ_splitFar = (clipFar.w == 0.0f) ? 1.0f : (clipFar.z / clipFar.w);

				// Create frustum for this cascade
				cascadeFrustums[cascade] = createCascadeFrustum(invPV, ndcZ_splitNear, ndcZ_splitFar);

				// Test intersection with this cascade
				bool intersectsThisCascade = testSpotlightFrustumIntersection(
					lightPos, spotDir, outerAngleRad, light.radius, cascadeFrustums[cascade]);

				if (intersectsThisCascade) {
					intersectingCascades[cascade] = true;
					validCascades++;
				}
			}

			if (validCascades == 0) {
				continue;
			}

			// Check if we have enough shadow map layers
			if (currentLayer + validCascades > SHADOW_MAX_LAYERS) {
				continue;
			}

			light.startOffset = currentLayer;

			// Generate shadow matrices for intersecting cascades
			int cascadeLayerOffset = 0;
			for (int cascade = 0; cascade < NUM_CASCADES; ++cascade) {
				if (intersectingCascades[cascade]) {
					// Calculate specialized matrix for this cascade
					light.lightSpaceMatrices[cascade] = calculateSpotlightCascadeMatrix(
						lightPos, spotDir, outerAngleRad, light.radius,
						cascadeFrustums[cascade], shadowRes);

					// Compute depth for depth buffer (same as directional lights)
					float splitFarDist = splits[cascade + 1];
					glm::vec3 splitFarWorld = nearPos + viewDir * (splitFarDist - nearDist);
					glm::vec4 clipFar = glmProj * glmView * glm::vec4(splitFarWorld, 1.0f);
					float ndcZ_splitFar = (clipFar.w == 0.0f) ? 1.0f : (clipFar.z / clipFar.w);
					float depthBufferFar = ndcZ_splitFar * 0.5f + 0.5f;
					light.splitDepths[cascade] = depthBufferFar;

					cascadeLayerOffset++;
				}
				else {
					// Mark unused cascades with zero matrix
					light.lightSpaceMatrices[cascade] = glm::mat4(0.0f);
					light.splitDepths[cascade] = 0.0f;
				}
			}

			currentLayer += validCascades;
		}
	}
}

/**
 * @brief Renders the shadow map using instanced rendering for all shadow-casting lights and cascades.
 * Sets up the shadow map FBO, viewport, and render state, then draws all geometry using instanced draw calls.
 * Restores previous OpenGL state after rendering.
 */
void Renderer::RenderShadowMapInstanced()
{
	UpdateLightsSSBO(editor::EditorCamera::GetInstance().GetViewMatrix());
	BindLightsBlockIfPresent(m_ShadowMapInstancedShader);

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

	// Cull front faces to reduce shadow acne (common technique)
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

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
	int maxLights = std::min(static_cast<int>(activeShadowLights.size()), 16);
	int totalInstances = maxLights * NUM_CASCADES;

	// Set up per-frame uniforms
	m_ShadowMapInstancedShader->SetUniform1i("u_NumShadowLights", maxLights);
	for (int i = 0; i < maxLights; ++i) {
		std::string uniformName = "u_ActiveShadowLights[" + std::to_string(i) + "]";
		m_ShadowMapInstancedShader->SetUniform1i(uniformName, activeShadowLights[i]);
	}

	// Render all geometry using true instanced rendering
	// First draw ModelComponent pipeline (models with multiple meshes)
	for (EntityID renderEntity : m_Entities)
	{
		if (!ecs.HasComponent<ModelComponent>(renderEntity)) continue;
		auto& modelComp = ecs.GetComponent<ModelComponent>(renderEntity);
		if (!modelComp.m_model) continue;

		const auto& trans = ecs.GetComponent<Transform>(renderEntity);

		// Build root transform
		glm::mat4 root = glm::mat4(1.0f);
		root = glm::translate(root, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
		glm::quat rotQuat = glm::quat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
		rotQuat = glm::normalize(rotQuat);
		root *= glm::mat4_cast(rotQuat);
		root = glm::scale(root, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

		const auto& meshes = modelComp.m_model->GetMeshes();
		for (const auto& mesh : meshes)
		{
			if (!mesh.vao || !mesh.ibo) continue;

			glm::mat4 modelMat = root * mesh.localTransform;
			m_ShadowMapInstancedShader->SetUniformMatrix4fv("model", modelMat);

			// Use instanced draw call
			DrawInstanced(mesh.vao, mesh.ibo, m_ShadowMapInstancedShader, totalInstances);
		}
	}

	// Then draw simple mesh+material entities
	for (EntityID renderEntity : m_Entities)
	{
		if (!(ecs.HasComponent<Mesh>(renderEntity) && ecs.HasComponent<Ermine::Material>(renderEntity)))
			continue;

		auto& mesh = ecs.GetComponent<Mesh>(renderEntity);
		auto& trans = ecs.GetComponent<Transform>(renderEntity);

		if (!mesh.vertex_array || !mesh.index_buffer) continue;

		// Build model matrix
		glm::mat4 modelMat = glm::mat4(1.0f);
		modelMat = glm::translate(modelMat, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
		glm::quat rotQuat = glm::quat(trans.rotation.w, trans.rotation.x, trans.rotation.y, trans.rotation.z);
		rotQuat = glm::normalize(rotQuat);
		modelMat *= glm::mat4_cast(rotQuat);
		modelMat = glm::scale(modelMat, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

		m_ShadowMapInstancedShader->SetUniformMatrix4fv("model", modelMat);

		// Use instanced draw call
		DrawInstanced(mesh.vertex_array, mesh.index_buffer, m_ShadowMapInstancedShader, totalInstances);
	}

	// Restore culling state (back to normal)
	glCullFace(GL_BACK);

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

	// Render shadows with optimized instanced approach
	RenderShadowMapInstanced();
}

#pragma endregion