/* Start Header ************************************************************************/
/*!
\file       Shader.cpp
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This file contains the definition of the Shader system.
            This file is used to load the shader files.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "Shader.h"

#include "Logger.h"
#include "Matrix3x3.h"
#include "glad/glad.h"

using namespace Ermine::graphics;

/**
 * @brief Get the uniform location of the shader
 * @param name The name of the uniform
 * @return The location of the uniform
 */
GLint Shader::GetUniformLocation(const std::string& name)
{
    if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
        return m_UniformLocationCache[name];

    if (m_RendererID == 0)
    {
        EE_CORE_ERROR("Shader program ID  is invalid!");
        return -1;
    }

    GLint location = glGetUniformLocation(m_RendererID, name.c_str());
    if (location == -1)
        EE_CORE_WARN("Uniform {0} not found in shader", name);

    m_UniformLocationCache[name] = location;
    return location;
}

/**
 * @brief Compile the shader
 * @param type The type of the shader
 * @param source The source of the shader
 * @return The shader ID
 */
GLuint Shader::CompileShader(GLenum type, const std::string& source)
{
    EE_CORE_TRACE("Compiling shader: {0}", source);
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

        glDeleteShader(shader);

        EE_CORE_ERROR("Shader compilation failed: {0}", infoLog.data());
        return 0;
    }
    
    return shader;
}

/**
 * @brief Load the shader source
 * @param filepath The path of the shader
 * @return The source of the shader
 */
std::string Shader::LoadShaderSource(const std::string& filepath)
{
    std::ifstream file(filepath);
    
    if (!file.is_open())
    {
        EE_CORE_ERROR("Failed to open shader file: {0}", filepath);
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    if (file.bad())
    {
        EE_CORE_ERROR("Error occurred while reading shader file: {0}", filepath);
        return "";
    }

    file.close();
    return buffer.str();
}

/**
 * @brief Create a shader
 * @param vertexPath The path of the vertex shader
 * @param fragmentPath The path of the fragment shader
 */
Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vertexSource = LoadShaderSource(vertexPath);
    std::string fragmentSource = LoadShaderSource(fragmentPath);

    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

    // Link the shaders to the program
    m_RendererID = glCreateProgram();
    glAttachShader(m_RendererID,vertexShader);
    glAttachShader(m_RendererID,fragmentShader);
    glLinkProgram(m_RendererID);

    GLint isLinked = 0;
    glGetProgramiv(m_RendererID, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetProgramiv(m_RendererID,GL_INFO_LOG_LENGTH,&maxLength);

        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(m_RendererID,maxLength,&maxLength, &infoLog[0]);

        glDeleteProgram(m_RendererID);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        EE_CORE_ERROR("Shader linking failed: {0}", infoLog.data());
        m_RendererID = 0;
        return;
    }

    // Delete the shaders as they are linked to the program and no longer needed
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

/**
 * @brief Destroy the shader
 */
Shader::~Shader()
{
    glDeleteProgram(m_RendererID);
}

/**
 * @brief Check if the shader is valid
 * @return True if the shader is valid, false otherwise
 */
bool Shader::IsValid() const
{
    return m_RendererID != 0;
}

/**
 * @brief Bind the shader
 */
void Shader::Bind() const
{
    glUseProgram(m_RendererID);
}

/**
 * @brief Unbind the shader
 */
void Shader::Unbind() const
{
    glUseProgram(0);
}

/**
 * @brief Set the uniform value of the shader
 * @param name The name of the uniform
 * @param value The value to set
 */
void Shader::SetUniform1i(const std::string& name, int value)
{
    glUniform1i(GetUniformLocation(name), value);
}

/**
 * @brief Set the uniform value of the shader
 * @param name The name of the uniform
 * @param value The value to set
 */
void Shader::SetUniform1f(const std::string& name, float value)
{
    glUniform1f(GetUniformLocation(name), value);
}

/**
 * @brief Set the uniform value of the shader
 * @param name The name of the uniform
 * @param value The value to set
 */
void Shader::SetUniform3f(const std::string& name, const glm::vec3& value)
{
    glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
}

/**
 * @brief Set the uniform value of the shader
 * @param name The name of the uniform
 * @param value The value to set
 */
void Shader::SetUniform4f(const std::string& name, const glm::vec4& value)
{
    glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w);
}

/**
 * @brief Set the uniform value of the shader
 * @param name The name of the uniform
 * @param value The value to set
 */
void Shader::SetUniformMatrix3fv(const std::string& name, const Mtx33& value)
{
    glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, &value.m2[0][0]);
}

/**
 * @brief Set the uniform value of the shader
 * @param name The name of the uniform
 * @param matrix The value to set
 */
void Shader::SetUniformMatrix4fv(const std::string& name, const Mtx44& matrix)
{
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix.m2[0][0]);
}

/**
 * @brief Get the renderer ID of the shader
 * @return The renderer ID of the shader
 */
GLuint Shader::GetRendererID() const
{
    return m_RendererID;
}
