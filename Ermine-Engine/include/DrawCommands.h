/* Start Header ************************************************************************/
/*!
\file       DrawCommands.h
\author     Generated with Claude Code
\date       25/10/2025
\brief      This file contains the declaration of draw command and draw info structs
            for the Ermine graphics system. These structures are used for indirect
            rendering and batch drawing operations.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace Ermine::graphics
{
    /**
     * @brief DrawElementsIndirectCommand structure for glDrawElementsIndirect
     * Matches the OpenGL spec for indirect draw commands
     */
    struct DrawElementsIndirectCommand
    {
        uint32_t count;         // Number of elements (indices) to draw
        uint32_t instanceCount; // Number of instances to draw
        uint32_t firstIndex;    // Starting index in the index buffer
        uint32_t baseVertex;    // Value added to each index before indexing into the vertex buffer
        uint32_t baseInstance;  // Starting instance ID
    };

    /**
     * @brief DrawArraysIndirectCommand structure for glDrawArraysIndirect
     * Matches the OpenGL spec for indirect array draw commands
     */
    struct DrawArraysIndirectCommand
    {
        uint32_t count;         // Number of vertices to draw
        uint32_t instanceCount; // Number of instances to draw
        uint32_t first;         // Starting vertex index
        uint32_t baseInstance;  // Starting instance ID
    };

    /**
     * @brief Draw info structure containing metadata for a draw call
     * Used to pass additional information alongside draw commands
     * Note: This struct must be std140/std430 compatible for SSBO usage
     */
    struct DrawInfo
    {
        glm::mat4 modelMatrix;  // 64 bytes - Model transformation matrix
        glm::vec3 aabbMin;      // 12 bytes - AABB minimum bounds
        uint32_t materialIndex; // 4 bytes  - Index into the material SSBO
        glm::vec3 aabbMax;      // 12 bytes - AABB maximum bounds
        uint32_t entityID;      // 4 bytes  - Entity ID for identification
        uint32_t flags;         // 4 bytes  - Flags (bit 0: useSkinning, bits 1-31: reserved)
        uint32_t _pad[3];       // 12 bytes - Padding to maintain 16-byte alignment
        // Total: 112 bytes (aligned to 16 bytes)
    };

    /**
     * @brief Batch draw info containing all data needed for a batched draw call
     * Combines draw command with associated metadata
     */
    struct BatchDrawInfo
    {
        DrawElementsIndirectCommand command;
        DrawInfo info;

        BatchDrawInfo() = default;
        BatchDrawInfo(uint32_t count, uint32_t instanceCount, uint32_t firstIndex,
                     uint32_t baseVertex, uint32_t baseInstance,
                     const glm::mat4& modelMatrix, const glm::vec3& aabbMin, const glm::vec3& aabbMax,
                     uint32_t materialIndex, uint32_t entityID)
            : command{count, instanceCount, firstIndex, baseVertex, baseInstance}
            , info{modelMatrix, aabbMin, materialIndex, aabbMax, entityID}
        {}
    };

    /**
     * @brief Multi-draw indirect structure for efficient batched rendering
     * Contains an array of draw commands that can be executed in a single draw call
     */
    struct MultiDrawIndirect
    {
        std::vector<DrawElementsIndirectCommand> commands;
        std::vector<DrawInfo> drawInfos;
        uint32_t drawCount = 0; // Number of valid draw commands

        void Clear()
        {
            commands.clear();
            drawInfos.clear();
            drawCount = 0;
        }

        void Reserve(size_t count)
        {
            commands.reserve(count);
            drawInfos.reserve(count);
        }

        void AddDraw(const DrawElementsIndirectCommand& cmd, const DrawInfo& info)
        {
            commands.push_back(cmd);
            drawInfos.push_back(info);
            ++drawCount;
        }
    };

    /**
     * @brief GPU buffer info for managing indirect draw buffers
     */
    struct IndirectDrawBuffer
    {
        uint32_t bufferID = 0;           // OpenGL buffer object ID
        uint32_t commandCount = 0;       // Number of commands in buffer
        size_t bufferSize = 0;           // Size of buffer in bytes
        bool isDirty = false;            // Flag to track if buffer needs updating (starts clean)

        void MarkDirty() { isDirty = true; }
        void MarkClean() { isDirty = false; }
        bool IsValid() const { return bufferID != 0; }
    };

} // namespace Ermine::graphics
