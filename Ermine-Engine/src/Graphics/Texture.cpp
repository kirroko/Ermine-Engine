/* Start Header ************************************************************************/
/*!
\file       Texture.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu (99%)
\co-authors LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu (1%)
\date       09/03/2025
\brief      This file contains the definition of the Texture system.
            This file is used to load textures using stb_image.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "Texture.h"
#include "Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "GPUProfiler.h"
#include "Window.h"

using namespace Ermine::graphics;

void Texture::Release(bool contextExpected) noexcept
{
    if (!m_RendererID)
        return;

    GLFWwindow* current = glfwGetCurrentContext();
    if (!current)
    {
        EE_CORE_ERROR("No current OpenGL context while deleting texture ID={0} path={1} (Leaking GPU resource)", m_RendererID, m_filePath);
        m_RendererID = 0;
        return;
    }

    if (contextExpected && !glIsTexture(m_RendererID))
        EE_CORE_WARN("GL object {0} not recognized as texture (already deleted?) path={1}", m_RendererID, m_filePath);

    GPUProfiler::TrackMemoryDeallocation(static_cast<size_t>(m_Width) * m_Height * 4, "Texture");
    glDeleteTextures(1, &m_RendererID);
    m_RendererID = 0;
	m_Width = m_Height = m_BPP = 0;
    m_LocalBuffer = nullptr;
}

/**
 * @brief Default Constructor
 */
Texture::Texture() : m_RendererID(0), m_LocalBuffer(nullptr), m_Width(0), m_Height(0), m_BPP(0)
{
}

/**
 * @brief Constructor for the Texture system
 * @param filePath The file path of the texture
 */
Texture::Texture(const std::string& filePath) : m_filePath(filePath)
{
    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_set_flip_vertically_on_load(1); // Flip the image vertically
    m_LocalBuffer = stbi_load(filePath.c_str(), &m_Width, &m_Height, &m_BPP, 4); // 4 channels for RGBA
    if (!m_LocalBuffer)
    {
        EE_CORE_WARN("Failed to load texture: {0}", filePath);
        return;
    }
    GPUProfiler::TrackMemoryAllocation(m_Width * m_Height * 4, "Texture");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_LocalBuffer);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(m_LocalBuffer);
}

/**
 * @brief Destructor for the Texture system
 */
Texture::~Texture()
{
    Release(true);
	/*GPUProfiler::TrackMemoryDeallocation(m_Width * m_Height * 4, "Texture");
	glDeleteTextures(1, &m_RendererID);*/
}

Texture::Texture(Texture&& other) noexcept
{
    *this = std::move(other);
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        Release(true);
        m_RendererID = other.m_RendererID;
        m_filePath = std::move(other.m_filePath);
        m_LocalBuffer = other.m_LocalBuffer;
        m_Width = other.m_Width;
        m_Height = other.m_Height;
		m_BPP = other.m_BPP;

        other.m_RendererID = 0;
        other.m_LocalBuffer = nullptr;
        other.m_Width = other.m_Height = other.m_BPP = 0;
    }

    return *this;
}

/**
 * @brief Check if the texture is valid
 * @return true if the texture is valid
 */
bool Texture::IsValid() const
{
    return m_RendererID != 0;
}

/**
 * @brief Bind the texture
 * @param slot The texture slot to bind
 */
void Texture::Bind(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
}

/**
 * @brief Unbind the texture
 */
void Texture::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

/*!***********************************************************************
\brief
 Getter for GLuint m_RendererID
\return
 Returns GLuint m_RendererID
*************************************************************************/
unsigned int Ermine::graphics::Texture::GetRendererID() const
{
    return m_RendererID;
}
