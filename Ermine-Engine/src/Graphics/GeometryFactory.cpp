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

	auto mesh = Mesh(vao, vbo, ibo);
	mesh.kind = MeshKind::Primitive;
	mesh.primitive.type = "Cube";
	mesh.primitive.size = Vec3{ width, height, depth };

    return mesh;
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
	auto mesh = Mesh(vao, vbo, ibo);
	mesh.kind = MeshKind::Primitive;
	mesh.primitive.type = "Quad";
	mesh.primitive.size = Vec3{ width, height, 0.0f };

    return mesh;
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

	auto mesh = Mesh(vao, vbo, ibo);
	mesh.kind = MeshKind::Primitive;
	mesh.primitive.type = "Sphere";
	mesh.primitive.size = Vec3{ radius, radius, radius };

    return mesh;
}

/**
 * @brief Calculate AABB for a mesh
 * 
 * @param mesh The mesh to calculate AABB for
 * @return AABB The calculated axis-aligned bounding box
 */
GeometryFactory::AABB GeometryFactory::CalculateAABB(const Mesh& mesh)
{
    AABB aabb;
    
    // If it's a primitive type, use the optimized calculation
    if (mesh.kind == MeshKind::Primitive) {
        return CalculatePrimitiveAABB(mesh.primitive.type, mesh.primitive.size);
    }
    
    // For non-primitive meshes (assets/models), we need actual vertex data
    aabb.min = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    aabb.max = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    
    if (!mesh.vertex_buffer || !mesh.vertex_array) {
        // Return a default AABB if mesh is invalid
        EE_CORE_WARN("Invalid mesh - using default AABB");
        aabb.min = Vec3(-0.5f, -0.5f, -0.5f);
        aabb.max = Vec3(0.5f, 0.5f, 0.5f);
        return aabb;
    }
    
    // Get vertex count from vertex array
    size_t vertexCount = mesh.vertex_array->GetVertexCount();
    
    if (vertexCount == 0) {
        // No vertices, return default bounds
        aabb.min = Vec3(-0.5f, -0.5f, -0.5f);
        aabb.max = Vec3(0.5f, 0.5f, 0.5f);
        return aabb;
    }
    
    // For model meshes, try to read the actual vertex data
    // This is a simplified approach - ideally we'd cache AABBs during model loading
    // For now, use a heuristic based on the mesh's vertex count and type
    
    if (mesh.kind == MeshKind::Asset && !mesh.asset.meshName.empty()) {
        // Try to load model and calculate bounds from actual vertex data
        auto& assetManager = AssetManager::GetInstance();
        std::string modelPath = "../Resources/Models/" + mesh.asset.meshName;
        std::shared_ptr<Model> model = assetManager.GetModel(modelPath);
        
        if (model) {
            // Iterate through all meshes in the model to find bounds
            const auto& modelMeshes = model->GetMeshes();
            
            bool foundBounds = false;
            for (const auto& modelMesh : modelMeshes) {
                if (!modelMesh.vao || !modelMesh.vbo) continue;
                
                // Bind vertex buffer to read data
                modelMesh.vbo->Bind();
                
                // Get buffer size and calculate vertex count
                GLint bufferSize = 0;
                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
                
                // Assuming standard vertex format: position(3) + normal(3) + texcoord(2) = 8 floats
                const size_t vertexStride = sizeof(Vertex); // 32 bytes
                size_t vCount = bufferSize / vertexStride;
                
                if (vCount > 0) {
                    // Map buffer to read vertex positions
                    float* vertexData = static_cast<float*>(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));
                    
                    if (vertexData) {
                        // Iterate through vertices (position is first 3 floats)
                        for (size_t i = 0; i < vCount; ++i) {
                            size_t offset = i * 8; // 8 floats per vertex
                            Vec3 pos(vertexData[offset], vertexData[offset + 1], vertexData[offset + 2]);
                            
                            // Update bounding box
                            aabb.min.x = std::min(aabb.min.x, pos.x);
                            aabb.min.y = std::min(aabb.min.y, pos.y);
                            aabb.min.z = std::min(aabb.min.z, pos.z);
                            
                            aabb.max.x = std::max(aabb.max.x, pos.x);
                            aabb.max.y = std::max(aabb.max.y, pos.y);
                            aabb.max.z = std::max(aabb.max.z, pos.z);
                            
                            foundBounds = true;
                        }
                        
                        glUnmapBuffer(GL_ARRAY_BUFFER);
                    }
                }
                
                modelMesh.vbo->Unbind();
                
                // If we found bounds from any mesh, we're done
                if (foundBounds) break;
            }
            
            // If we successfully calculated bounds, return them
            if (foundBounds) {
                EE_CORE_INFO("Calculated AABB from model vertex data: min=({:.2f}, {:.2f}, {:.2f}), max=({:.2f}, {:.2f}, {:.2f})",
                           aabb.min.x, aabb.min.y, aabb.min.z, aabb.max.x, aabb.max.y, aabb.max.z);
                return aabb;
            }
        }
        
        // Fallback: use vertex count to estimate size (heuristic)
        float approxSize = std::cbrt(static_cast<float>(vertexCount)) * 0.1f;
        aabb.min = Vec3(-approxSize, -approxSize, -approxSize);
        aabb.max = Vec3(approxSize, approxSize, approxSize);
        
        EE_CORE_WARN("Using heuristic AABB for model: size={:.2f}", approxSize);
        return aabb;
    }
    
    // Generic fallback for unknown mesh types
    aabb.min = Vec3(-1.0f, -1.0f, -1.0f);
    aabb.max = Vec3(1.0f, 1.0f, 1.0f);
    return aabb;
}

/**
 * @brief Calculate AABB for a primitive type
 * 
 * @param type The type of primitive ("Cube", "Quad", "Sphere", etc.")
 * @param size The size/dimensions of the primitive
 * @return AABB The calculated axis-aligned bounding box
 */
GeometryFactory::AABB GeometryFactory::CalculatePrimitiveAABB(const std::string& type, const Vec3& size)
{
    AABB aabb;
    
    if (type == "Cube") {
        // Cube is centered at origin with given dimensions
        Vec3 halfSize = size * 0.5f;
        aabb.min = Vec3(-halfSize.x, -halfSize.y, -halfSize.z);
        aabb.max = Vec3(halfSize.x, halfSize.y, halfSize.z);
    }
    else if (type == "Quad") {
        // Quad is in XY plane, centered at origin
        Vec3 halfSize = size * 0.5f;
        aabb.min = Vec3(-halfSize.x, -halfSize.y, 0.0f);
        aabb.max = Vec3(halfSize.x, halfSize.y, 0.0f);
    }
    else if (type == "Sphere") {
        // Sphere is centered at origin with radius = size.x (assuming uniform)
        float radius = size.x;
        aabb.min = Vec3(-radius, -radius, -radius);
        aabb.max = Vec3(radius, radius, radius);
    }
    else {
        // Default cube-like bounds
        Vec3 halfSize = size * 0.5f;
        aabb.min = Vec3(-halfSize.x, -halfSize.y, -halfSize.z);
        aabb.max = Vec3(halfSize.x, halfSize.y, halfSize.z);
    }
    
    return aabb;
}
