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
 * @brief Initialize the asset manager with database path
 * @param databasePath Path to the resource database
 * @param projectGuid Project GUID (optional, will try to auto-detect)
 * @return true if initialization successful
 */
bool AssetManager::Initialize(const std::string& databasePath, const std::string& projectGuid)
{
    m_databasePath = databasePath;
    m_projectGuid = projectGuid;

    EE_CORE_INFO("Initializing AssetManager with database: {0}", databasePath);

    // Try to load the resource database
    if (LoadResourceDatabase())
    {
        EE_CORE_INFO("Resource database loaded successfully with {0} entries", m_resourceDatabase.size());
        return true;
    }
    else
    {
        EE_CORE_WARN("Could not load resource database - falling back to direct file loading");
        return true; // Don't fail initialization, just fall back to direct loading
    }
}

/**
 * @brief Load the resource database from the pipeline output
 * @return true if database loaded successfully
 */
bool AssetManager::LoadResourceDatabase()
{
    // If project GUID is not set, try to find it by scanning the database directory
    if (m_projectGuid.empty())
    {
        if (!std::filesystem::exists(m_databasePath))
        {
            EE_CORE_WARN("Database path does not exist: {0}", m_databasePath);
            return false;
        }

        // Look for project folders (should be GUIDs)
        for (const auto& entry : std::filesystem::directory_iterator(m_databasePath))
        {
            if (entry.is_directory())
            {
                std::string folderName = entry.path().filename().string();
                // Check if this looks like a GUID folder by looking for Browser.dbase
                std::string browserPath = entry.path().string() + "/Browser.dbase";
                if (std::filesystem::exists(browserPath))
                {
                    m_projectGuid = folderName;
                    EE_CORE_INFO("Auto-detected project GUID: {0}", m_projectGuid);
                    break;
                }
            }
        }

        if (m_projectGuid.empty())
        {
            EE_CORE_WARN("Could not auto-detect project GUID in database");
            return false;
        }
    }

    // Build path to resource database file
    std::string resourceDbPath = m_databasePath + "/" + m_projectGuid + "/Browser.dbase/resource_database.txt";

    if (!std::filesystem::exists(resourceDbPath))
    {
        EE_CORE_WARN("Resource database file not found: {0}", resourceDbPath);
        return false;
    }

    // Parse the resource database
    std::ifstream dbFile(resourceDbPath);
    if (!dbFile.is_open())
    {
        EE_CORE_ERROR("Failed to open resource database file: {0}", resourceDbPath);
        return false;
    }

    m_resourceDatabase.clear();

    std::string line;
    ResourceEntry currentEntry = {};
    bool readingEntry = false;

    while (std::getline(dbFile, line))
    {
        if (line.find("RESOURCE_START") == 0)
        {
            readingEntry = true;
            currentEntry = ResourceEntry{};
        }
        else if (line.find("RESOURCE_END") == 0)
        {
            if (readingEntry && !currentEntry.sourcePath.empty())
            {
                // Store using normalized source path as key
                std::string normalizedPath = ConvertToRelativePath(currentEntry.sourcePath);
                m_resourceDatabase[normalizedPath] = currentEntry;

                EE_CORE_TRACE("Loaded resource: {0} -> GUID 0x{1:x}",
                    normalizedPath, currentEntry.instanceGUID);
            }
            readingEntry = false;
        }
        else if (readingEntry)
        {
            if (line.find("InstanceGUID=") == 0)
            {
                currentEntry.instanceGUID = std::stoull(line.substr(13), nullptr, 16);
            }
            else if (line.find("TypeGUID=") == 0)
            {
                currentEntry.typeGUID = std::stoull(line.substr(9), nullptr, 16);
            }
            else if (line.find("SourcePath=") == 0)
            {
                currentEntry.sourcePath = line.substr(11);
            }
            else if (line.find("OutputPath=") == 0)
            {
                currentEntry.outputPath = line.substr(11);
            }
            else if (line.find("LastModified=") == 0)
            {
                auto timeVal = std::stoull(line.substr(13));
                currentEntry.lastModified = std::filesystem::file_time_type(
                    std::chrono::duration<uint64_t>(timeVal));
            }
        }
    }

    m_databaseLoaded = true;
    return true;
}

/**
 * @brief Reload the resource database
 * @return true if reload successful
 */
bool AssetManager::ReloadResourceDatabase()
{
    EE_CORE_INFO("Reloading resource database...");
    m_databaseLoaded = false;
    return LoadResourceDatabase();
}

/**
 * @brief Find a resource entry by source path
 * @param sourcePath The original source path (PNG file path)
 * @return Resource entry if found, nullptr otherwise
 */
ResourceEntry* AssetManager::FindResourceBySourcePath(const std::string& sourcePath)
{
    // Normalize the path for lookup
    std::string normalizedPath = ConvertToRelativePath(sourcePath);

    auto it = m_resourceDatabase.find(normalizedPath);
    if (it != m_resourceDatabase.end())
    {
        return &it->second;
    }

    // Try with different path separators
    std::string altPath = normalizedPath;
    std::replace(altPath.begin(), altPath.end(), '/', '\\');
    it = m_resourceDatabase.find(altPath);
    if (it != m_resourceDatabase.end())
    {
        return &it->second;
    }

    std::replace(altPath.begin(), altPath.end(), '\\', '/');
    it = m_resourceDatabase.find(altPath);
    if (it != m_resourceDatabase.end())
    {
        return &it->second;
    }

    return nullptr;
}

/**
 * @brief Convert absolute path to relative path for database lookup
 * @param absolutePath The absolute file path
 * @return Relative path for database lookup
 */
std::string AssetManager::ConvertToRelativePath(const std::string& absolutePath)
{
    // Convert backslashes to forward slashes for consistency
    std::string normalized = absolutePath;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');

    // Remove leading "./" if present
    if (normalized.starts_with("./"))
    {
        normalized = normalized.substr(2);
    }

    return normalized;
}

/**
 * @brief Load a texture from the file path and store it in the asset manager, if it is loaded before, return the texture
 * @param filePath The file path of the texture
 * @return The texture that is loaded
 */
std::shared_ptr<graphics::Texture> AssetManager::LoadTexture(const std::string& filePath)
{
    
    EE_CORE_TRACE("Loading texture: {0}", filePath);

    // Check cache first using original file path as key
    auto it = m_textures.find(filePath);
    if (it != m_textures.end())
    {
        EE_CORE_TRACE("Texture found in cache: {0}", filePath);
        return it->second;
    }

    std::shared_ptr<graphics::Texture> texture;

    // Try to load from resource database first
    if (m_databaseLoaded)
    {
        ResourceEntry* resourceEntry = FindResourceBySourcePath(filePath);
        if (resourceEntry != nullptr)
        {
            EE_CORE_TRACE("Found resource in database: {0} -> DDS path: {1}",
                filePath, resourceEntry->outputPath);

            // Create texture from DDS file
            texture = std::make_shared<graphics::Texture>();
            if (texture->LoadFromDDS(resourceEntry->outputPath))
            {
                EE_CORE_INFO("Texture loaded from pipeline DDS: {0}", filePath);
                m_textures[filePath] = texture;
                return texture;
            }
            else
            {
                EE_CORE_WARN("Failed to load DDS file: {0}, falling back to direct load",
                    resourceEntry->outputPath);
            }
        }
        else
        {
            EE_CORE_TRACE("Resource not found in database: {0}, trying direct load", filePath);
        }
    }

    // Fallback: Load directly from source file (PNG)
    texture = std::make_shared<graphics::Texture>(filePath);
    if (!texture->IsValid())
    {
        EE_CORE_ERROR("Failed to load texture: {0}", filePath);
        return nullptr;
    }

    m_textures[filePath] = texture;
    EE_CORE_INFO("Texture loaded directly: {0}", filePath);
    return texture;
}

/**
 * @brief Load a texture directly by GUID
 * @param instanceGUID The instance GUID of the texture resource
 * @return The loaded texture
 */
std::shared_ptr<graphics::Texture> AssetManager::LoadTextureByGUID(uint64_t instanceGUID)
{
    EE_CORE_TRACE("Loading texture by GUID: 0x{0:x}", instanceGUID);

    // Create cache key from GUID
    std::string cacheKey = "GUID_" + std::to_string(instanceGUID);

    auto it = m_textures.find(cacheKey);
    if (it != m_textures.end())
    {
        return it->second;
    }

    if (!m_databaseLoaded)
    {
        EE_CORE_ERROR("Cannot load by GUID: resource database not loaded");
        return nullptr;
    }

    // Find the resource by GUID
    for (const auto& [sourcePath, entry] : m_resourceDatabase)
    {
        if (entry.instanceGUID == instanceGUID)
        {
            auto texture = std::make_shared<graphics::Texture>();
            if (texture->LoadFromDDS(entry.outputPath))
            {
                m_textures[cacheKey] = texture;
                EE_CORE_INFO("Texture loaded by GUID 0x{0:x} from: {1}", instanceGUID, entry.outputPath);
                return texture;
            }
            break;
        }
    }

    EE_CORE_ERROR("Failed to load texture by GUID: 0x{0:x}", instanceGUID);
    return nullptr;
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
