/* Start Header ************************************************************************/
/*!
\file       IndexBuffer.cpp
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This file contains the implementation of the IndexBuffer system.
            This file is used to create an index buffer for the rendering system.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "IndexBuffer.h"
#include "Logger.h"

using namespace Ermine::graphics;

IndexBuffer::IndexBuffer(const unsigned int* data, unsigned int count)
    : m_Count(count)
{
    glGenBuffers(1, &m_RendererID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count, data, GL_STATIC_DRAW);
}

IndexBuffer::~IndexBuffer()
{
    glDeleteBuffers(1, &m_RendererID);
}

void IndexBuffer::Bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
}

void IndexBuffer::Unbind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}