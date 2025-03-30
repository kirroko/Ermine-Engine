/* Start Header ************************************************************************/
/*!
\file       Renderer.h
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This file contains the declaration of the Renderer system.
            This file is used to render the game objects to the screen.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/

#pragma once
#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Shader.h"
#include "Systems.h"

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

        ~Renderer();

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
        
        /**
         * @brief Draw the game objects to the screen.
         */
        void Draw(const std::shared_ptr<VertexArray>& vao, const std::shared_ptr<IndexBuffer>& ibo, const std::shared_ptr<Shader>& shader) const;

        /**
         * @brief Clear the screen.
         */
        void Clear() const;
    private:
		std::shared_ptr<OffscreenBuffer> m_OffscreenBuffer;
    };
}
