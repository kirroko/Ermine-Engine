/* Start Header ************************************************************************/
/*!
\file       AssetManager.h
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This reflects the brief of the AssetManager system.
            This file is used to manage all the assets in the game.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/

#pragma once

#include "Shader.h"
#include "Texture.h"

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
    
public:
        static AssetManager& GetInstance()
        {
            static AssetManager instance;
            return instance;
        }

        // Texture management
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

        // Shader management
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

        /**
         * @brief Load the contents of a file into a buffer.
         * @param filepath The path to the file to load.
         * @return The contents of the file as a buffer.
         */
        const char* load_file_contents(const char* filepath);

        // Clear all loaded assets
        void Clear();
    };
}
