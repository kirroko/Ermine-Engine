/* Start Header ************************************************************************/
/*!
\file       MeshManager.h
\author     Ridhwan Afandi, mohamedridhwan.b, 2301367, mohamedridhwan.b\@digipen.edu
\date       27/09/2025
\brief      This file contains the declaration of MeshManager for multi-draw indirect rendering.

MeshManager centralizes all mesh data into contiguous GPU buffers to enable efficient
multi-draw indirect rendering. All meshes (static and skinned) are stored in single
large vertex and index buffers, with offsets tracked per mesh.

Usage Pattern (per scene):
1. Call Initialize() once at engine startup
2. Register all meshes using RegisterMesh() or RegisterSkinnedMesh() during scene load
3. Call UploadAndBuild() once after all meshes are registered
4. Access SSBOs via GetVertexSSBO(), GetIndexSSBO(), GetIndirectBuffer() for rendering
5. Call Clear() when loading a new scene, then repeat from step 2

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include "MeshTypes.h"
#include "DrawCommands.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <glad/glad.h>

namespace Ermine::graphics {

    /*!***********************************************************************
    \brief
        MeshManager handles centralized mesh storage for multi-draw indirect rendering.

        All meshes are stored in contiguous GPU buffers with per-mesh offsets.
        This enables efficient batched rendering using glMultiDrawElementsIndirect.

    Simplified Workflow:
        Scene Load:
            1. RegisterMesh/RegisterSkinnedMesh for all meshes (stages data in CPU)
            2. UploadAndBuild() once when done (uploads to GPU and builds indirect commands)
            3. Data is cleared from CPU, ready for rendering

        New Scene:
            1. Clear() to reset everything
            2. Repeat scene load workflow
    *************************************************************************/
    class MeshManager {
    public:
        MeshManager();
        ~MeshManager();

        // Mesh registration and lookup
        MeshHandle RegisterMesh(const std::vector<Vertex>& vertices,
            const std::vector<uint32_t>& indices,
            const std::string& meshID);

        MeshHandle RegisterSkinnedMesh(const std::vector<SkinnedVertex>& vertices,
            const std::vector<uint32_t>& indices,
            const std::string& meshID);

        MeshHandle GetMeshHandle(const std::string& meshID) const;
        const MeshSubset* GetMeshData(MeshHandle handle) const;

        // GPU buffer management
        void Initialize();
        void UploadAndBuild();  // Upload staged data and build indirect commands (call once per scene)
        void Clear();           // Clear all data for new scene

        // Getters for rendering (SSBOs)
        GLuint GetVertexSSBO() const { return m_VertexSSBO; }
        GLuint GetSkinnedVertexSSBO() const { return m_SkinnedVertexSSBO; }
        GLuint GetIndirectBuffer() const { return m_IndirectBuffer.bufferID; }
        const IndirectDrawBuffer& GetIndirectBufferInfo() const { return m_IndirectBuffer; }
        size_t GetMeshCount() const { return m_LoadedMeshes.size(); }

        // Check if new meshes have been registered and need uploading
        bool IsDirty() const { return m_IndirectBuffer.isDirty; }
        bool HasStagedData() const { return !m_StagedVertices.empty() || !m_StagedSkinnedVertices.empty(); }

        // Public SSBO handles for Renderer access
        GLuint m_DrawCommandsSSBO = 0;    // Binding 2 - Draw commands
        GLuint m_DrawInfoSSBO = 0;        // Binding 3 - Draw info (per-draw data) [DEPRECATED - use m_PersistentDrawInfoBuffer]
        GLuint m_IndexSSBO = 0;           // Binding 1 - All indices

        // Persistent mapped buffer for efficient DrawInfo updates
        PersistentDrawInfoBuffer m_PersistentDrawInfoBuffer;

    private:
        void CreateBuffers();

        // SSBO Binding Points
        static constexpr GLuint VERTEX_SSBO_BINDING = 0;
        static constexpr GLuint INDEX_SSBO_BINDING = 1;
        static constexpr GLuint DRAW_COMMANDS_SSBO_BINDING = 2;
        static constexpr GLuint DRAW_INFO_SSBO_BINDING = 3;
		static constexpr GLuint SKINNED_VERTEX_SSBO_BINDING = 4;

        // Mesh registry
        std::vector<MeshSubset> m_LoadedMeshes;
        std::unordered_map<std::string, MeshHandle> m_MeshCache;

        // OpenGL SSBO buffers (no VAOs needed for SSBO-based rendering)
        GLuint m_VertexSSBO = 0;          // Binding 0 - All vertices
        GLuint m_SkinnedVertexSSBO = 0;   // Separate SSBO for skinned vertices (optional)

        IndirectDrawBuffer m_IndirectBuffer;  // Draw commands buffer metadata

        // Staged data (cleared after upload)
        std::vector<Vertex> m_StagedVertices;
        std::vector<SkinnedVertex> m_StagedSkinnedVertices;
        std::vector<uint32_t> m_StagedIndices;
    };

}