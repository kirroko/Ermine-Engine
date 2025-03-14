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
#include "glm/gtc/matrix_transform.hpp"
#include "glm/glm.hpp"

using namespace Ermine::graphics;

/**
 * @brief Update all mesh entities and draw them
 * @param view view matrix
 * @param projection projection matrix
 */
void Renderer::Update(const Mtx44& view, const Mtx44& projection)
{
    for (auto& entity : m_Entities)
    {
        // auto& trans = ECS::GetInstance().GetComponent<Transform>(entity);
        auto& mesh = ECS::GetInstance().GetComponent<Mesh>(entity);
        auto& material = ECS::GetInstance().GetComponent<Material>(entity);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 _view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 _projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
        glm::mat4 mvp = _projection * _view * model;
        GLint mvp_location = glGetUniformLocation(material.m_shader->GetRendererID(),"MVP");

        material.m_shader->Bind();
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) &mvp);

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
