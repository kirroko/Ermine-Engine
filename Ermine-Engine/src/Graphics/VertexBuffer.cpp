/* Start Header ************************************************************************/
/*!
\file       VertexBuffer.cpp
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This file contains the implementation of the VertexBuffer system.
            This file is used to create a vertex buffer object.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "VertexBuffer.h"

#include "Logger.h"

using namespace Ermine::graphics;

VertexBuffer::VertexBuffer(const void* data, unsigned int size)
{
    glGenBuffers(1, &m_RendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW); // usage is static draw because the data will not change, unless we are using dynamic draw
}

VertexBuffer::~VertexBuffer()
{
    glDeleteBuffers(1, &m_RendererID);
}

void VertexBuffer::Bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
}

void VertexBuffer::Unbind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}