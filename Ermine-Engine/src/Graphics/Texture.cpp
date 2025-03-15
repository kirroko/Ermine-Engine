/* Start Header ************************************************************************/
/*!
\file       Texture.cpp
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This file contains the definition of the Texture system.
            This file is used to load textures using stb_image.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "Texture.h"
#include "Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace Ermine::graphics;

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_LocalBuffer);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(m_LocalBuffer);
}

/**
 * @brief Destructor for the Texture system
 */
Texture::~Texture()
{
    // stbi_image_free(m_LocalBuffer);
    // glDeleteTextures(1, &m_RendererID);
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
