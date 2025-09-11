/* Start Header ************************************************************************/
/*!
\file       Renderer.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
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

#include "ECS.h"
#include "Logger.h"
#include "MathUtils.h"
#include "Matrix3x3.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/glm.hpp"

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

	// For Batch Rendering
	if (m_InstanceVBO == 0) {
		glGenBuffers(1, &m_InstanceVBO);
	}

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
 * @brief Update all mesh entities and draw them
 */
void Renderer::Update(const Mtx44& view, const Mtx44& projection)
{
#ifdef _DEBUG
	glBindFramebuffer(GL_FRAMEBUFFER, m_OffscreenBuffer->FBO);
	glViewport(0, 0, m_OffscreenBuffer->width, m_OffscreenBuffer->height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

	// Use a single GPU timing event for the entire update
	//GPUProfiler::BeginEvent("Renderer Update");

	for (auto& entity : m_Entities)
	{
		auto& trans = ECS::GetInstance().GetComponent<Transform>(entity);
		auto& mesh = ECS::GetInstance().GetComponent<Mesh>(entity);
		auto& material = ECS::GetInstance().GetComponent<Material>(entity);

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(trans.position.x, trans.position.y, trans.position.z)); // Translate using the position data
		model = glm::rotate(model, radian(trans.rotation.x), glm::vec3(1, 0, 0)); // Rotate around the X axis
		model = glm::rotate(model, radian(trans.rotation.y), glm::vec3(0, 1, 0));
		model = glm::rotate(model, radian(trans.rotation.z), glm::vec3(0, 0, 1));
		model = glm::scale(model, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

		material.m_texture->Bind();

		material.m_shader->Bind();
		material.m_shader->SetUniformMatrix4fv("model", &model[0][0]);
		material.m_shader->SetUniformMatrix4fv("view", &view.m2[0][0]);
		material.m_shader->SetUniformMatrix4fv("projection", &projection.m2[0][0]);

		// Calculate normal matrix (inverse transpose of the upper 3x3 part of model-view matrix)
		glm::mat4 glmView = glm::mat4(
			view.m00, view.m01, view.m02, view.m03,
			view.m10, view.m11, view.m12, view.m13,
			view.m20, view.m21, view.m22, view.m23,
			view.m30, view.m31, view.m32, view.m33
		);
		glm::mat4 modelView = glmView * model;
		glm::mat3 normalMatrix = transpose(inverse(glm::mat3(modelView)));
		material.m_shader->SetUniformMatrix3fv("NormalMatrix", &normalMatrix[0][0]);

		// TODO: Can be moved to a light component
		// Light Properties
		glm::vec4 lightPosWorld(10.f, 10.f, 10.f, 1.0f); // Example position in world space
		glm::vec4 lightPosView = glmView * lightPosWorld;           // Transform to view space
		material.m_shader->SetUniform4f("Light.Position", lightPosView);
		glm::vec3 ld(1.0f, 1.0f, 1.0f); // Light color
		material.m_shader->SetUniform3f("Light.La", glm::vec3(0.2f, 0.2f, 0.2f));
		material.m_shader->SetUniform3f("Light.Ld", ld);
		material.m_shader->SetUniform3f("Light.Ls", glm::vec3(1.0f, 1.0f, 1.0f));

		// Material properties
		glm::vec3 kd(0.9f, 0.9f, 0.9f); // Diffuse reflectivity
		material.m_shader->SetUniform3f("Material.Ka", glm::vec3(0.2f, 0.2f, 0.2f));
		material.m_shader->SetUniform3f("Material.Kd", kd);
		material.m_shader->SetUniform3f("Material.Ks", glm::vec3(0.8f, 0.8f, 0.8f));
		material.m_shader->SetUniform1f("Material.Shininess", 100.0f);

		Draw(mesh.vertex_array, mesh.index_buffer, material.m_shader);
	}
	//GPUProfiler::EndEvent();
#ifdef _DEBUG
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

void Ermine::graphics::Renderer::UpdateWithBatchRender(const Mtx44& view, const Mtx44& projection)
{
#ifdef _DEBUG
	glBindFramebuffer(GL_FRAMEBUFFER, m_OffscreenBuffer->FBO);
	glViewport(0, 0, m_OffscreenBuffer->width, m_OffscreenBuffer->height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif


	std::map<Renderer::BatchKey, std::vector<Renderer::InstanceData>> batches;

	for (auto& entity : m_Entities) {
		if (!ECS::GetInstance().HasComponent<Transform>(entity) ||
			!ECS::GetInstance().HasComponent<Mesh>(entity) ||
			!ECS::GetInstance().HasComponent<Material>(entity))
			continue;

		auto& trans = ECS::GetInstance().GetComponent<Transform>(entity);
		auto& mesh = ECS::GetInstance().GetComponent<Mesh>(entity);
		auto& material = ECS::GetInstance().GetComponent<Material>(entity);

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(trans.position.x, trans.position.y, trans.position.z));
		model = glm::rotate(model, radian(trans.rotation.x), glm::vec3(1, 0, 0));
		model = glm::rotate(model, radian(trans.rotation.y), glm::vec3(0, 1, 0));
		model = glm::rotate(model, radian(trans.rotation.z), glm::vec3(0, 0, 1));
		model = glm::scale(model, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));

		glm::mat4 glmView = glm::mat4(
			view.m00, view.m01, view.m02, view.m03,
			view.m10, view.m11, view.m12, view.m13,
			view.m20, view.m21, view.m22, view.m23,
			view.m30, view.m31, view.m32, view.m33
		);
		glm::mat4 modelView = glmView * model;
		glm::mat3 normalMatrix = transpose(inverse(glm::mat3(modelView)));

		batches[{ mesh.vertex_array.get(), mesh.index_buffer.get(), material.m_shader, material.m_texture }]
			.push_back(Renderer::InstanceData{ model, normalMatrix/*, glm::vec4(1.0f)*/ });
	}

	// now render each batch
	for (auto& [key, instances] : batches) {
		// upload instance buffer
		if (m_InstanceVBO == 0)
			glGenBuffers(1, &m_InstanceVBO);

		glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
		glBufferData(GL_ARRAY_BUFFER,
			instances.size() * sizeof(InstanceData),
			instances.data(),
			GL_DYNAMIC_DRAW);

		key.k_texture->Bind();
		key.k_shader->Bind();
		key.k_shader->SetUniformMatrix4fv("view", &view.m2[0][0]);
		key.k_shader->SetUniformMatrix4fv("projection", &projection.m2[0][0]);

		key.k_vao->Bind();

		glm::mat4 glmView = glm::mat4(
			view.m00, view.m01, view.m02, view.m03,
			view.m10, view.m11, view.m12, view.m13,
			view.m20, view.m21, view.m22, view.m23,
			view.m30, view.m31, view.m32, view.m33
		);
		glm::vec4 lightPosWorld(10.f, 10.f, 10.f, 1.0f);
		glm::vec4 lightPosView = glmView * lightPosWorld;

		key.k_shader->SetUniform4f("Light.Position", lightPosView);
		key.k_shader->SetUniform3f("Light.La", glm::vec3(0.2f, 0.2f, 0.2f));
		key.k_shader->SetUniform3f("Light.Ld", glm::vec3(1.0f, 1.0f, 1.0f));
		key.k_shader->SetUniform3f("Light.Ls", glm::vec3(1.0f, 1.0f, 1.0f));

		// Material
		key.k_shader->SetUniform3f("Material.Ka", glm::vec3(0.2f, 0.2f, 0.2f));
		key.k_shader->SetUniform3f("Material.Kd", glm::vec3(0.9f, 0.9f, 0.9f));
		key.k_shader->SetUniform3f("Material.Ks", glm::vec3(0.8f, 0.8f, 0.8f));
		key.k_shader->SetUniform1f("Material.Shininess", 100.0f);

		// setup attributes
		std::size_t vec4Size = sizeof(glm::vec4);

		// mat4 model (locations 3–6)
		for (int i = 0; i < 4; i++) {
			glEnableVertexAttribArray(3 + i);
			glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE,
				sizeof(InstanceData), (void*)(i * vec4Size));
			glVertexAttribDivisor(3 + i, 1);
		}

		// mat3 normal (locations 7–9)
		std::size_t mat4Size = sizeof(glm::mat4);
		for (int i = 0; i < 3; i++) {
			glEnableVertexAttribArray(7 + i);
			glVertexAttribPointer(7 + i, 3, GL_FLOAT, GL_FALSE,
				sizeof(InstanceData), (void*)(mat4Size + i * sizeof(glm::vec3)));
			glVertexAttribDivisor(7 + i, 1);
		}

		// colour (location 10)
		//glEnableVertexAttribArray(10);
		//glVertexAttribPointer(10, 4, GL_FLOAT, GL_FALSE,
		//	sizeof(InstanceData), (void*)(mat4Size + sizeof(glm::mat3)));
		//glVertexAttribDivisor(10, 1);

		// Instanced Draw
		glDrawElementsInstanced(GL_TRIANGLES,
			//m_mesh->index_buffer->GetCount(),
			key.k_ibo->GetCount(),
			GL_UNSIGNED_INT, 0,
			(GLsizei)instances.size());

		// Track batched draw
		GLsizei instanceCount = (GLsizei)instances.size();
		GPUProfiler::TrackDrawCall(
			key.k_vao->GetVertexCount()* instanceCount,
			key.k_ibo->GetCount()* instanceCount
		);

		key.k_vao->Unbind();
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

const GPUProfiler::PerformanceMetrics& Renderer::GetPerformanceMetrics() const
{
	return GPUProfiler::GetMetrics();
}
