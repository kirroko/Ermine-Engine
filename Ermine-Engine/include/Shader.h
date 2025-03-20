/* Start Header ************************************************************************/
/*!
\file       Shader.h
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This file contains the declaration of the Shader class.
            It will Compile, link and bind the shader to the program.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"

#include <glm/glm.hpp>
#include <glad/glad.h>

#include "Matrix3x3.h"
#include "Matrix4x4.h"

namespace Ermine::graphics
{
    /**
     * @brief The Shader class is used to compile, link and bind the shader to the program.
     */
    class Shader
    {
        GLuint m_RendererID;
        std::unordered_map<std::string, GLint> m_UniformLocationCache;

        /**
         * @brief Get the uniform location of the shader
         * @param name The name of the uniform
         * @return The location of the uniform
         */
        GLint GetUniformLocation(const std::string& name);
        /**
         * @brief Compile the shader
         * @param type The type of the shader
         * @param source The source of the shader
         * @return The shader ID
         */
        GLuint CompileShader(GLenum type, const std::string& source);
        /**
         * @brief Load the shader source
         * @param filepath The path of the shader
         * @return The source of the shader
         */
        std::string LoadShaderSource(const std::string& filepath);
    public:
        Shader() = default;
        
        /**
         * @brief Create a shader
         * @param vertexPath The path of the vertex shader
         * @param fragmentPath The path of the fragment shader
         */
        Shader(const std::string& vertexPath, const std::string& fragmentPath);
        /**
         * @brief Destroy the shader
         */
        ~Shader();

        /**
         * @brief Check if the shader is valid
         * @return True if the shader is valid, false otherwise
         */
        bool IsValid() const;

        /**
        * @brief Bind the shader
        */
        void Bind() const;
        /**
         * @brief Unbind the shader
         */
        void Unbind() const;

        /**
         * @brief Set the uniform value of the shader
         * @param name The name of the uniform
         * @param value The value to set
         */
        void SetUniform1i(const std::string& name, int value);
    
        /**
         * @brief Set the uniform value of the shader
         * @param name The name of the uniform
         * @param value The value to set
         */
        void SetUniform1f(const std::string& name, float value);

        /**
         * @brief Set the uniform value of the shader
         * @param name The name of the uniform
         * @param value The value to set
         */
        void SetUniform3f(const ::std::string& name, const glm::vec3& value);

        /**
         * @brief Set the uniform value of the shader
         * @param name The name of the uniform
         * @param value The value to set
         */
        void SetUniform4f(const std::string& name, const glm::vec4& value);

        /**
         * @brief Set the uniform value of the shader
         * @param name The name of the uniform
         * @param value The value to set
         */
        void SetUniformMatrix3fv(const std::string& name, const Mtx33& value);

        /**
         * @biref Set the uniform value of the shader
         * @param name The name of the uniform
         * @param matrix The matrix to set
         */
        void SetUniformMatrix4fv(const std::string& name, const Mtx44& matrix);

        /**
         * @biref return the renderer ID
         * @return the renderer id
         */
        GLuint GetRendererID() const;
    };
}
