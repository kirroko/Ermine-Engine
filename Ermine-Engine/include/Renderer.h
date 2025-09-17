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
#include "Material.h"
#include "Components.h" 

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
        /**
         * @brief Initialize the renderer with the screen width and height.
         * @param screenWidth The width of the screen
         * @param screenHeight The height of the screen
		 */
		void Init(const int& screenWidth, const int& screenHeight);

        /**
         * @brief Offscreen buffer structure for rendering to texture
		 */
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
        struct GBuffer
        {
            unsigned int FBO;
            unsigned int DepthTexture;
			// Multiple Render Targets (MRTs)
            
            // RT0: RGB32_UINT - 96 bits total
            // R32: Albedo RGB 8:8:8 + 8 spare bits
            // G32: Normal RGB 11:10:11 
            // B32: Emissive RGBE 9:9:9:5
            unsigned int PackedTexture0;

            // RT1: RG32_UINT - 64 bits total  
			// R32: Roughness 8 bits + Metallic 8 bits + AO 8 bits + 8 spare bits
            // G32: Motion vectors 2x16 bits
            unsigned int PackedTexture1;


            int width;
            int height;
        };

        enum GBufferTextureType
        {
            GBufferPacked0 = 0,    // RT0: Albedo + Normal + Emissive
            GBufferPacked1 = 1,    // RT1: Material properties + Motion vectors
            GBufferDepth = 2,      // Depth buffer
            GBufferCOUNT = 3
        };

        ~Renderer();


        /**
         * @brief Create an offscreen buffer for viewport/scene rendering
         * @param width The width of the offscreen buffer
         * @param height The height of the offscreen buffer
         * @return OffscreenBuffer The offscreen buffer
         */
        OffscreenBuffer CreateOffscreenBuffer(const int& width, const int& height);

        /**
         * @brief Resize the offscreen buffer to new dimensions without recreating the FBO
         * @param width New width
         * @param height New height
		 */
        void ResizeOffscreenBuffer(const int& width, const int& height);

        /**
         * @brief Create optimized g-buffer for deferred rendering
         * @param width The width of the g-buffer
         * @param height The height of the g-buffer
         * @return GBuffer The g-buffer structure
         */
        GBuffer CreateGBuffer(const int& width, const int& height);

        /**
         * @brief Resize the g-buffer to new dimensions
         * @param width New width
         * @param height New height
         */
        void ResizeGBuffer(const int& width, const int& height);

        /**
         * @brief Begin geometry pass for deferred rendering
         * Clear g-buffer and set up for geometry rendering
         */
        void BeginGeometryPass();

        /**
         * @brief End geometry pass and prepare for lighting pass
         */
        void EndGeometryPass();

        /**
         * @brief Begin lighting pass for deferred rendering
         * Bind g-buffer textures and set up for lighting calculations
         */
        void BeginLightingPass();

        /**
         * @brief End lighting pass and finalize frame
         */
        void EndLightingPass();

        /**
         * @brief Render a geometry pass for deferred rendering
         * This function handles the CPU-side setup and issues draw calls
         * The actual g-buffer writing happens in the geometry fragment shader
         * @param view The view matrix
         * @param projection The projection matrix
         */
        void RenderGeometryPass(const Mtx44& view, const Mtx44& projection);

        /**
         * @brief Render lighting pass for deferred rendering
         * Reads from g-buffer textures and performs lighting calculations on GPU
         * @param view The view matrix
         * @param projection The projection matrix
         */
        void RenderLightingPass(const Mtx44& view, const Mtx44& projection);

        /**
         * @brief Complete deferred rendering pipeline
         * @param view The view matrix
         * @param projection The projection matrix
         */
        void RenderDeferredPipeline(const Mtx44& view, const Mtx44& projection);

        /**
         * @brief Bind g-buffer textures to specified texture units
         * @param startingTextureUnit The first texture unit to bind to (default: 0)
         */
        void BindGBufferTextures(int startingTextureUnit = 0);

        std::shared_ptr<OffscreenBuffer> GetOffscreenBuffer() const { return m_OffscreenBuffer; }
        std::shared_ptr<GBuffer> GetGBuffer() const { return m_GBuffer; }

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

        void ToggleDeferredRendering();


    private:
        std::shared_ptr<OffscreenBuffer> m_OffscreenBuffer;

        // Lighting UBO
        GLuint m_LightsUBO = 0;
        static constexpr GLuint LightsBindingPoint = 1;
        static constexpr size_t MaxLights = 16;
        std::unordered_set<GLuint> m_LightBlockBoundPrograms;
		bool m_IsBlinnPhong = false; // Default to PBR shading

        // Material UBO
        GLuint m_MaterialUBO = 0;
        static constexpr GLuint MaterialBindingPoint = 2;
        std::unordered_set<GLuint> m_MaterialBlockBoundPrograms;


		// Deferred rendering buffers
		bool m_UseDeferredRendering = true;
		Ermine::Mesh m_QuadMesh;
        std::shared_ptr<GBuffer> m_GBuffer;
		std::shared_ptr<Shader> m_GBufferShader = 0; // Shader for executing g-buffer pass
        std::shared_ptr<Shader> m_LightPassShader = 0; // Shader for lighting pass
        void CleanupGBuffer();
        std::shared_ptr<Texture> tempTexture;
    };
}