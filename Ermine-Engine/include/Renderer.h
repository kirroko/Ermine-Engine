/* Start Header ************************************************************************/
/*!
\file       Renderer.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       09/03/2025
\brief      This file contains the declaration of the Renderer system.
            This file is used to render the game objects to the screen.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Shader.h"
#include "Systems.h"
#include "GPUProfiler.h"
#include "Texture.h"
#include "Components.h"

namespace Ermine::graphics
{
    /**
     * @brief The Renderer class is responsible for rendering the game objects to the screen.
     */
    class Renderer : public System
    {
    public:
        struct OffscreenBuffer
        {
			unsigned int FBO; // Frame Buffer Object
            unsigned int ColorTexture;
			unsigned int RBO; // Render Buffer Object

            // Viewport size
            int width;
            int height;
        };

        struct InstanceData {
            glm::mat4 model; // per-entity transform
            glm::mat3 normalMat; // per-entity normal matrix
            //glm::vec4 colour; // optional tint
        };

        // group by mesh pointer, shader, texture
        struct BatchKey {
            //Mesh* k_mesh;
            const graphics::VertexArray* k_vao;
            const graphics::IndexBuffer* k_ibo;

            std::shared_ptr<Shader> k_shader;
            std::shared_ptr<Texture> k_texture;

            bool operator<(const BatchKey& other) const {
                if (k_vao != other.k_vao) return k_vao < other.k_vao;
                if (k_ibo != other.k_ibo) return k_ibo < other.k_ibo;
                if (k_shader != other.k_shader) return k_shader < other.k_shader;
                return k_texture < other.k_texture;
            }
        };

        //~Renderer();

        /**
		 * @brief Create an offscreen buffer for viewport/scene rendering
		 * @param width The width of the offscreen buffer
		 * @param height The height of the offscreen buffer
		 * @return OffscreenBuffer The offscreen buffer
         */
        OffscreenBuffer Create(const int& width, const int& height);

		std::shared_ptr<OffscreenBuffer> GetOffscreenBuffer() const { return m_OffscreenBuffer; }

        /**
         * @brief Update the game objects to the screen.
         */
        void Update(const Mtx44& view, const Mtx44& projection);

        void UpdateWithBatchRender(const Mtx44& view, const Mtx44& projection);
        
        /**
         * @brief Draw the game objects to the screen.
         */
        void Draw(const std::shared_ptr<VertexArray>& vao, const std::shared_ptr<IndexBuffer>& ibo, const std::shared_ptr<Shader>& shader) const;

        /**
         * @brief Clear the screen.
         */
        void Clear() const;

        /**
         * @brief Get the current performance metrics
         * @return CurrentGPU performance metrics
         */
        const GPUProfiler::PerformanceMetrics& GetPerformanceMetrics() const;

    private:
		std::shared_ptr<OffscreenBuffer> m_OffscreenBuffer;
        unsigned int m_InstanceVBO = 0;
    };
}
