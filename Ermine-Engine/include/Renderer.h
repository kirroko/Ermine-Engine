/* Start Header ************************************************************************/
/*!
\file       Renderer.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\co-author  Jeremy Lim Ting Jie, jeremytingjie.lim, 2301370, jeremytingjie.lim\@digipen.edu
\co-author  Ridhwan
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
#include "Material.h" // Add this include

namespace Ermine::graphics
{
    // Forward declarations
    struct MaterialUBO;

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

        /**
         * @brief Get the current performance metrics
         * @return CurrentGPU performance metrics
         */
        const GPUProfiler::PerformanceMetrics& GetPerformanceMetrics() const;
        /**
         * @brief Set the shading mode for rendering.
         * @param isBlinnPhong If true, enables Blinn-Phong shading; if false, uses PBR shading.
         */
        void SetShadingMode(bool isBlinnPhong) { m_IsBlinnPhong = isBlinnPhong; }
        /**
         * @brief Get the current shading mode.
         * @return True if Blinn-Phong shading is enabled, false if PBR shading is used.
         */
        bool GetShadingMode() const { return m_IsBlinnPhong; }
        /**
         * @brief Updates the lights' uniform buffer object (UBO) with the current light and transform data from all living entities.
         * @param view The view matrix to transform the positions and directions of the lights into view space.
         */
        void UpdateLightsUBO(const Mtx44& view);
        /**
         * @brief Binds the Lights uniform block to the specified shader program if it has not been bound before.
         * @param shader The shader program to which the lights block should be bound.
         */
        void BindLightsBlockIfPresent(const std::shared_ptr<Shader>& shader);
        /**
         * @brief Updates the material's uniform buffer object (UBO) with the specified material data.
         * @param materialData The material data to be uploaded to the UBO, including properties like color, texture, etc.
         */
        void UpdateMaterialUBO(const MaterialUBO& materialData);
        /**
         * @brief Binds the MaterialBlock uniform block to the specified shader program if it has not been bound before.
         * @param shader The shader program to which the material block should be bound.
         */
        void BindMaterialBlockIfPresent(const std::shared_ptr<Shader>& shader);

    private:
        std::shared_ptr<OffscreenBuffer> m_OffscreenBuffer;

        // Lighting UBO
        GLuint m_LightsUBO = 0;
        static constexpr GLuint LightsBindingPoint = 1;
        static constexpr size_t MaxLights = 16;
        std::unordered_set<GLuint> m_LightBlockBoundPrograms;
        bool m_IsBlinnPhong = true; // Default to Blinn-Phong

        // Material UBO
        GLuint m_MaterialUBO = 0;
        static constexpr GLuint MaterialBindingPoint = 2;
        std::unordered_set<GLuint> m_MaterialBlockBoundPrograms;
    };
}