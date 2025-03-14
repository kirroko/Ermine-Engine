/* Start Header ************************************************************************/
/*!
\file       VertexBuffer.h
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This file contains the declaration of the VertexBuffer system.
            This file is used to create a VertexBuffer using OpenGL.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/

#pragma once
#include <glad/glad.h>

namespace Ermine::graphics
{
    /**
     * @brief VertexBuffer class
     */
    class VertexBuffer
    {
        GLuint m_RendererID;
    public:
        /**
         * @brief Construct a new Vertex Buffer object
         * @param data The data to be stored in the buffer
         * @param size The size of the data
         */
        VertexBuffer(const void* data, unsigned int size);
        /**
         * @brief Destroy the Vertex Buffer object
         */
        ~VertexBuffer();

        /**
         * @brief Bind the buffer
         */
        void Bind() const;
        /**
         * @brief Unbind the buffer
         */
        void Unbind() const;
    };
    
}
