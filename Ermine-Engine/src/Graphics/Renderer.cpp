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
#include "Components.h"
#include "MathUtils.h"
#include "Matrix3x3.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/glm.hpp"
#include "Input.h"
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
 * @brief Updates the lights' uniform buffer object (UBO) with the current light and transform data from all living entities.
 * @param view The view matrix to transform the positions and directions of the lights into view space.
 */
void Renderer::UpdateLightsUBO(const Mtx44& view)
{
	std::vector<LightGPU> lights;
	lights.reserve(MaxLights);

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

		// View-space position
		/*
		Vec3 posView3 = view * trans.position;
		Vec4 posView(posView3.x, posView3.y, posView3.z, 1.0f);
		*/

		glm::mat4 glmView = glm::mat4(
			view.m00, view.m01, view.m02, view.m03,
			view.m10, view.m11, view.m12, view.m13,
			view.m20, view.m21, view.m22, view.m23,
			view.m30, view.m31, view.m32, view.m33
		);
		glm::vec4 posView = glmView * glm::vec4{ trans.position.x, trans.position.y, trans.position.z, 1.0f };

		// Build rotation from Euler (Z * Y * X)
		Mtx44 rx, ry, rz;
		Mtx44RotXRad(rx, radian(trans.rotation.x));
		Mtx44RotYRad(ry, radian(trans.rotation.y));
		Mtx44RotZRad(rz, radian(trans.rotation.z));
		Mtx44 rot = rz * ry * rx;

		// World-space direction
		Vec3 fwd(0.0f, 0.0f, 1.0f); // Light coming from +Z when unrotated
		Vec3 dirWorld = rot * fwd;
		Vec3 dirWorldN;
		Vec3Normalize(dirWorldN, dirWorld);

		// View-space direction
		Vec3 dirViewRaw = Vec3(
			view.m00 * dirWorldN.x + view.m01 * dirWorldN.y + view.m02 * dirWorldN.z,
			view.m10 * dirWorldN.x + view.m11 * dirWorldN.y + view.m12 * dirWorldN.z,
			view.m20 * dirWorldN.x + view.m21 * dirWorldN.y + view.m22 * dirWorldN.z
		);
		Vec3 dirView;
		Vec3Normalize(dirView, dirViewRaw);

		// Set spot angles
		float innerCos = 1.0f, outerCos = 1.0f;
		if (light.type == LightType::SPOT) {
			float innerAngle = radian(10.f);
			float outerAngle = radian(10.f);
			innerCos = cos(innerAngle);
			outerCos = cos(outerAngle);
		}

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
		glCheckError();
	}

	// Upload material data
	glBindBuffer(GL_UNIFORM_BUFFER, m_MaterialUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(graphics::MaterialUBO), &materialData);
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
		model = glm::rotate(model, radian(trans.rotation.x), glm::vec3(1, 0, 0));
		model = glm::rotate(model, radian(trans.rotation.y), glm::vec3(0, 1, 0));
		model = glm::rotate(model, radian(trans.rotation.z), glm::vec3(0, 0, 1));
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
   shader->Unbind();
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
}

