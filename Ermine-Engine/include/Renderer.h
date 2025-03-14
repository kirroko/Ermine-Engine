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
        /**
         * @brief Update the game objects to the screen.
         * 
         * @param view The view matrix
         * @param projection The projection matrix
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
    };
}
