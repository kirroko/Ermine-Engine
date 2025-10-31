/* Start Header ************************************************************************/
/*!
\file       Renderer.h
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\co-author  Jeremy Lim Ting Jie, jeremytingjie.lim, 2301370, jeremytingjie.lim\@digipen.edu
\co-author  Ridhwan Afandi, mohamedridhwan.b, 2301367, mohamedridhwan.b\@digipen.edu
\co-author  Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       27/09/2025
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
#include "shadow_config.h"
#include "MeshManager.h"

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

    /*!***********************************************************************
    \brief Light System. Contains all light entities in the scene.
    *************************************************************************/
    class LightSystem : public System {};

    /*!***********************************************************************
    \brief Light GPU structure
    *************************************************************************/
    struct LightGPU
    {
        glm::vec4 position_type;    // xyz = position (view space), w = light type
        glm::vec4 color_intensity;  // xyz = color, w = intensity
        glm::vec4 direction_range;  // xyz = direction (view space), w = range
		glm::vec4 spot_angles_castshadows_startOffset; // x = inner angle (cos), y = outer angle (cos), z = cast shadows (bool), w = shadow map index or 0 if no shadows
        glm::mat4 lightSpaceMatrix[NUM_CASCADES];
		glm::vec4 splitDepths[(NUM_CASCADES+3)/4]; // split depths for cascaded shadow maps xyzw
    };


    // Forward declarations
    struct MaterialSSBO;
    class Skybox;

    //physic wireframe
    struct DebugVertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    /**
     * @brief The Renderer class is responsible for rendering the game objects to the screen.
     */
    class Renderer : public System
    {
    public:
		// Mesh Manager - Composition
		MeshManager m_MeshManager;

        // Lighting Pass Parameters
        // SSAO parameters
        bool m_SSAOEnabled = false;
		int  m_SSAOSamples = 16;
		float m_SSAORadius = 10.0f;
		float m_SSAOBias = 0.01f;
		float m_SSAOIntensity = 1.0f;
		float m_SSAOFadeout = 0.1f;
		float m_SSAOMaxDistance = 100.0f;

        // Post-processing uniforms - toggles
        bool m_VignetteEnabled = false;
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
        float m_BloomRadius = 1.0f;

        // Maximum bone array size expected in shader
        static constexpr int MAX_BONE_UNIFORMS = 128;

        /**
         * @brief To run the pass and read GL_STENCIL_INDEX at a pixel
         * @param x The x coordinate from the framebuffer
         * @param y The y coordinate from the framebuffer
         * @param view camera view matrix
         * @param projection camera projection matrix
         * @return
         */
        std::pair<bool, EntityID> PickEntityAt(const int& x, const int& y, const Mtx44& view, const Mtx44& projection);

        /**
		 * @brief Update the shadow maps for all lights that cast shadows.
		 */
        void InitializeShadowMapResources();
        /**
         * @brief Initialize the renderer with the screen width and height.
         * @param screenWidth The width of the screen
         * @param screenHeight The height of the screen
         */
        void Init(const int& screenWidth, const int& screenHeight);

        //PHYSICS
        void SubmitDebugLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);
        void RenderDebugLines(const glm::mat4& view, const glm::mat4& proj);
        void RenderDebugLines(const Mtx44& view, const Mtx44& proj);

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


        //struct InstanceData {
        //    glm::mat4 model; // per-entity transform
        //    glm::mat3 normalMat; // per-entity normal matrix
        //    //glm::vec4 colour; // optional tint
        //};

        //// group by mesh pointer, shader, texture
        //struct BatchKey {
        //    //Mesh* k_mesh;
        //    const graphics::VertexArray* k_vao;
        //    const graphics::IndexBuffer* k_ibo;

        //    std::shared_ptr<Shader> k_shader;
        //    std::shared_ptr<Texture> k_texture;

        //    bool operator<(const BatchKey& other) const {
        //        if (k_vao != other.k_vao) return k_vao < other.k_vao;
        //        if (k_ibo != other.k_ibo) return k_ibo < other.k_ibo;
        //        if (k_shader != other.k_shader) return k_shader < other.k_shader;
        //        return k_texture < other.k_texture;
        //    }
        //};

        /**
        * @brief G buffer structure for rendering to Lighting pass
        */
        //~Renderer();
        struct GBuffer
        {
            unsigned int FBO = 0;
            unsigned int DepthTexture = 0;

            // Multiple Render Targets (MRTs)

            uint64_t HandlePackedTexture0 = 0;
            uint64_t HandlePackedTexture1 = 0;
            uint64_t HandlePackedTexture2 = 0;
            uint64_t HandlePackedTexture3 = 0;
            uint64_t HandleDepthTexture = 0;


            unsigned int PackedTexture0 = 0;
            unsigned int PackedTexture1 = 0;
            unsigned int PackedTexture2 = 0;
            unsigned int PackedTexture3 = 0;



            int width = 0;
            int height = 0;
        };


        /**
        * @brief PostProcessing buffer structure for each post-processing effect
        */
        struct PostProcessBuffer
        {
			unsigned int FBO = 0;
			unsigned int ColorTexture = 0;
            unsigned int DepthTexture = 0;

			int width = 0;
			int height = 0;
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
        void Draw(const std::shared_ptr<VertexArray>& vao, const std::shared_ptr<IndexBuffer>& ibo) const;

        /**
         * @brief Draw the game objects to the screen using instanced rendering.
         */
        void DrawInstanced(const std::shared_ptr<VertexArray>& vao, const std::shared_ptr<IndexBuffer>& ibo, int instanceCount) const;

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
         * @brief Updates the lights' shader UBO with the current light and transform data from all living entities.
         * @param view The view matrix to transform the positions and directions of the lights into view space.
         */
        void UpdateLightsUBO(const Mtx44& view);
        
        /**
         * @brief Updates the material's shader storage buffer object (SSBO) at a specific index.
         * Used for dynamic material updates after initial compilation.
         * @param materialData The material data to be uploaded to the SSBO.
         * @param materialIndex The index in the material array to update.
         */
        void UpdateMaterialSSBO(const graphics::MaterialSSBO& materialData, uint32_t materialIndex);

        /**
         * @brief Updates the material's SSBO with the specified material data (legacy version).
         * Updates index 0 by default. Prefer using the indexed version.
         * @param materialData The material data to be uploaded to the SSBO.
         */
        void UpdateMaterialSSBO(const graphics::MaterialSSBO& materialData);

        /**
         * @brief Update multiple entities' material albedo color and upload to GPU.
         * @param entities Vector of entity IDs whose materials will be updated.
         * @param color The new albedo color to set for each material.
		 */ 
        void UpdateMultipleEntitiesMaterials(const std::vector<EntityID>& entities,
            const Vec3& color)
        {
            auto& ecs = ECS::GetInstance();
            auto renderer = ecs.GetSystem<Renderer>();

            for (EntityID entity : entities) {
                if (!ecs.HasComponent<Ermine::Material>(entity)) continue;

                auto& materialComp = ecs.GetComponent<Ermine::Material>(entity);
                auto* material = materialComp.GetMaterial();

                if (!material) continue;

                // Update material
                material->SetVec3("materialAlbedo", color);

                // Upload to GPU
                auto ssboData = material->GetSSBOData();
                uint32_t materialIndex = renderer->GetMaterialIndex(entity);
                renderer->UpdateMaterialSSBO(ssboData, materialIndex);
            }
        }

        /**
         * @brief Update an entity's material properties and upload to GPU.
         * @param entity The entity whose material will be updated.
         * @param albedo The new albedo color.
         * @param roughness The new roughness value.
         * @param metallic The new metallic value.
         * @param emissive The new emissive color.
		 */
        void UpdateMaterialColor(EntityID entity,
            const Vec3& albedo,
            float roughness,
            float metallic,
            const Vec3& emissive);

        /**
		* @brief Retrieves the material index for the specified entity.
		* @param entity The entity whose material index to retrieve.
        */
        uint32_t GetMaterialIndex(EntityID entity) const;

        /**
         * @brief Sets the u_MaterialIndex uniform for the entity's material.
         * Call this before each draw call to tell the shader which material to use.
         * @param entity The entity whose material index to set.
         * @param shader The shader program to set the uniform on.
         */
        void SetMaterialIndex(EntityID entity, const std::shared_ptr<Shader>& shader);

        
        /**
         * @brief Compiles all materials from entities with Material and Model components into a single SSBO.
         * This function collects material data from all entities, uploads it to GPU memory, and assigns
         * material indices to each entity for shader access. Should be called once after scene load or
         * when materials are added/removed.
         */
        void CompileMaterials();

        /**
         * @brief Marks materials as dirty, triggering recompilation on next frame.
         * Call this when materials are added, removed, or modified.
         */
        void MarkMaterialsDirty() { m_MaterialsDirty = true; }

        /**
         * @brief Registers a texture in the global texture array.
         * @param texture Shared pointer to the texture.
         * @return The index of the texture in the array, or -1 if registration failed.
         */
        int RegisterTexture(std::shared_ptr<Texture> texture);

        /**
         * @brief Gets the texture array index for a given texture ID.
         * @param textureID The OpenGL texture ID.
         * @return The array index, or -1 if not found.
         */
        int GetTextureArrayIndex(GLuint textureID) const;

        /**
         * @brief Builds the bindless texture array SSBO.
         * This should be called after all textures are registered and before rendering.
         */
        void BuildTextureArray();

        /**
         * @brief Compiles draw commands and draw info for all passes.
         * Routes opaque meshes to geometry/shadow passes, transparent/custom shader meshes to forward pass.
         * Iterates through all entities once and builds DrawElementsIndirectCommand + DrawInfo for all rendering.
         * Should be called every frame.
         */
        void CompileDrawData();

        /**
         * @brief Binds the MaterialBlock shader storage buffer to the specified shader program if it has not been bound before.
         * @param shader The shader program to which the material block should be bound.
         */
        void BindMaterialBlockIfPresent(const std::shared_ptr<Shader>& shader);
        /**
         * @brief Toggles between forward and deferred rendering pipelines.
         *
         * This function flips the internal flag @c m_UseDeferredRendering. When enabled,
         * all rendering will go through the deferred pipeline using a G-buffer and lighting pass.
         * When disabled, rendering falls back to forward shading, where each object is drawn directly
         * with its material and lighting applied in a single pass.
         *
         * It also logs a message indicating the current rendering mode.
         */
        void ToggleDeferredRendering();
        /**
         * @brief Renders a model using the deferred rendering pipeline.
         *
         * In this mode, the function uses the shared G-buffer shader (@c m_GBufferShader) to
         * write geometry data (position, normals, material properties) into the G-buffer.
         * Per-entity materials are not bound as shaders, but their UBO data (albedo, metallic,
         * roughness, emissive, etc.) is uploaded to the GPU so the G-buffer can store them.
         *
         * @param model The model to render, containing mesh geometry and local transforms.
         * @param material Pointer to the material providing UBO data (albedo, metallic, etc.).
         * @param view The view matrix representing the camera transform.
         * @param projection The projection matrix (perspective or orthographic).
         * @param rootTransform Root transform matrix for the entity (translation, rotation, scale).
         */
        void RenderModelDeferred(const Model& model, graphics::Material* material, const Mtx44& view, const Mtx44& projection, const glm::mat4& rootTransform);
        /**
         * @brief Renders a model using the forward rendering pipeline.
         *
         * In this mode, the function binds the entity's own material and its shader, then
         * issues draw calls for each mesh in the model. Lighting and material shading are
         * evaluated directly during rasterization (per-fragment).
         *
         * @param model The model to render, containing mesh geometry and local transforms.
         * @param material Pointer to the material to bind, providing textures and shader.
         * @param view The view matrix representing the camera transform.
         * @param projection The projection matrix (perspective or orthographic).
         * @param rootTransform Root transform matrix for the entity (translation, rotation, scale).
         */
        void RenderModelForward(const Model& model, graphics::Material* material, const Mtx44& view, const Mtx44& projection, const glm::mat4& rootTransform);

        /**
         * @brief Set the skybox to be rendered
         * @param skybox Pointer to the skybox to render
         */
        void SetSkybox(graphics::Skybox* skybox) { m_skybox = skybox; }


#pragma region ShadowMapMemberFunctions
        /**
         * @brief Initializes the shadow map framebuffer object (FBO).
         * Creates and binds the FBO for shadow mapping. If a depth texture array exists, attaches it.
         * Does not validate completeness unless a depth attachment is present.
         * @return True if the FBO was successfully created, false otherwise.
         */
        bool InitializeShadowMap();
        /**
         * @brief Creates a shadow map texture array for cascaded shadow mapping.
         * Attempts to allocate a depth texture array with as many layers as possible, falling back if allocation fails.
         * Attaches the texture array to the shadow map FBO and sets up bindless texture handle.
         * @return True if the texture array was successfully created and attached, false otherwise.
         */
        bool CreateShadowMapArray();
        /**
         * @brief Calculates light-space matrices for all shadow-casting lights.
         * Computes cascade splits and shadow matrices for directional and spot lights based on the camera's view and projection.
         * Updates each light's shadow matrix and split depth for use in shadow mapping.
         * @param editorCamera Reference to the editor camera providing view and projection matrices.
         */
        void CalculateLightMatrix(const editor::EditorCamera& editorCamera);
        /**
         * @brief Renders the shadow map for all shadow-casting lights and cascades.
         * Reuses pre-skinned positions from geometry pass to avoid redundant bone calculations.
         * Restores previous OpenGL state after rendering.
         */
		void RenderShadowMapInstanced();
        /**
         * @brief Executes the full shadow pass for all shadow-casting lights.
         * Calculates light-space matrices and renders the shadow map using instanced rendering.
         */
        void RenderShadowPass();
        /**
         * @brief Computes the eight frustum corners in world space for a given cascade split.
         * Unprojects normalized device coordinates (NDC) to world space using the inverse projection-view matrix.
         * @param invPV Inverse projection-view matrix.
         * @param nearSplit NDC Z value for the near plane of the cascade.
         * @param farSplit NDC Z value for the far plane of the cascade.
         * @return Array of eight world-space frustum corners.
         */
        std::array<glm::vec3, 8> createCascadeFrustum(const glm::mat4& invPV, float nearSplit, float farSplit);
        /**
         * @brief Tests whether a spotlight's cone intersects a given frustum.
         * Checks if any frustum corner is inside the spotlight cone or if the cone intersects the frustum's AABB.
         * @param lightPos Position of the spotlight.
         * @param spotDir Direction vector of the spotlight.
         * @param outerAngleRad Outer angle of the spotlight cone in radians.
         * @param lightRadius Maximum range of the spotlight.
         * @param frustumCorners Array of eight frustum corners in world space.
         * @return True if the spotlight cone intersects the frustum, false otherwise.
        */
        bool testSpotlightFrustumIntersection(const glm::vec3& lightPos, const glm::vec3& spotDir,
            float outerAngleRad, float lightRadius,
            const std::array<glm::vec3, 8>& frustumCorners);
        /**
         * @brief Calculates the shadow matrix for a spotlight.
         * Computes a view and orthographic projection matrix that tightly fits the cascade frustum in light space.
         * Applies texel snapping and margin adjustments for stable shadows.
         * @param lightPos Position of the spotlight.
         * @param spotDir Direction vector of the spotlight.
         * @param outerAngleRad Outer angle of the spotlight cone in radians.
         * @param lightRadius Maximum range of the spotlight.
        */
        glm::mat4 calculateSpotlightShadowMatrix(const glm::vec3& lightPos,
            const glm::vec3& spotDir,
            float outerAngleRad,
            float lightRadius);
#pragma endregion

        /**
         * @brief Render transparent objects using forward rendering with depth peeling
         * @param view The view matrix
         * @param projection The projection matrix
         */
        void RenderForwardPass(const Mtx44& view, const Mtx44& projection);

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

        /**
         * @brief Check if material uses a custom shader (not standard deferred pipeline)
         * @param material Material to check
         * @return true if material has custom shader
         */
        bool HasCustomShader(const Ermine::graphics::Material* material) const;

        /**
         * @brief Handle window resize events to adjust buffers and viewports
         * @param width New window width
         * @param height New window height
		 */
		void OnWindowResize(const int& width, const int& height);

    protected:
		/**
		 * @brief Called when an entity is added to this system
		 * @param entity The entity that was added
		 */
		

    private:
        // Texture Array Management (Bindless Texture System)
        struct TextureArrayEntry
        {
            GLuint textureID = 0;
            std::string filePath;
            int arrayIndex = -1;
        };

        std::vector<GLuint> m_TextureArray;                           // All textures in the array
        std::unordered_map<std::string, int> m_TexturePathToIndex;    // Map file path to array index
        std::unordered_map<GLuint, int> m_TextureIDToIndex;           // Map texture ID to array index
        GLuint m_TextureArraySSBO = 0;                                // SSBO containing texture handles
        static constexpr GLuint TextureArrayBindingPoint = 6;         // SSBO binding point for texture array
        bool m_TextureArrayDirty = true;                              // Flag to trigger texture array rebuild

        // Renderer state
		uint8_t frameCounter = 0;

		// Light System
		std::shared_ptr<LightSystem> m_LightSystem = nullptr;
        std::shared_ptr<OffscreenBuffer> m_OffscreenBuffer;

        // Lighting UBO
        GLuint m_LightsUBO = 0;
        static constexpr GLuint LightsBindingPoint = 1;
        std::unordered_set<GLuint> m_LightBlockBoundPrograms;
        bool m_IsBlinnPhong = false; // Default to PBR shading

        // Material SSBO
        GLuint m_MaterialSSBO = 0;
        static constexpr GLuint MaterialBindingPoint = 5;  // Moved to 5 to make room for mesh SSBOs (0-3)
        std::unordered_set<GLuint> m_MaterialBlockBoundPrograms;
        std::unordered_map<EntityID, uint32_t> m_EntityMaterialIndices; // Maps entity to material index in SSBO
        
        // Material compilation system - upload all materials at load time
        std::vector<MaterialSSBO> m_CompiledMaterials; // All materials compiled into a single vector
        bool m_MaterialsDirty = true; // Flag to trigger recompilation when materials change

        /**
         * @brief Uploads all compiled materials to the GPU SSBO at once.
         * This should be called once after CompileMaterials() during load time.
         */
        void UploadMaterialsToGPU();

        // Draw data for geometry/shadow passes (opaque, non-custom shader meshes)
        std::vector<DrawElementsIndirectCommand> m_StandardDrawCommands;
        std::vector<DrawInfo> m_StandardDrawInfos;
        std::vector<DrawElementsIndirectCommand> m_SkinnedDrawCommands;
        std::vector<DrawInfo> m_SkinnedDrawInfos;

        // Draw data for forward pass (transparent + custom shader meshes)
        std::vector<DrawElementsIndirectCommand> m_ForwardPassDrawCommands;
        std::vector<DrawInfo> m_ForwardPassDrawInfos;

        // Cached shadow pass draw commands (reused to avoid per-frame allocation)
        std::vector<DrawElementsIndirectCommand> m_ShadowStandardCommands;
        std::vector<DrawElementsIndirectCommand> m_ShadowSkinnedCommands;

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
		std::shared_ptr<PostProcessBuffer> m_AntiAliasingBuffer;
        std::shared_ptr<Shader> m_BloomShader = 0; // Shader for bloom effect
        std::shared_ptr<Shader> m_PostProcessShader = 0; // Shader for post-processing effects
		std::shared_ptr<Shader> m_AAShader = 0; // Shader for anti-aliasing

        // Skybox
        Skybox* m_skybox = nullptr;

        // Shadow mapping
        std::shared_ptr<Shader> m_ShadowMapInstancedShader = nullptr;
        GLuint m_ShadowMapCube = 0;
        uint64_t m_ShadowMapArrayHandle = 0;
        GLuint m_ShadowMapFBO = 0;
        GLuint m_ShadowMapArray = 0;

        // Pre-skinned positions buffer (binding 8) - written by geometry pass, read by shadow pass
        GLuint m_PreSkinnedPositionsSSBO = 0;
        size_t m_PreSkinnedBufferSize = 0;
        unsigned int m_TotalShadowLayers = 0; // Total layers used by all shadow-casting lights

        // Forward rendering shader for transparent objects
        std::shared_ptr<Shader> m_ForwardShader = nullptr;
        std::vector<TransparentObject> m_transparentObjects;

        //Physics
        std::vector<DebugVertex> m_DebugLines;
        unsigned int m_DebugVAO = 0, m_DebugVBO = 0;
        std::shared_ptr<Shader> debugShader = nullptr;

        void BindMaterialTextures(Ermine::graphics::Material* material);

        // Picking (stencil) helpers
        struct PickingBuffer
        {
            GLuint FBO = 0;
            GLuint ColorID = 0; // GL_R32UI
            GLuint Depth = 0; // GL_DEPTH24
            int width = 0;
            int height = 0;
        };

        std::shared_ptr<PickingBuffer> m_PickingBuffer;
        std::shared_ptr<Shader> m_PickingShader = nullptr;

        /**
         * @brief Create an offscreen buffer for entity picking using stencil buffer
         * @param width The width of the picking buffer
         * @param height The height of the picking buffer
         */
        void CreatePickingBuffer(const int& width, const int& height);
        /**
         * @brief Resize the picking buffer to new dimensions without recreating the FBO
         * @param width New width
         * @param height New height
         */
        void ResizePickingBuffer(const int& width, const int& height);
        /**
         * @brief Render entities into the offscreen FBO's stencil buffer using camera VP and G-Buffer depth.
         * @param view the camera view matrix
         * @param projection the camera projection matrix
         */
        void RenderPickingPass(const Mtx44& view, const Mtx44& projection);

        /**
         * @brief Converts an Ermine::Mtx44 matrix to a glm::mat4 matrix.
         *
         * This function takes a 4x4 matrix of type Ermine::Mtx44 and converts it into
         * a glm::mat4 by directly mapping each element from row-major to the glm matrix.
         *
         * @param m The source 4x4 matrix in Ermine::Mtx44 format.
         * @return glm::mat4 A glm 4x4 matrix containing the same values as @p m.
         */
        inline glm::mat4 ToGlm(const Ermine::Mtx44& m) {
            return glm::mat4(
                m.m00, m.m01, m.m02, m.m03,
                m.m10, m.m11, m.m12, m.m13,
                m.m20, m.m21, m.m22, m.m23,
                m.m30, m.m31, m.m32, m.m33
            );
        }

#pragma region IndirectDraw
        void DrawIndirect();
#pragma endregion
    };
}