/* Start Header ************************************************************************/
/*!
\file       AssetManager.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu (80%)   
\co-author  Jeremy Lim Ting Jie, jeremytingjie.lim, 2301370, jeremytingjie.lim\@digipen.edu (20%)
\co-authors Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       10/09/2025
\brief      This reflects the brief of the AssetManager system.
            This file is used to manage all the assets in the game.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include "Shader.h"
#include "Texture.h"
#include "Cubemap.h"


#include "Model.h"

#include <assimp/Importer.hpp>  // for the importer class
#include <assimp/scene.h>       // for the output data structure
#include <assimp/postprocess.h> // for post processing flags


// Forward declaration to avoid circular includes
namespace Ermine::graphics {
    class Material;
}
namespace Ermine
{
    /**
     * @brief The AssetManager class is a singleton class that manages all the assets in the game.
     *        This includes textures and shaders.
     */
    class AssetManager
    {
        AssetManager() = default; // Private constructor for singleton
        ~AssetManager() = default;

        // Internal caching for assets
        std::unordered_map<std::string, std::shared_ptr<graphics::Texture>> m_textures;
        std::unordered_map<std::string, std::shared_ptr<graphics::Shader>> m_shaders;
        std::unordered_map<std::string, std::shared_ptr<graphics::Cubemap>> m_cubemaps;
        std::unordered_map<std::string, std::shared_ptr<graphics::Material>> m_materials;
        std::unordered_map<std::string, std::shared_ptr<graphics::Model>> m_models;
    
public:
        static AssetManager& GetInstance()
        {
            static AssetManager instance;
            return instance;
        }

        // ================== Texture Management ==================
        /**
         * @brief Load a texture from a file
         * @param filePath The path to the texture file
         * @return The loaded texture
         */
        std::shared_ptr<graphics::Texture> LoadTexture(const std::string& filePath);
        /**
         * @brief Get a texture from the cache
         * @param filePath The path to the texture file
         * @return The texture if it exists, nullptr otherwise
         */
        std::shared_ptr<graphics::Texture> GetTexture(const std::string& filePath);

        // ================== Shader Management ==================
        const std::unordered_map<std::string, std::shared_ptr<graphics::Texture>>& GetLoadedTextures() const;
        /**
         * @brief Load a shader from a vertex and fragment file
         * @param vertexPath The path to the vertex shader file
         * @param fragmentPath The path to the fragment shader file
         * @return The loaded shader
         */
        std::shared_ptr<graphics::Shader> LoadShader(const std::string& vertexPath, const std::string& fragmentPath);
        /**
         * @brief Get a shader from the cache
         * @param shaderName The name of the shader
         * @return The shader if it exists, nullptr otherwise
         */
        std::shared_ptr<graphics::Shader> GetShader(const std::string& shaderName);

        // ================== Model Management ==================
        /**
         * @brief Load a 3D model from file using Assimp.
         * @param filePath The path to the model file (e.g. .fbx, .obj, .gltf).
         * @return The loaded model.
         */
        std::shared_ptr<graphics::Model> LoadModel(const std::string& filePath);
        /**
         * @brief Get a model from the cache.
         * @param filePath The path to the model file.
         * @return The model if it exists, nullptr otherwise.
         */
        std::shared_ptr<graphics::Model> GetModel(const std::string& filePath);

        // ================== Utilities ==================
        /**
         * @brief Load the contents of a file into a buffer.
         * @param filepath The path to the file to load.
         * @return The contents of the file as a buffer.
         */
        const char* load_file_contents(const char* filepath);

        // Cubemap management
        /**
         * @brief Load a cubemap from individual face textures
         * @param faces Array of 6 face texture paths in order: +X, -X, +Y, -Y, +Z, -Z
         * @param name Optional name for the cubemap (for caching)
         * @return The loaded cubemap
         */
        std::shared_ptr<graphics::Cubemap> LoadCubemap(const std::array<std::string, 6>& faces, const std::string& name = "");
        
        /**
         * @brief Load a cubemap from an equirectangular texture
         * @param equirectangularPath Path to the equirectangular texture
         * @param name Optional name for the cubemap (for caching)
         * @return The loaded cubemap
         */
        std::shared_ptr<graphics::Cubemap> LoadCubemapFromEquirectangular(const std::string& equirectangularPath, const std::string& name = "");
        
        /**
         * @brief Get a cubemap from the cache
         * @param name The name of the cubemap
         * @return The cubemap if it exists, nullptr otherwise
         */
        std::shared_ptr<graphics::Cubemap> GetCubemap(const std::string& name);

        // Material management
        /**
         * @brief Create and cache a material with the given name
         * @param name The name/key for the material
         * @param shader The shader to use for the material
         * @param materialTemplate Optional material template to apply
         * @return The created material
         */
        std::shared_ptr<graphics::Material> CreateMaterial(const std::string& name, 
                                                         std::shared_ptr<graphics::Shader> shader,
                                                         const std::string& materialTemplate = "");
        
        /**
         * @brief Get a material from the cache
         * @param name The name of the material
         * @return The material if it exists, nullptr otherwise
         */
        std::shared_ptr<graphics::Material> GetMaterial(const std::string& name);
        
        /**
         * @brief Create a shared material for common use cases
         * @param materialType Type of material (e.g., "wood", "metal", "plastic")
         * @param shader The shader to use
         * @param baseTexture Optional base texture
         * @return The created shared material
         */
        std::shared_ptr<graphics::Material> CreateSharedMaterial(const std::string& materialType,
                                                               std::shared_ptr<graphics::Shader> shader,
                                                               std::shared_ptr<graphics::Texture> baseTexture = nullptr);

        // Clear all loaded assets
        void Clear();
    };
}
