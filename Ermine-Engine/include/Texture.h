/* Start Header ************************************************************************/
/*!
\file       Texture.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       09/03/2025
\brief      This file contains the declaration of the Texture class.
            This file is used to load and bind textures to the renderer.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"
#include <glad/glad.h>

namespace Ermine::graphics
{
    class Texture
    {
        GLuint m_RendererID;
        std::string m_filePath;
        unsigned char* m_LocalBuffer;
        int m_Width, m_Height, m_BPP;

        void Release(bool contextExpected) noexcept;
    public:
        /**
         * @brief Default Construct
         */
        Texture();
        
        /**
         * @brief Construct a new Texture object
         */
        explicit Texture(const std::string& filePath);

        /**
         * @brief Destroy the Texture object
         */
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture && other) noexcept;
        Texture& operator=(Texture&& other) noexcept;

        /**
         * @brief check if the texture is valid
         * @return true if the texture is valid, false otherwise
         */
        bool IsValid() const;

        /**
         * @brief Bind the texture
         * @param slot the slot to bind the texture to
         */
        void Bind(unsigned int slot = 0) const;
        /**
         * @brief Unbind the texture
         */
        void Unbind() const;
    };
}
