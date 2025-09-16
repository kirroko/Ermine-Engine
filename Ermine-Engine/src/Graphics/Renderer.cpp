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
	// Create a fullscreen quad for rendering the offscreen buffer to the screen
	m_QuadMesh = GeometryFactory::CreateQuad(2.0f, 2.0f);

	// Load deferred shading shaders
	m_GBufferShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/gBuffer_vertex.glsl", "../Resources/Shaders/gBuffer_fragment.glsl");
	m_LightPassShader = AssetManager::GetInstance().LoadShader("../Resources/Shaders/lighting_vertex.glsl", "../Resources/Shaders/lighting_fragment.glsl");
	// Create initial g-buffer
	CreateGBuffer(screenWidth, screenHeight);

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
Renderer::OffscreenBuffer Renderer::Create(const int& width, const int& height)
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
 * @brief Create optimized g-buffer for deferred rendering
 * Total: 160 bits per pixel using RGB32_UINT + RG32_UINT format
 */
Renderer::GBuffer Renderer::CreateGBuffer(const int& width, const int& height)
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
		return gBuffer;
	}

	// Create framebuffer
	glGenFramebuffers(1, &gBuffer.FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.FBO);

	// Create RT0: RGB32_UINT (96 bits) - Albedo, Normal, Emissive
	glGenTextures(1, &gBuffer.PackedTexture0);
	glBindTexture(GL_TEXTURE_2D, gBuffer.PackedTexture0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, width, height, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBuffer.PackedTexture0, 0);

	// Create RT1: RG32_UINT (32 bits) - Material properties
	glGenTextures(1, &gBuffer.PackedTexture1);
	glBindTexture(GL_TEXTURE_2D, gBuffer.PackedTexture1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0, GL_RG_INTEGER, GL_UNSIGNED_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gBuffer.PackedTexture1, 0);

	// Create depth texture for depth testing and reconstruction. 16 bits for memory efficiency
	glGenTextures(1, &gBuffer.DepthTexture);
	glBindTexture(GL_TEXTURE_2D, gBuffer.DepthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gBuffer.DepthTexture, 0);

	// Set up MRTs
	GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, drawBuffers);

	// Check framebuffer completeness
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		EE_CORE_ERROR("ERROR: G-Buffer framebuffer not complete! Status: {0}", status);
		CleanupGBuffer();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return gBuffer;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glCheckError();

	m_GBuffer = std::make_shared<GBuffer>(gBuffer);
	EE_CORE_INFO("Created optimized G-Buffer: {0}x{1}, 160 bits per pixel", width, height);

	return gBuffer;
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
	glDepthMask(GL_TRUE);

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
 * @brief Begin lighting pass for deferred rendering
 */
void Renderer::BeginLightingPass()
{

#ifdef _DEBUG
	glBindFramebuffer(GL_FRAMEBUFFER, m_OffscreenBuffer->FBO);
	glViewport(0, 0, m_OffscreenBuffer->width, m_OffscreenBuffer->height);
#else
	// Bind default framebuffer for final output
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// Get current viewport size
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glViewport(0, 0, viewport[2], viewport[3]);
#endif

	// Bind g-buffer textures for reading
	BindGBufferTextures(0);

	// Set up for lighting calculations
	glDisable(GL_DEPTH_TEST); // No depth testing needed for full-screen pass
	glDisable(GL_BLEND);       // No blending needed for final output
}

/**
 * @brief End lighting pass and finalize frame
 */
void Renderer::EndLightingPass()
{
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glCheckError();

#ifdef _DEBUG
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
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
		model = glm::rotate(model, glm::radians(trans.rotation.x), glm::vec3(1, 0, 0));
		model = glm::rotate(model, glm::radians(trans.rotation.y), glm::vec3(0, 1, 0));
		model = glm::rotate(model, glm::radians(trans.rotation.z), glm::vec3(0, 0, 1));
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
		glm::mat3 normalMatrix = transpose(inverse(glm::mat3(modelView)));
		m_GBufferShader->SetUniformMatrix3fv("NormalMatrix", normalMatrix);

		UpdateMaterialUBO(material->GetUBOData());

		// TODO: Use new texture system to bind textures
		tempTexture->Bind(0);
		// Draw the mesh
		Draw(mesh.vertex_array, mesh.index_buffer, m_GBufferShader);
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
	BindGBufferTextures(0);

	// Set texture uniform locations
	m_LightPassShader->SetUniform1i("u_GBuffer0", 0); // RT0
	m_LightPassShader->SetUniform1i("u_GBuffer1", 1); // RT1  
	m_LightPassShader->SetUniform1i("u_GBufferDepth", 2); // Depth


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
 * @brief Complete deferred rendering pipeline
 * @param view The view matrix
 * @param projection The projection matrix
 */
void Renderer::RenderDeferredPipeline(const Mtx44& view, const Mtx44& projection)
{
	// Geometry pass - write to g-buffer
	RenderGeometryPass(view, projection);

	// Lighting pass - read from g-buffer and perform lighting
	RenderLightingPass(view, projection);
}

/**
 * @brief Bind g-buffer textures to specified texture units
 */
void Renderer::BindGBufferTextures(int startingTextureUnit)
{
	if (!m_GBuffer)
	{
		EE_CORE_WARN("G-Buffer not initialized, cannot bind textures");
		return;
	}

	// Bind packed texture 0 (Albedo + Normal + Emissive)
	glActiveTexture(GL_TEXTURE0 + startingTextureUnit + GBufferPacked0);
	glBindTexture(GL_TEXTURE_2D, m_GBuffer->PackedTexture0);

	// Bind packed texture 1 (Material properties + Motion vectors)
	glActiveTexture(GL_TEXTURE0 + startingTextureUnit + GBufferPacked1);
	glBindTexture(GL_TEXTURE_2D, m_GBuffer->PackedTexture1);

	// Bind depth texture
	glActiveTexture(GL_TEXTURE0 + startingTextureUnit + GBufferDepth);
	glBindTexture(GL_TEXTURE_2D, m_GBuffer->DepthTexture);

	// Reset to texture unit 0
	glActiveTexture(GL_TEXTURE0);
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
		if (m_GBuffer->DepthTexture != 0)
		{
			glDeleteTextures(1, &m_GBuffer->DepthTexture);
		}
		m_GBuffer.reset();
	}
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
	const unsigned long int maxId = ecs.GetLivingEntityCount();
	for (Ermine::EntityID e = 1; e <= maxId && lights.size() < MaxLights; e++)
	{
		if (!ecs.IsEntityValid(e)) continue;
		if (!ecs.HasComponent<Light>(e)) continue;
		if (!ecs.HasComponent<Transform>(e)) continue;

		const auto& trans = ecs.GetComponent<Transform>(e);
		const auto& light = ecs.GetComponent<Light>(e);

		// View-space position using GLM
		glm::vec4 posWorld(trans.position.x, trans.position.y, trans.position.z, 1.0f);
		glm::vec4 posView = glmView * posWorld;

		// Build rotation from Euler angles using GLM
		glm::mat4 rotationMatrix = glm::mat4(1.0f);
		rotationMatrix = glm::rotate(rotationMatrix, glm::radians(trans.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		rotationMatrix = glm::rotate(rotationMatrix, glm::radians(trans.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		rotationMatrix = glm::rotate(rotationMatrix, glm::radians(trans.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));

		// World-space direction using GLM
		glm::vec3 fwd(0.0f, 0.0f, 1.0f); // Light coming from +Z when unrotated
		glm::vec3 dirWorld = glm::mat3(rotationMatrix) * fwd;
		dirWorld = glm::normalize(dirWorld);

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

		// Convert back to Vec4 for LightGPU structure (maintaining compatibility)
		LightGPU gpu{};
		gpu.position_type = Vec4(posView.x, posView.y, posView.z, static_cast<float>(light.type));
		gpu.color_intensity = Vec4(light.color.x, light.color.y, light.color.z, light.intensity);
		gpu.direction_range = Vec4(dirView.x, dirView.y, dirView.z, 100.0f);
		gpu.spot_angles = Vec4(innerCos, outerCos, 0.0f, 0.0f);
		lights.emplace_back(gpu);
	}

	// Upload
	glBindBuffer(GL_UNIFORM_BUFFER, m_LightsUBO);

	Vec4 count(static_cast<float>(lights.size()), 0.0f, 0.0f, 0.0f);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Vec4), count.m);

	if (!lights.empty())
	{
		const GLsizeiptr bodyOffset = static_cast<GLsizeiptr>(sizeof(Vec4));
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

		// Update lights UBO for this frame
		UpdateLightsUBO(view);

		for (auto& entity : m_Entities)
		{
			auto& trans = ECS::GetInstance().GetComponent<Transform>(entity);
			auto& mesh = ECS::GetInstance().GetComponent<Mesh>(entity);
			auto& materialComponent = ECS::GetInstance().GetComponent<Ermine::Material>(entity);

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
			model = glm::rotate(model, glm::radians(trans.rotation.x), glm::vec3(1, 0, 0));
			model = glm::rotate(model, glm::radians(trans.rotation.y), glm::vec3(0, 1, 0));
			model = glm::rotate(model, glm::radians(trans.rotation.z), glm::vec3(0, 0, 1));
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

void Renderer::DrawMesh(const Mesh& mesh,
	const Material& material,
	const Mtx44& view,
	const Mtx44& projection,
	const glm::mat4& model,
	const std::shared_ptr<graphics::Texture>& overrideTex,
	const std::vector<glm::mat4>* bones)
{
	auto shader = material.m_shader;
	shader->Bind();
	shader->SetUniformMatrix4fv("model", &model[0][0]);
	shader->SetUniformMatrix4fv("view", &view.m2[0][0]);
	shader->SetUniformMatrix4fv("projection", &projection.m2[0][0]);

	// Bone
	if (bones)
	{
		for (int i = 0; i < bones->size(); ++i)
		{
			std::string name = "u_Bones[" + std::to_string(i) + "]";
			shader->SetUniformMatrix4fv(name.c_str(), &(*bones)[i][0][0]);
		}
		shader->SetUniform1i("u_BoneCount", (int)bones->size());
	}

	// Normal matrix
	glm::mat4 glmView = glm::mat4(
		view.m00, view.m01, view.m02, view.m03,
		view.m10, view.m11, view.m12, view.m13,
		view.m20, view.m21, view.m22, view.m23,
		view.m30, view.m31, view.m32, view.m33
	);
	glm::mat4 modelView = glmView * model;
	glm::mat3 normalMatrix = transpose(inverse(glm::mat3(modelView)));
	shader->SetUniformMatrix3fv("NormalMatrix", &normalMatrix[0][0]);

	// Simple hardcoded light
	glm::vec4 lightPosWorld(10.f, 10.f, 10.f, 1.0f);
	glm::vec4 lightPosView = glmView * lightPosWorld;
	shader->SetUniform4f("Light.Position", lightPosView);
	shader->SetUniform3f("Light.La", glm::vec3(0.2f));
	shader->SetUniform3f("Light.Ld", glm::vec3(1.0f));
	shader->SetUniform3f("Light.Ls", glm::vec3(1.0f));

	// Material values
	shader->SetUniform3f("Material.Ka", glm::vec3(0.2f));
	shader->SetUniform3f("Material.Kd", glm::vec3(0.9f));
	shader->SetUniform3f("Material.Ks", glm::vec3(0.8f));
	shader->SetUniform1f("Material.Shininess", 100.0f);

	// Texture binding
	if (overrideTex)
		overrideTex->Bind();
	else if (material.m_texture)
		material.m_texture->Bind();

	this->Draw(mesh.vertex_array, mesh.index_buffer, shader);
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
}

void Renderer::ToggleDeferredRendering()
{
	m_UseDeferredRendering = !m_UseDeferredRendering;
	if (m_UseDeferredRendering)
		EE_CORE_INFO("Switched to Deferred Rendering");
	else
		EE_CORE_INFO("Switched to Forward Rendering");

}
