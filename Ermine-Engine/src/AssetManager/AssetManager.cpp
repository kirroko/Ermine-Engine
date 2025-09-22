/* Start Header ************************************************************************/
/*!
\file       AssetManager.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu (80%)
\co-author  Jeremy Lim Ting Jie, jeremytingjie.lim, 2301370, jeremytingjie.lim\@digipen.edu (20%)
\co-authors Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       10/09/2025
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
#include "Material.h"

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
 * @brief Load a shader from a vertex, geometry and fragment file
 * @param vertexPath The path to the vertex shader file
 * @param geometryPath The path to the geometry shader file
 * @param fragmentPath The path to the fragment shader file
 * @return The loaded shader
 */
std::shared_ptr<graphics::Shader> AssetManager::LoadShader(const std::string& vertexPath, const std::string& geometryPath, const std::string& fragmentPath)
{
    EE_CORE_TRACE("Loading shader: {0} | {1} | {2}", vertexPath, geometryPath, fragmentPath);
    std::string key = vertexPath + "|" + geometryPath + "|" + fragmentPath;

    std::shared_ptr<graphics::Shader> shader = std::make_shared<graphics::Shader>(vertexPath, geometryPath, fragmentPath);
    if (!shader->IsValid())
    {
        EE_CORE_ERROR("Failed to load shader: {0} | {1} | {2}", vertexPath, geometryPath, fragmentPath);
        return nullptr;
    }

    m_shaders[key] = shader;
    EE_CORE_INFO("Shader loaded: {0} | {1} | {2}", vertexPath, geometryPath, fragmentPath);
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
* @brief Load a 3D model from file using Assimp.
* @param filePath The path to the model file (e.g. .fbx, .obj, .gltf).
* @return The loaded model.
*/
std::shared_ptr<graphics::Model> AssetManager::LoadModel(const std::string& filePath)
{
    EE_CORE_TRACE("Loading model: {0}", filePath);
    auto it = m_models.find(filePath);
    if (it != m_models.end()) // Already loaded
        return it->second;

    try
    {
        std::shared_ptr<graphics::Model> model = std::make_shared<graphics::Model>(filePath);
        m_models[filePath] = model;
        EE_CORE_INFO("Model loaded: {0}", filePath);
        return model;
    }
    catch (const std::exception& e)
    {
        EE_CORE_ERROR("Failed to load model: {0}, reason: {1}", filePath, e.what());
        return nullptr;
    }
}

/**
* @brief Get a model from the cache.
* @param filePath The path to the model file.
* @return The model if it exists, nullptr otherwise.
*/
std::shared_ptr<graphics::Model> AssetManager::GetModel(const std::string& filePath)
{
    auto it = m_models.find(filePath);
    return it != m_models.end() ? it->second : nullptr;
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
    EE_CORE_INFO("Clearing assets: {0} textures, {1} shaders, {2} cubemaps, {3} materials, {4} models", 
                 m_textures.size(), m_shaders.size(), m_cubemaps.size(), m_materials.size(), m_models.size());
    m_textures.clear();
    m_shaders.clear();
    m_cubemaps.clear();
    m_materials.clear();
    m_models.clear();
}

/**
 * @brief Load a cubemap from individual face textures and store it in the asset manager
 * @param faces Array of 6 face texture paths in order: +X, -X, +Y, -Y, +Z, -Z  
 * @param name Optional name for the cubemap (for caching)
 * @return The loaded cubemap
 */
std::shared_ptr<graphics::Cubemap> AssetManager::LoadCubemap(const std::array<std::string, 6>& faces, const std::string& name)
{
    // Create a cache key
    std::string key = name.empty() ? 
        (faces[0] + "|" + faces[1] + "|" + faces[2] + "|" + faces[3] + "|" + faces[4] + "|" + faces[5]) : 
        name;
    
    EE_CORE_TRACE("Loading cubemap: {0}", key);
    
    // Check if already loaded
    auto it = m_cubemaps.find(key);
    if (it != m_cubemaps.end())
        return it->second;
    
    // Create new cubemap
    std::shared_ptr<graphics::Cubemap> cubemap = std::make_shared<graphics::Cubemap>(faces);
    if (!cubemap->IsValid())
    {
        EE_CORE_ERROR("Failed to load cubemap: {0}", key);
        return nullptr;
    }
    
    m_cubemaps[key] = cubemap;
    EE_CORE_INFO("Cubemap loaded: {0}", key);
    return cubemap;
}

/**
 * @brief Load a cubemap from an equirectangular texture
 * @param equirectangularPath Path to the equirectangular texture
 * @param name Optional name for the cubemap (for caching)
 * @return The loaded cubemap
 */
std::shared_ptr<graphics::Cubemap> AssetManager::LoadCubemapFromEquirectangular(const std::string& equirectangularPath, const std::string& name)
{
    std::string key = name.empty() ? equirectangularPath : name;
    
    EE_CORE_TRACE("Loading cubemap from equirectangular: {0}", key);
    
    // Check if already loaded
    auto it = m_cubemaps.find(key);
    if (it != m_cubemaps.end())
        return it->second;
    
    // Create new cubemap from equirectangular
    std::shared_ptr<graphics::Cubemap> cubemap = std::make_shared<graphics::Cubemap>(equirectangularPath);
    if (!cubemap->IsValid())
    {
        EE_CORE_ERROR("Failed to load cubemap from equirectangular: {0}", key);
        return nullptr;
    }
    
    m_cubemaps[key] = cubemap;
    EE_CORE_INFO("Cubemap loaded from equirectangular: {0}", key);
    return cubemap;
}

/**
 * @brief Get a cubemap from the cache
 * @param name The name of the cubemap
 * @return The cubemap if it exists, nullptr otherwise
 */
std::shared_ptr<graphics::Cubemap> AssetManager::GetCubemap(const std::string& name)
{
    auto it = m_cubemaps.find(name);
    return it != m_cubemaps.end() ? it->second : nullptr;
}

/**
 * @brief Create and cache a material with the given name
 * @param name The name/key for the material
 * @param shader The shader to use for the material
 * @param materialTemplate Optional material template to apply
 * @return The created material
 */
std::shared_ptr<graphics::Material> AssetManager::CreateMaterial(const std::string& name, 
                                                               std::shared_ptr<graphics::Shader> shader,
                                                               const std::string& materialTemplate)
{
    EE_CORE_TRACE("Creating material: {0}", name);
    
    // Check if already exists
    auto it = m_materials.find(name);
    if (it != m_materials.end())
    {
        EE_CORE_TRACE("Material {0} already exists, returning cached version", name);
        return it->second;
    }
    
    if (!shader || !shader->IsValid())
    {
        EE_CORE_ERROR("Cannot create material {0} with invalid shader", name);
        return nullptr;
    }
    
    // Create new material
    auto material = std::make_shared<graphics::Material>(shader);
    
    // Apply template if specified
    if (!materialTemplate.empty())
    {
        if (materialTemplate == "PBR_WHITE")
            material->LoadTemplate(graphics::MaterialTemplates::PBR_WHITE());
        else if (materialTemplate == "PBR_RED")
            material->LoadTemplate(graphics::MaterialTemplates::PBR_RED());
        else if (materialTemplate == "PBR_METAL")
            material->LoadTemplate(graphics::MaterialTemplates::PBR_METAL());
        else if (materialTemplate == "PBR_REFLECTIVE")
            material->LoadTemplate(graphics::MaterialTemplates::PBR_REFLECTIVE(0.9f, 0.1f));
        else if (materialTemplate == "EMISSIVE_WHITE")
            material->LoadTemplate(graphics::MaterialTemplates::EMISSIVE(Vec3(1.0f, 1.0f, 1.0f), 10.0f));
        else
            EE_CORE_WARN("Unknown material template: {0}", materialTemplate);
    }
    
    m_materials[name] = material;
    EE_CORE_INFO("Material created and cached: {0}", name);
    return material;
}

/**
 * @brief Get a material from the cache
 * @param name The name of the material
 * @return The material if it exists, nullptr otherwise
 */
std::shared_ptr<graphics::Material> AssetManager::GetMaterial(const std::string& name)
{
    auto it = m_materials.find(name);
    return it != m_materials.end() ? it->second : nullptr;
}

/**
 * @brief Create a shared material for common use cases
 * @param materialType Type of material (e.g., "wood", "metal", "plastic")
 * @param shader The shader to use
 * @param baseTexture Optional base texture
 * @return The created shared material
 */
std::shared_ptr<graphics::Material> AssetManager::CreateSharedMaterial(const std::string& materialType,
                                                                     std::shared_ptr<graphics::Shader> shader,
                                                                     std::shared_ptr<graphics::Texture> baseTexture)
{
    std::string materialName = materialType + "_shared";
    
    // Check if this shared material already exists
    auto existing = GetMaterial(materialName);
    if (existing)
        return existing;
    
    EE_CORE_TRACE("Creating shared material: {0}", materialName);
    
    auto material = std::make_shared<graphics::Material>(shader);
    
    // Set up material based on type
    if (materialType == "wood")
    {
        material->LoadTemplate(graphics::MaterialTemplates::PBR_WHITE());
        material->SetFloat("material.roughness", 0.8f);
        material->SetFloat("material.metallic", 0.0f);
    }
    else if (materialType == "metal")
    {
        material->LoadTemplate(graphics::MaterialTemplates::PBR_METAL());
        material->SetFloat("material.roughness", 0.2f);
        material->SetFloat("material.metallic", 1.0f);
    }
    else if (materialType == "plastic")
    {
        material->LoadTemplate(graphics::MaterialTemplates::PBR_WHITE());
        material->SetFloat("material.roughness", 0.7f);
        material->SetFloat("material.metallic", 0.0f);
    }
    else if (materialType == "ceramic")
    {
        material->LoadTemplate(graphics::MaterialTemplates::PBR_WHITE());
        material->SetFloat("material.roughness", 0.1f);
        material->SetFloat("material.metallic", 0.0f);
    }
    else if (materialType == "rubber")
    {
        material->LoadTemplate(graphics::MaterialTemplates::PBR_WHITE());
        material->SetFloat("material.roughness", 0.9f);
        material->SetFloat("material.metallic", 0.0f);
    }
    else
    {
        // Default material
        material->LoadTemplate(graphics::MaterialTemplates::PBR_WHITE());
        EE_CORE_WARN("Unknown material type {0}, using default PBR_WHITE", materialType);
    }
    
    // Apply base texture if provided
    if (baseTexture && baseTexture->IsValid())
    {
        material->SetTexture("materialAlbedoMap", baseTexture);
        material->SetTexture("texture0", baseTexture); // Fallback for compatibility
    }
    
    m_materials[materialName] = material;
    EE_CORE_INFO("Shared material created and cached: {0}", materialName);
    return material;
}
