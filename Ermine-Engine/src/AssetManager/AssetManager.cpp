/* Start Header ************************************************************************/
/*!
\file       AssetManager.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       09/03/2025
\brief      This file contains the definition of the AssetManager system.
            This file is used to manage all the assets in the game.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "AssetManager.h"

#include "Logger.h"

using namespace Ermine;

/**
 * @brief Load a texture from the file path and store it in the asset manager, if it is loaded before, return the texture
 * @param filePath The file path of the texture
 * @return The texture that is loaded
 */
std::shared_ptr<graphics::Texture> AssetManager::LoadTexture(const std::string& filePath)
{
    
    EE_CORE_TRACE("Loading texture: {0}", filePath);
    auto it = m_textures.find(filePath);
    if (it != m_textures.end()) // If the texture is already loaded
        return it->second;

    std::shared_ptr<graphics::Texture> texture = std::make_shared<graphics::Texture>(filePath);
    if (!texture->IsValid())
    {
        EE_CORE_ERROR("Failed to load texture: {0}", filePath);
        return nullptr;
    }
    
    m_textures[filePath] = texture;
    EE_CORE_INFO("Texture loaded: {0}", filePath);
    return texture;
}

/**
 * @brief Get the texture from the asset manager
 * @param filePath The file path of the texture
 * @return The texture that is loaded
 */
std::shared_ptr<graphics::Texture> AssetManager::GetTexture(const std::string& filePath)
{
    auto it = m_textures.find(filePath);
    return it != m_textures.end() ? it->second : nullptr;
}

const std::unordered_map<std::string, std::shared_ptr<graphics::Texture>>& AssetManager::GetLoadedTextures() const
{
    return m_textures;
}

/**
 * @brief Load a shader from the vertex and fragment file path and store it in the asset manager, if it is loaded before, return the shader
 * @param vertexPath The file path of the vertex shader
 * @param fragmentPath The file path of the fragment shader
 * @return The shader that is loaded
 */
std::shared_ptr<graphics::Shader> AssetManager::LoadShader(const std::string& vertexPath,
    const std::string& fragmentPath)
{
    EE_CORE_TRACE("Loading shader: {0} | {1}", vertexPath, fragmentPath);
    std::string key = vertexPath + "|" + fragmentPath;
    auto it = m_shaders.find(key);
    if (it != m_shaders.end())
        return it->second;

    std::shared_ptr<graphics::Shader> shader = std::make_shared<graphics::Shader>(vertexPath, fragmentPath);
    if (!shader->IsValid())
    {
        EE_CORE_ERROR("Failed to load shader: {0} | {1}", vertexPath, fragmentPath);
        return nullptr;
    }
    
    m_shaders[key] = shader;
    EE_CORE_INFO("Shader loaded: {0} | {1}", vertexPath, fragmentPath);
    return shader;
}

/**
 * @brief Get the shader from the asset manager
 * @param shaderName The name of the shader
 * @return The shader that is loaded
 */
std::shared_ptr<graphics::Shader> AssetManager::GetShader(const std::string& shaderName)
{
    auto it = m_shaders.find(shaderName);
    return it != m_shaders.end() ? it->second : nullptr;
}

/**
 * @brief Load the contents of a file into a buffer.
 * @param filepath The path to the file to load.
 * @return The contents of the file as a buffer.
 */
const char* AssetManager::load_file_contents(const char* filepath)
{
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        EE_CORE_ERROR("Failed to open file: {0}", filepath);
        return nullptr;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size <= 0)
    {
        EE_CORE_ERROR("File is empty or cannot determine size: {0}", filepath);
        return nullptr;
    }

    auto buffer = new char[size + 1];
    file.read(buffer, size);
    buffer[size] = '\0';

    file.close();
    return buffer;
}

void AssetManager::Clear()
{
    EE_CORE_INFO("Clearing assets: {0} textures, {1} shaders", m_textures.size(), m_shaders.size());
    m_textures.clear();
    m_shaders.clear();
}
