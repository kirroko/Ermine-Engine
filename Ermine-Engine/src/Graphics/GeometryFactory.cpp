/* Start Header ************************************************************************/
/*!
\file       GeometryFactory.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       15/03/2025
\brief      This reflects the brief

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "GeometryFactory.h"
#include "MathUtils.h"

using namespace Ermine::graphics;

/**
 * @brief Create a cube
 * 
 * @param width The width of the cube
 * @param height The height of the cube
 * @param depth The depth of the cube
 * @return Mesh The cube mesh
 */
Ermine::Mesh GeometryFactory::CreateCube(float width, float height, float depth)
{
    float w = width * 0.5f;
    float h = height * 0.5f;
    float d = depth * 0.5f;

    std::vector<Vertex> vertices = {
        // Front face
        {{-w, -h,  d}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // 0
        {{ w, -h,  d}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, // 1
        {{ w,  h,  d}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 2
        {{-w,  h,  d}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // 3
    
        // Back face
        {{ w, -h, -d}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}}, // 4
        {{-w, -h, -d}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}}, // 5
        {{-w,  h, -d}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}}, // 6
        {{ w,  h, -d}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}}, // 7
    
        // Top face
        {{-w,  h,  d}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // 8
        {{ w,  h,  d}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 9
        {{ w,  h, -d}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}, // 10
        {{-w,  h, -d}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}, // 11
    
        // Bottom face
        {{-w, -h, -d}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}}, // 12
        {{ w, -h, -d}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}}, // 13
        {{ w, -h,  d}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}}, // 14
        {{-w, -h,  d}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}}, // 15
    
        // Right face
        {{ w, -h,  d}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 16
        {{ w, -h, -d}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // 17
        {{ w,  h, -d}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}, // 18
        {{ w,  h,  d}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // 19
    
        // Left face
        {{-w, -h, -d}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 20
        {{-w, -h,  d}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // 21
        {{-w,  h,  d}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}, // 22
        {{-w,  h, -d}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}  // 23
    };

    std::vector<unsigned int> indices = {
        // Front face
        0, 1, 2, 2, 3, 0,
    
        // Back face
        4, 5, 6, 6, 7, 4,
    
        // Top face
        8, 9, 10, 10, 11, 8,
    
        // Bottom face
        12, 13, 14, 14, 15, 12,
    
        // Right face
        16, 17, 18, 18, 19, 16,
    
        // Left face
        20, 21, 22, 22, 23, 20
    };

    auto vao = std::make_shared<VertexArray>();
    vao->SetVertexCount(vertices.size());

    auto vbo = std::make_shared<VertexBuffer>(vertices.data(), vertices.size() * sizeof(Vertex));

    vao->LinkAttribute(0, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    vao->LinkAttribute(1, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, norms));
    vao->LinkAttribute(2, 2, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, tex));
    vbo->Unbind();

    auto ibo = std::make_shared<IndexBuffer>(indices.data(), indices.size() * sizeof(unsigned int));

    return {vao, vbo, ibo};
}

/**
 * @brief Create a quad
 * 
 * @param width The width of the quad
 * @param height The height of the quad
 * @return Mesh The quad mesh
 */
Ermine::Mesh GeometryFactory::CreateQuad(float width, float height)
{
    float w = width * 0.5f;
    float h = height * 0.5f;

    std::vector<Vertex> vertices = {
        {{-w, -h, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{ w, -h, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
            {{ w,  h, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-w,  h, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,
        2, 3, 0
    };

    auto vao = std::make_shared<VertexArray>();
	vao->SetVertexCount(vertices.size());
    
    auto vbo = std::make_shared<VertexBuffer>(vertices.data(), vertices.size() * sizeof(Vertex));

    vao->LinkAttribute(0, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    vao->LinkAttribute(1, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, norms));
    vao->LinkAttribute(2, 2, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, tex));
    vbo->Unbind();

    auto ibo = std::make_shared<IndexBuffer>(indices.data(), indices.size() * sizeof(unsigned int));

    return {vao, vbo, ibo};
}

Ermine::Mesh GeometryFactory::CreateSphere(float radius, unsigned int sectors, unsigned int stacks)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    float lengthInv = 1.0f / radius;    // vertex normal

    float sectorStep = 2 * PI<float> / sectors;
    float stackStep = PI<float> / stacks;

    // Generate vertices
    for(unsigned int i = 0; i <= stacks; ++i)
    {
        float stackAngle = PI<float> / 2 - i * stackStep;      // starting from pi/2 to -pi/2
        float xy = radius * cosf(stackAngle);             // r * cos(u)
        float z = radius * sinf(stackAngle);              // r * sin(u)

        // Add (sectors+1) vertices per stack
        // The first and last vertices have same position and normal, but different tex coords
        for(unsigned int j = 0; j <= sectors; ++j)
        {
            float sectorAngle = j * sectorStep;           // starting from 0 to 2pi

            // vertex position (x, y, z)
            float x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
            float y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)

            // normalized vertex normal (nx, ny, nz)
            float nx = x * lengthInv;
            float ny = y * lengthInv;
            float nz = z * lengthInv;

            // vertex tex coord (s, t) range between [0, 1]
            float s = (float)j / sectors;
            float t = (float)i / stacks;

            // Add vertex
            vertices.push_back({{x, y, z}, {nx, ny, nz}, {s, t}});
        }
    }

    // Generate indices
    unsigned int k1, k2;
    for(unsigned int i = 0; i < stacks; ++i) {
        k1 = i * (sectors + 1);     // beginning of current stack
        k2 = k1 + sectors + 1;      // beginning of next stack

        for(unsigned int j = 0; j < sectors; ++j, ++k1, ++k2) {
            // 2 triangles per sector excluding first and last stacks
            // k1 => k2 => k1+1
            if(i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            // k1+1 => k2 => k2+1
            if(i != stacks-1) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    auto vao = std::make_shared<VertexArray>();
    vao->SetVertexCount(vertices.size());
    
    auto vbo = std::make_shared<VertexBuffer>(vertices.data(), vertices.size() * sizeof(Vertex));

    vao->LinkAttribute(0, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    vao->LinkAttribute(1, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, norms));
    vao->LinkAttribute(2, 2, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, tex));
    vbo->Unbind();

    auto ibo = std::make_shared<IndexBuffer>(indices.data(), indices.size() * sizeof(unsigned int));

    return {vao, vbo, ibo};
}
