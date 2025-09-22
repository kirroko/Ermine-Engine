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
#include "EditorCamera.h"

namespace Ermine::graphics
{

    // Transparency rendering support
    struct TransparentObject {
        EntityID entity;
        float distanceToCamera;
        glm::mat4 modelMatrix;

        bool operator<(const TransparentObject& other) const {
            return distanceToCamera > other.distanceToCamera; // Sort back-to-front
        }
    };

    // Lights

    constexpr int MaxLights = 16;
    constexpr int NUM_CASCADES = 4;
	constexpr int SHADOW_MAX_LAYERS = 32;
	constexpr int SHADOW_MAP_RESOLUTION = 4096;
    constexpr float SHADOW_MAP_ARRAY_LAMBDA = 0.95f;

    class LightSystem : public System {};

    /*!***********************************************************************
    \brief
     Light GPU structure
    *************************************************************************/
    struct LightGPU
    {
        glm::vec4 position_type;    // xyz = position (view space), w = light type
        glm::vec4 color_intensity;  // xyz = color, w = intensity
        glm::vec4 direction_range;  // xyz = direction (view space), w = range
		glm::vec4 spot_angles_castshadows_startOffset; // x = inner angle (cos), y = outer angle (cos), z = cast shadows (bool), w = shadow map index or 0 if no shadows
        glm::mat4 lightSpaceMatrix[NUM_CASCADES];
		glm::vec4 splitDepths; // split depths for cascaded shadow maps xyzw
    };


    // Forward declarations
    struct MaterialUBO;
    class Skybox;

    /**
     * @brief The Renderer class is responsible for rendering the game objects to the screen.
     */
    class Renderer : public System
    {
    public:
        GLuint m_ShadowMapFBO = 0;
        GLuint m_ShadowMapArray = 0;

        // Lighting Pass Parameters
        bool m_SSAOEnabled = false;

        // Post-processing uniforms - toggles
        bool m_VignetteEnabled = true;
        bool m_FXAAEnabled = true;
        bool m_ToneMappingEnabled = true;
        bool m_GammaCorrectionEnabled = true;
        bool m_BloomEnabled = true;
        bool m_SkyBoxisHDR = false;

        // Post-processing uniforms - parameters
        float m_Exposure = 1.0f;
        float m_Contrast = 1.0f;
        float m_Saturation = 1.0f;
        float m_Gamma = 2.2f;
        float m_VignetteIntensity = 0.3f;
        float m_VignetteRadius = 0.8f;
        float m_BloomStrength = 0.04f;

        // FXAA parameters
        float m_FXAASpanMax = 8.0f;
        float m_FXAAReduceMin = 1.0f / 128.0f;
        float m_FXAAReduceMul = 1.0f / 8.0f;

        // Bloom pass parameters
        float m_BloomThreshold = 1.0f;
        float m_BloomIntensity = 2.0f;
        float m_BloomRadius = 5.0f;


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
        
         /**
		 * @brief G buffer structure for rendering to Lighting pass
		 */
        //~Renderer();
        struct GBuffer
        {
            unsigned int FBO;
            unsigned int DepthTexture;

            // Multiple Render Targets (MRTs)

            uint64_t HandlePackedTexture0 = 0;
            uint64_t HandlePackedTexture1 = 0;
            uint64_t HandlePackedTexture2 = 0;
            uint64_t HandlePackedTexture3 = 0;
            uint64_t HandleDepthTexture = 0;
            

            unsigned int PackedTexture0;
            unsigned int PackedTexture1;
			unsigned int PackedTexture2;
			unsigned int PackedTexture3;
            


            int width;
            int height;
        };


         /**
		 * @brief PostProcessing buffer structure for each post-processing effect
		 */
		struct PostProcessBuffer
        {
			unsigned int FBO;
			unsigned int ColorTexture;
			unsigned int DepthTexture = 0; // Optional depth texture for skybox rendering

			int width;
			int height;
        };


        /**
		 * @brief Destructor - cleans up allocated resources
         */
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
         * @brief Create  g-buffer for deferred rendering
         * @param width The width of the g-buffer
         * @param height The height of the g-buffer
         */
        void CreateGBuffer(const int& width, const int& height);


        /**
		 * @brief Create post-processing buffer
		 * @param width The width of the post-processing buffer
		 * @param height The height of the post-processing buffer
         */
        void CreatePostProcessBuffer(const int& width, const int& height);

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
		 * @brief Render Post-processing effects using the lighting pass output
         */
        void RenderPostProcessPass();

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
        void BindGBufferTextures();

        std::shared_ptr<OffscreenBuffer> GetOffscreenBuffer() const { return m_OffscreenBuffer; }
        std::shared_ptr<GBuffer> GetGBuffer() const { return m_GBuffer; }

         /**
         * @brief Cleanup g-buffer resources
         */
        void CleanupGBuffer();

        /**
        * @brief Cleanup postprocess buffer resources
        */
        void CleanupPostProcessBuffer();

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

        /**
		 * @brief Toggles the flag for using deferred rendering.
         */
        void ToggleDeferredRendering();

        /**
         * @brief Set the skybox to be rendered
         * @param skybox Pointer to the skybox to render
         */
        void SetSkybox(graphics::Skybox* skybox) { m_skybox = skybox; }

        void RenderModel(const Model& model, const Mtx44& view, const Mtx44& projection, const glm::mat4& rootTransform);


        // Shadow mapping
        bool InitializeShadowMap();
        bool CreateShadowMapArray();
        bool CreateShadowMapCube(const unsigned int resolution);
        void CalculateDirectionalMatrix(const editor::EditorCamera& editorCamera);
		void RenderShadowMap();
        void RenderShadowPass();

        /**
         * @brief Render transparent objects using forward rendering with depth peeling
         * @param view The view matrix
         * @param projection The projection matrix
         */
        void RenderTransparentPass(const Mtx44& view, const Mtx44& projection);

        /**
         * @brief Sort transparent objects by distance from camera
         * @param cameraPos Camera position in world space
         */
        void SortTransparentObjects(const Vec3& cameraPos);

        /**
         * @brief Check if material is transparent based on transparency value
         * @param material The material to check
         * @return true if material should be rendered in transparent pass
         */
        bool IsTransparentMaterial(const Ermine::graphics::Material* material) const;


    private:
		// Light System
		std::shared_ptr<LightSystem> m_LightSystem = nullptr;
        std::shared_ptr<OffscreenBuffer> m_OffscreenBuffer;

        // Lighting UBO
        GLuint m_LightsUBO = 0;
        static constexpr GLuint LightsBindingPoint = 1;
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


		// Post-processing buffer
		std::shared_ptr<PostProcessBuffer> m_PostProcessBuffer;
		std::shared_ptr<PostProcessBuffer> m_BloomExtractBuffer;
        std::shared_ptr<PostProcessBuffer> m_BloomBlurBuffer1;
        std::shared_ptr<PostProcessBuffer> m_BloomBlurBuffer2;
		std::shared_ptr<Shader> m_BloomShader = 0; // Shader for bloom effect
		std::shared_ptr<Shader> m_PostProcessShader = 0; // Shader for post-processing effects

		// Skybox
		graphics::Skybox* m_skybox = nullptr;

        // Shadow mapping
        std::shared_ptr<Shader> m_ShadowMapGeometryShader = nullptr;
        // Just one FBO and one 2D shadow map for one directional light for now
        GLuint m_ShadowMapCube = 0;
        uint64_t m_ShadowMapArrayHandle = 0;
        std::vector<TransparentObject> m_transparentObjects;

        // Forward rendering shader for transparent objects
        std::shared_ptr<Shader> m_ForwardShader = nullptr;

        void BindMaterialTextures(Ermine::graphics::Material* material);


    };
}