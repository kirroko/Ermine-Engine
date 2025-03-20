/* Start Header ************************************************************************/
/*!
\file       Renderer.cpp
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This file contains the definition of the Renderer system.
            This file is used to render the game objects.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "Renderer.h"

#include "ECS.h"
#include "Logger.h"
#include "Components.h"
#include "MathUtils.h"
#include "Matrix3x3.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/glm.hpp"

using namespace Ermine::graphics;

/**
 * @brief Update all mesh entities and draw them
 */
void Renderer::Update(const Mtx44& view, const Mtx44& projection)
{
    for (auto& entity : m_Entities)
    {
        auto& trans = ECS::GetInstance().GetComponent<Transform>(entity);
        auto& mesh = ECS::GetInstance().GetComponent<Mesh>(entity);
        auto& material = ECS::GetInstance().GetComponent<Material>(entity);
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(trans.position.x, trans.position.y, trans.position.z)); // Translate using the position data
        model = glm::rotate(model, radian(trans.rotation.x), glm::vec3(1, 0, 0)); // Rotate around the X axis
        model = glm::rotate(model, radian(trans.rotation.y), glm::vec3(0, 1, 0));
        model  = glm::rotate(model, radian(trans.rotation.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(trans.scale.x, trans.scale.y, trans.scale.z));
        // glm::mat4 _view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // glm::mat4 _projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
        // glm::mat4 mvp = _projection * _view * model;

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
        
        // Light Properties
        glm::vec4 lightPosWorld(10.f, 10.f, 10.f, 1.0f); // Example position in world space
        glm::vec4 lightPosView = glmView * lightPosWorld;           // Transform to view space
        material.m_shader->SetUniform4f("Light.Position", lightPosView);
        glm::vec3 ld(1.0f, 1.0f, 1.0f); // Light color
        material.m_shader->SetUniform3f("Light.La", glm::vec3(0.2f,0.2f,0.2f));
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
