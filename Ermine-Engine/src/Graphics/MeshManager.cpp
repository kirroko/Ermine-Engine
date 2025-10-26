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

namespace Ermine::graphics {

    MeshManager::MeshManager()
    {
        // Default initialization - member variables are already initialized to 0
        // due to their default member initializers in the header
    }

    MeshManager::~MeshManager()
    {
        // Cleanup OpenGL SSBO resources
        if (m_VertexSSBO != 0) {
            glDeleteBuffers(1, &m_VertexSSBO);
            m_VertexSSBO = 0;
        }
        if (m_SkinnedVertexSSBO != 0) {
            glDeleteBuffers(1, &m_SkinnedVertexSSBO);
            m_SkinnedVertexSSBO = 0;
        }
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
    }

    void MeshManager::Initialize()
    {
        // Create GPU buffers (SSBOs)
        CreateBuffers();

        /*

        // Setup standard vertex VAO
        glBindVertexArray(m_VertexVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VertexEBO);

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

        // Setup skinned vertex VAO
        glBindVertexArray(m_SkinnedVertexVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_SkinnedVertexVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SkinnedVertexEBO);

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

        glBindVertexArray(0);
        */

        EE_CORE_INFO("MeshManager: Initialized");
    }

    void MeshManager::CreateBuffers()
    {
        // Create Vertex SSBO (Binding 0)
        glGenBuffers(1, &m_VertexSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_VertexSSBO);
        // Allocate empty buffer - will be filled in UploadAndBuild()
        glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, VERTEX_SSBO_BINDING, m_VertexSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        EE_CORE_INFO("MeshManager: Created Vertex SSBO at binding {}", VERTEX_SSBO_BINDING);

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

        // Create Skinned Vertex SSBO (Binding 4) - Separate buffer for skinned vertices
        glGenBuffers(1, &m_SkinnedVertexSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SkinnedVertexSSBO);
        // Allocate empty buffer - will be filled in UploadAndBuild()
        glBufferData(GL_SHADER_STORAGE_BUFFER, 0, nullptr, GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SKINNED_VERTEX_SSBO_BINDING, m_SkinnedVertexSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        EE_CORE_INFO("MeshManager: Created Skinned Vertex SSBO at binding {}", SKINNED_VERTEX_SSBO_BINDING);
    }

    MeshHandle MeshManager::RegisterMesh(const std::vector<Vertex>& vertices,
                                         const std::vector<uint32_t>& indices,
                                         const std::string& meshID)
    {
        // Check if mesh already exists
        auto it = m_MeshCache.find(meshID);
        if (it != m_MeshCache.end()) {
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

		EE_CORE_INFO("MeshManager: Registered mesh '{}'", meshID);

        return handle;
    }

    MeshHandle MeshManager::RegisterSkinnedMesh(const std::vector<SkinnedVertex>& vertices,
                                                const std::vector<uint32_t>& indices,
                                                const std::string& meshID)
    {
        // Check if mesh already exists
        auto it = m_MeshCache.find(meshID);
        if (it != m_MeshCache.end()) {
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

        EE_CORE_INFO("MeshManager: Registered mesh '{}'", meshID);

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
        // Upload Vertex SSBO data
        if (!m_StagedVertices.empty()) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_VertexSSBO);
            size_t vertexBufferSize = m_StagedVertices.size() * sizeof(Vertex);
            glBufferData(GL_SHADER_STORAGE_BUFFER,
                        vertexBufferSize,
                        m_StagedVertices.data(),
                        GL_STATIC_DRAW);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            EE_CORE_INFO("MeshManager: Uploaded {} vertices ({} bytes) to Vertex SSBO",
                         m_StagedVertices.size(), vertexBufferSize);
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
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_SkinnedVertexSSBO);
            size_t skinnedVertexBufferSize = m_StagedSkinnedVertices.size() * sizeof(SkinnedVertex);
            glBufferData(GL_SHADER_STORAGE_BUFFER,
                        skinnedVertexBufferSize,
                        m_StagedSkinnedVertices.data(),
                        GL_STATIC_DRAW);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            EE_CORE_INFO("MeshManager: Uploaded {} skinned vertices ({} bytes) to Skinned Vertex SSBO",
                         m_StagedSkinnedVertices.size(), skinnedVertexBufferSize);
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

} // namespace Ermine::graphics