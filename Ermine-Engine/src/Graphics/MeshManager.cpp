/* Start Header ************************************************************************/
/*!
\file       MeshManager.cpp
\author     Ridhwan Afandi, mohamedridhwan.b, 2301367, mohamedridhwan.b\@digipen.edu
\date       27/09/2025
\brief      Implementation of MeshManager for multi-draw indirect rendering.

Example Usage:
    // At engine startup
    meshManager.Initialize();

    // During scene load
    meshManager.RegisterMesh(cubeVerts, cubeIndices, "cube");
    meshManager.RegisterMesh(sphereVerts, sphereIndices, "sphere");
    meshManager.UploadAndBuild();

    // During rendering
    glBindVertexArray(meshManager.GetVertexVAO());
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, meshManager.GetIndirectBuffer());
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                meshManager.GetMeshCount(), 0);

    // When loading new scene
    meshManager.Clear();  // Start fresh for new scene

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "MeshManager.h"
#include "SSBO_Bindings.h"

namespace Ermine::graphics {

    MeshManager::MeshManager()
    {
        // Default initialization - member variables are already initialized to 0
        // due to their default member initializers in the header
    }

    MeshManager::~MeshManager()
    {
        if (m_IndexSSBO != 0) {
            glDeleteBuffers(1, &m_IndexSSBO);
            m_IndexSSBO = 0;
        }
        if (m_DrawCommandsSSBO != 0) {
            glDeleteBuffers(1, &m_DrawCommandsSSBO);
            m_DrawCommandsSSBO = 0;
        }
        if (m_DrawInfoSSBO != 0) {
            glDeleteBuffers(1, &m_DrawInfoSSBO);
            m_DrawInfoSSBO = 0;
        }
        if (m_IndirectBuffer.bufferID != 0) {
            glDeleteBuffers(1, &m_IndirectBuffer.bufferID);
            m_IndirectBuffer.bufferID = 0;
        }

        // Cleanup VBO and VAO resources
        if (m_VertexVBO != 0) {
            glDeleteBuffers(1, &m_VertexVBO);
            m_VertexVBO = 0;
        }
        if (m_StandardVAO != 0) {
            glDeleteVertexArrays(1, &m_StandardVAO);
            m_StandardVAO = 0;
        }
        if (m_SkinnedVBO != 0) {
            glDeleteBuffers(1, &m_SkinnedVBO);
            m_SkinnedVBO = 0;
        }
        if (m_SkinnedVAO != 0) {
            glDeleteVertexArrays(1, &m_SkinnedVAO);
            m_SkinnedVAO = 0;
        }
    }

    void MeshManager::Initialize()
    {
        // Create GPU buffers (SSBOs)
        CreateBuffers();

        // Setup VAOs with attribute bindings
        SetupStandardVAO();
        SetupSkinnedVAO();

        // Initialize persistent mapped buffer for DrawInfo (max 10000 draws)
        constexpr size_t MAX_DRAW_CALLS = 100000;
        if (!m_PersistentDrawInfoBuffer.Initialize(MAX_DRAW_CALLS))
        {
            EE_CORE_ERROR("Failed to initialize persistent DrawInfo buffer");
        }

        // Initialize skeletal SSBO (max 100 skeletons = 100 * 128 bones = 12800 bones)
        constexpr size_t MAX_SKELETONS = 10;
        if (!m_SkeletalSSBO.Initialize(MAX_SKELETONS))
        {
            EE_CORE_ERROR("Failed to initialize skeletal SSBO");
        }

        EE_CORE_INFO("MeshManager: Initialized");
    }

    void MeshManager::CreateBuffers()
    {
        // Create Vertex VBO for standard vertices (64 bytes each)
        glGenBuffers(1, &m_VertexVBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexVBO);
        // Allocate empty buffer - will be filled in UploadAndBuild()
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        EE_CORE_INFO("MeshManager: Created Vertex VBO (standard vertices)");

        // Create Skinned VBO for skinned vertices (96 bytes each)
        glGenBuffers(1, &m_SkinnedVBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_SkinnedVBO);
        // Allocate empty buffer - will be filled in UploadAndBuild()
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        EE_CORE_INFO("MeshManager: Created Skinned VBO (skinned vertices)");

        // Create Index SSBO (Binding 1)
        glGenBuffers(1, &m_IndexSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_IndexSSBO);
        // Allocate empty buffer - will be filled in UploadAndBuild()
        glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_SSBO_BINDING, m_IndexSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        EE_CORE_INFO("MeshManager: Created Index SSBO at binding {}", INDEX_SSBO_BINDING);

        // Create Draw Commands SSBO (Binding 2)
        glGenBuffers(1, &m_DrawCommandsSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_DrawCommandsSSBO);
        // Allocate empty buffer - will be filled in BuildIndirectCommands()
        glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DRAW_COMMANDS_SSBO_BINDING, m_DrawCommandsSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        EE_CORE_INFO("MeshManager: Created Draw Commands SSBO at binding {}", DRAW_COMMANDS_SSBO_BINDING);

        // Create Draw Info SSBO (Binding 3)
        glGenBuffers(1, &m_DrawInfoSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_DrawInfoSSBO);
        // Allocate empty buffer - will be filled in BuildIndirectCommands()
        glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DRAW_INFO_SSBO_BINDING, m_DrawInfoSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        EE_CORE_INFO("MeshManager: Created Draw Info SSBO at binding {}", DRAW_INFO_SSBO_BINDING);
    }

    void MeshManager::SetupStandardVAO()
    {
        // Create Standard VAO
        glGenVertexArrays(1, &m_StandardVAO);
        glBindVertexArray(m_StandardVAO);

        // Bind vertex buffer (VBO)
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexVBO);

        // Bind index buffer (still using SSBO for indices)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexSSBO);

        // Position attribute (location 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             (void*)offsetof(Vertex, position));

        // Normal attribute (location 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             (void*)offsetof(Vertex, normal));

        // TexCoord attribute (location 2)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             (void*)offsetof(Vertex, texCoord));

        // Tangent attribute (location 3)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             (void*)offsetof(Vertex, tangent));

        EE_CORE_INFO("MeshManager: Configured Standard VAO (locations 0-3) - NOT USED YET");

        // CRITICAL: Unbind array buffer to avoid VAO 0 corruption, but DON'T unbind element buffer!
        // Element buffer binding is VAO state - unbinding it here would remove it from the VAO!
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Unbind VAO (element buffer remains bound to the VAO)
        glBindVertexArray(0);
    }

    void MeshManager::SetupSkinnedVAO()
    {
        // Create Skinned VAO
        glGenVertexArrays(1, &m_SkinnedVAO);
        glBindVertexArray(m_SkinnedVAO);

        // Bind skinned vertex buffer (VBO)
        glBindBuffer(GL_ARRAY_BUFFER, m_SkinnedVBO);

        // Bind index buffer (still using SSBO for indices)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexSSBO);

        // Position attribute (location 0) - offset 0
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                             (void*)offsetof(SkinnedVertex, position));

        // Normal attribute (location 1) - offset 16
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                             (void*)offsetof(SkinnedVertex, normal));

        // TexCoord attribute (location 2) - offset 32
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                             (void*)offsetof(SkinnedVertex, texCoord));

        // Tangent attribute (location 3) - offset 48
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                             (void*)offsetof(SkinnedVertex, tangent));

        // BoneIDs attribute (location 4) - offset 64 (ivec4)
        glEnableVertexAttribArray(4);
        glVertexAttribIPointer(4, 4, GL_INT, sizeof(SkinnedVertex),
                              (void*)offsetof(SkinnedVertex, boneIDs));

        // BoneWeights attribute (location 5) - offset 80 (vec4)
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                             (void*)offsetof(SkinnedVertex, boneWeights));

        EE_CORE_INFO("MeshManager: Configured Skinned VAO (locations 0-5)");

        // CRITICAL: Unbind array buffer to avoid VAO 0 corruption, but DON'T unbind element buffer!
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Unbind VAO (element buffer remains bound to the VAO)
        glBindVertexArray(0);
    }

    MeshHandle MeshManager::RegisterMesh(const std::vector<Vertex>& vertices,
                                         const std::vector<uint32_t>& indices,
                                         const std::string& meshID)
    {
        // Check if mesh already exists
        auto it = m_MeshCache.find(meshID);
        if (it != m_MeshCache.end()) {
            EE_CORE_WARN("MeshManager: Mesh '{}' already exists in cache (handle index: {}), returning cached handle",
                         meshID, it->second.index);
            return it->second;
        }

        // Create new mesh subset
        MeshSubset subset;
        subset.meshID = meshID;
        subset.vertexOffset = static_cast<uint32_t>(m_StagedVertices.size());
        subset.indexOffset = static_cast<uint32_t>(m_StagedIndices.size());
        subset.indexCount = static_cast<uint32_t>(indices.size());
        subset.baseVertex = subset.vertexOffset;

        // Add to staged data
        m_StagedVertices.insert(m_StagedVertices.end(), vertices.begin(), vertices.end());
        m_StagedIndices.insert(m_StagedIndices.end(), indices.begin(), indices.end());

        // Register mesh
        MeshHandle handle;
        handle.index = static_cast<uint32_t>(m_LoadedMeshes.size());
        m_LoadedMeshes.push_back(subset);
        m_MeshCache[meshID] = handle;

        // Mark indirect buffer as dirty so it will be uploaded
        m_IndirectBuffer.MarkDirty();

		EE_CORE_INFO("MeshManager: Registered NEW mesh '{}' (handle index: {}, vertexOffset: {}, vertices: {}, indexOffset: {}, indices: {})",
                     meshID, handle.index, subset.vertexOffset, vertices.size(), subset.indexOffset, indices.size());

        return handle;
    }

    MeshHandle MeshManager::RegisterSkinnedMesh(const std::vector<SkinnedVertex>& vertices,
                                                const std::vector<uint32_t>& indices,
                                                const std::string& meshID)
    {
        // Check if mesh already exists
        auto it = m_MeshCache.find(meshID);
        if (it != m_MeshCache.end()) {
            EE_CORE_WARN("MeshManager: Skinned mesh '{}' already exists in cache (handle index: {}), returning cached handle",
                         meshID, it->second.index);
            return it->second;
        }

        // Create new mesh subset
        MeshSubset subset;
        subset.meshID = meshID;
        subset.vertexOffset = static_cast<uint32_t>(m_StagedSkinnedVertices.size());
        subset.indexOffset = static_cast<uint32_t>(m_StagedIndices.size());
        subset.indexCount = static_cast<uint32_t>(indices.size());
        subset.baseVertex = subset.vertexOffset;

        // Add to staged data
        m_StagedSkinnedVertices.insert(m_StagedSkinnedVertices.end(), vertices.begin(), vertices.end());
        m_StagedIndices.insert(m_StagedIndices.end(), indices.begin(), indices.end());

        // Register mesh
        MeshHandle handle;
        handle.index = static_cast<uint32_t>(m_LoadedMeshes.size());
        m_LoadedMeshes.push_back(subset);
        m_MeshCache[meshID] = handle;

        // Mark indirect buffer as dirty so it will be uploaded
        m_IndirectBuffer.MarkDirty();

        EE_CORE_INFO("MeshManager: Registered NEW skinned mesh '{}' (handle index: {}, vertexOffset: {}, vertices: {}, indexOffset: {}, indices: {})",
                     meshID, handle.index, subset.vertexOffset, vertices.size(), subset.indexOffset, indices.size());

        return handle;
    }

    MeshHandle MeshManager::GetMeshHandle(const std::string& meshID) const
    {
        auto it = m_MeshCache.find(meshID);
        if (it != m_MeshCache.end()) {
            return it->second;
        }
        return MeshHandle(); // Returns invalid handle
    }

    const MeshSubset* MeshManager::GetMeshData(MeshHandle handle) const
    {
        if (!handle.isValid() || handle.index >= m_LoadedMeshes.size()) {
            return nullptr;
        }
        return &m_LoadedMeshes[handle.index];
    }

    void MeshManager::UploadAndBuild()
    {
        EE_CORE_INFO("MeshManager::UploadAndBuild() called - Staged: {} regular vertices, {} skinned vertices, {} indices",
                     m_StagedVertices.size(), m_StagedSkinnedVertices.size(), m_StagedIndices.size());

        // Upload Vertex SSBO data
        if (!m_StagedVertices.empty()) {
            size_t vertexBufferSize = m_StagedVertices.size() * sizeof(Vertex);
            glBindBuffer(GL_ARRAY_BUFFER, m_VertexVBO);
            glBufferData(GL_ARRAY_BUFFER,
                        vertexBufferSize,
                        m_StagedVertices.data(),
                        GL_STATIC_DRAW);

            // DIAGNOSTIC: Verify VBO has data
            GLint vboSize = 0;
            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &vboSize);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            EE_CORE_INFO("MeshManager: Uploaded {} vertices ({} bytes) to Vertex VBO (ID: {})",
                         m_StagedVertices.size(), vertexBufferSize, m_VertexVBO);
            EE_CORE_INFO("MeshManager: VBO size verified: {} bytes", vboSize);
        }

        // Upload Index SSBO data
        if (!m_StagedIndices.empty()) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_IndexSSBO);
            size_t indexBufferSize = m_StagedIndices.size() * sizeof(uint32_t);
            glBufferData(GL_SHADER_STORAGE_BUFFER,
                        indexBufferSize,
                        m_StagedIndices.data(),
                        GL_STATIC_DRAW);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            EE_CORE_INFO("MeshManager: Uploaded {} indices ({} bytes) to Index SSBO",
                         m_StagedIndices.size(), indexBufferSize);
        }

        // Upload Skinned Vertex SSBO data
        if (!m_StagedSkinnedVertices.empty()) {
            size_t skinnedVertexBufferSize = m_StagedSkinnedVertices.size() * sizeof(SkinnedVertex);

            // Also upload to Skinned VBO
            glBindBuffer(GL_ARRAY_BUFFER, m_SkinnedVBO);
            glBufferData(GL_ARRAY_BUFFER,
                        skinnedVertexBufferSize,
                        m_StagedSkinnedVertices.data(),
                        GL_STATIC_DRAW);

            // Verify SkinnedVBO has data
            GLint skinnedVboSize = 0;
            glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &skinnedVboSize);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            EE_CORE_INFO("MeshManager: Uploaded {} skinned vertices ({} bytes) to Skinned VBO (ID: {})",
                         m_StagedSkinnedVertices.size(), skinnedVertexBufferSize, m_SkinnedVBO);
            EE_CORE_INFO("MeshManager: Skinned VBO size verified: {} bytes", skinnedVboSize);
        }

		// Clear staged data to free CPU memory (need to reregister meshes for new scene)
        //m_StagedVertices.clear();
        //m_StagedVertices.shrink_to_fit();
        //m_StagedSkinnedVertices.clear();
        //m_StagedSkinnedVertices.shrink_to_fit();
        //m_StagedIndices.clear();
        //m_StagedIndices.shrink_to_fit();

        // Clear dirty flag
        m_IndirectBuffer.MarkClean();


        EE_CORE_INFO("MeshManager: Upload complete. {} meshes, {} vertices ({} regular + {} skinned), {} indices",
            m_LoadedMeshes.size(),
            m_StagedVertices.size() + m_StagedSkinnedVertices.size(),
            m_StagedVertices.size(),
            m_StagedSkinnedVertices.size(),
            m_StagedIndices.size());
    }

    void MeshManager::Clear()
    {
        // Clear all CPU-side data
        m_LoadedMeshes.clear();
        m_MeshCache.clear();
        m_StagedVertices.clear();
        m_StagedSkinnedVertices.clear();
        m_StagedIndices.clear();

        // Free memory
        m_LoadedMeshes.shrink_to_fit();
        m_StagedVertices.shrink_to_fit();
        m_StagedSkinnedVertices.shrink_to_fit();
        m_StagedIndices.shrink_to_fit();

        // Reset indirect buffer metadata
        m_IndirectBuffer.commandCount = 0;
        m_IndirectBuffer.bufferSize = 0;
        m_IndirectBuffer.MarkClean();
    }

    void MeshManager::SetupShadowVAOs(GLuint preSkinnedBuffer)
    {
        // ==================== STANDARD SHADOW VAO ====================
        // Create shadow VAO for standard (non-skinned) meshes
        glGenVertexArrays(1, &m_StandardShadowVAO);
        glBindVertexArray(m_StandardShadowVAO);

        // Bind standard vertex buffer for attributes 0-3
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexVBO);

        // Bind index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexSSBO);

        // Position attribute (location 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             (void*)offsetof(Vertex, position));

        // Normal attribute (location 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             (void*)offsetof(Vertex, normal));

        // TexCoord attribute (location 2)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             (void*)offsetof(Vertex, texCoord));

        // Tangent attribute (location 3)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             (void*)offsetof(Vertex, tangent));

        // Bind pre-skinned buffer for attribute 6 (vec4 - xyz = position, w = unused)
        glBindBuffer(GL_ARRAY_BUFFER, preSkinnedBuffer);
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);

        EE_CORE_INFO("MeshManager: Configured Standard Shadow VAO (locations 0-3, 6)");

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // ==================== SKINNED SHADOW VAO ====================
        // Create shadow VAO for skinned meshes
        glGenVertexArrays(1, &m_SkinnedShadowVAO);
        glBindVertexArray(m_SkinnedShadowVAO);

        // Bind skinned vertex buffer for attributes 0-5
        glBindBuffer(GL_ARRAY_BUFFER, m_SkinnedVBO);

        // Bind index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexSSBO);

        // Position attribute (location 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                             (void*)offsetof(SkinnedVertex, position));

        // Normal attribute (location 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                             (void*)offsetof(SkinnedVertex, normal));

        // TexCoord attribute (location 2)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                             (void*)offsetof(SkinnedVertex, texCoord));

        // Tangent attribute (location 3)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                             (void*)offsetof(SkinnedVertex, tangent));

        // BoneIDs attribute (location 4)
        glEnableVertexAttribArray(4);
        glVertexAttribIPointer(4, 4, GL_INT, sizeof(SkinnedVertex),
                              (void*)offsetof(SkinnedVertex, boneIDs));

        // BoneWeights attribute (location 5)
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                             (void*)offsetof(SkinnedVertex, boneWeights));

        // Bind pre-skinned buffer for attribute 6 (vec4 - xyz = position, w = unused)
        glBindBuffer(GL_ARRAY_BUFFER, preSkinnedBuffer);
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);

        EE_CORE_INFO("MeshManager: Configured Skinned Shadow VAO (locations 0-6)");

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

} // namespace Ermine::graphics