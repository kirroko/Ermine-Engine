/* Start Header ************************************************************************/
/*!
\file       DrawCommands.cpp
\author     Ridhwan Afandi, mohamedridhwan.b, 2301367, mohamedridhwan.b\@digipen.edu
\date       27/10/2025
\brief      Implementation of persistent mapped buffer for DrawInfo data

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "DrawCommands.h"
#include "Logger.h"
#include <glad/glad.h>
#include <cstring>

namespace Ermine::graphics
{
    PersistentDrawInfoBuffer::~PersistentDrawInfoBuffer()
    {
        if (m_MappedPtr)
        {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_BufferID);
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            m_MappedPtr = nullptr;
        }

        if (m_BufferID != 0)
        {
            glDeleteBuffers(1, &m_BufferID);
            m_BufferID = 0;
        }
    }

    bool PersistentDrawInfoBuffer::Initialize(size_t maxDrawCalls)
    {
        if (m_BufferID != 0)
        {
            EE_CORE_WARN("PersistentDrawInfoBuffer already initialized");
            return false;
        }

        m_MaxDrawCalls = maxDrawCalls;
        m_BufferSize = maxDrawCalls * sizeof(DrawInfo);

        // Create buffer
        glGenBuffers(1, &m_BufferID);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_BufferID);

        // Allocate storage with persistent mapping flags
        // GL_MAP_WRITE_BIT: Allow CPU writes
        // GL_MAP_PERSISTENT_BIT: Keep mapping alive
        // GL_MAP_COHERENT_BIT: Automatic synchronization (no manual flushing needed)
        GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, m_BufferSize, nullptr, flags);

        // Check for errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
            EE_CORE_ERROR("Failed to create persistent mapped buffer for DrawInfo, error: {0}", error);
            glDeleteBuffers(1, &m_BufferID);
            m_BufferID = 0;
            return false;
        }

        // Map the buffer persistently
        m_MappedPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, m_BufferSize, flags);

        if (!m_MappedPtr)
        {
            EE_CORE_ERROR("Failed to map DrawInfo buffer persistently");
            glDeleteBuffers(1, &m_BufferID);
            m_BufferID = 0;
            return false;
        }

        // Bind to SSBO binding point 3 (DRAW_INFO_SSBO_BINDING)
        constexpr GLuint DRAW_INFO_SSBO_BINDING = 3;
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DRAW_INFO_SSBO_BINDING, m_BufferID);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        EE_CORE_INFO("Initialized persistent DrawInfo buffer: {0} max draws, {1} bytes, binding {2}",
                     maxDrawCalls, m_BufferSize, DRAW_INFO_SSBO_BINDING);

        return true;
    }

    void PersistentDrawInfoBuffer::WriteDrawInfos(const std::vector<DrawInfo>& drawInfos, size_t offset)
    {
        if (!IsValid())
        {
            EE_CORE_ERROR("PersistentDrawInfoBuffer not initialized");
            return;
        }

        if (offset + drawInfos.size() > m_MaxDrawCalls)
        {
            EE_CORE_WARN("DrawInfo offset + count ({0}) exceeds max capacity ({1}), clamping",
                         offset + drawInfos.size(), m_MaxDrawCalls);
        }

        // Calculate how many draws to actually write
        size_t drawsToWrite = std::min(drawInfos.size(), m_MaxDrawCalls - offset);
        m_DrawCount = offset + drawsToWrite;

        // Direct memory copy to persistent mapped buffer at offset
        // No glBufferData/glBufferSubData needed - just memcpy!
        size_t bytesToWrite = drawsToWrite * sizeof(DrawInfo);
        void* destPtr = static_cast<char*>(m_MappedPtr) + (offset * sizeof(DrawInfo));
        std::memcpy(destPtr, drawInfos.data(), bytesToWrite);

        // With GL_MAP_COHERENT_BIT, no manual flush is needed
        // GPU will see the changes automatically
    }

} // namespace Ermine::graphics
