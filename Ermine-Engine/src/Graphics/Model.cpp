/* Start Header ************************************************************************/
/*!
\file       Model.cpp
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       27/09/2025
\brief      This file contains the definition of the Model class for loading and processing
            3D models using Assimp. Provides mesh data, bone data, and animation integration
            for rendering and animation systems.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "Model.h"

using namespace Ermine::graphics;

/**
 * @brief Construct a new Model by loading a file.
 * @param path Path to the 3D model file
 */
Model::Model(const std::string& path)
{
    LoadModel(path);
}

/**
 * @brief Load a model from file and process its nodes and meshes.
 * @param path Path to the model file
 */
void Model::LoadModel(const std::string& path)
{
    // Reset model data before loading
    m_meshes.clear();
    m_BoneMapping.clear();
    m_BoneOffsets.clear();
    m_BoneTransforms.clear();

    // Create importer owned by the Model instance
    m_Importer = std::make_unique<Assimp::Importer>();

    // Choose the flags required
    unsigned int flags = aiProcess_Triangulate
        | aiProcess_GenSmoothNormals
        | aiProcess_FlipUVs
        | aiProcess_LimitBoneWeights
        | aiProcess_JoinIdenticalVertices
        | aiProcess_CalcTangentSpace;

    const aiScene* scene = m_Importer->ReadFile(path, flags);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::string err = m_Importer->GetErrorString();
        EE_CORE_ERROR("ERROR::ASSIMP:: " + err);
        m_Scene = nullptr;
        return;
    }
    m_Scene = scene;

    m_directory = path.substr(0, path.find_last_of('/'));
	m_name = path.substr(path.find_last_of('/') + 1);
    ProcessNode(scene->mRootNode, scene, aiMatrix4x4());

    // Init bone transforms to identity
    m_BoneTransforms.resize(m_BoneOffsets.size(), glm::mat4(1.0f));
}

/**
 * @brief Recursively process Assimp nodes into MeshData.
 * @param node Current node
 * @param scene Assimp scene reference
 * @param parentTransform Parent transformation matrix
 */
void Model::ProcessNode(aiNode* node, const aiScene* scene, const aiMatrix4x4& parentTransform)
{
    aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;

    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        MeshData meshData = ProcessMesh(mesh);
        meshData.localTransform = ToGlm(nodeTransform);
        m_meshes.push_back(meshData);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
        ProcessNode(node->mChildren[i], scene, nodeTransform);
}

/**
 * @brief Convert an Assimp mesh into engine MeshData.
 * @param mesh Pointer to aiMesh
 * @return Processed MeshData
 */
MeshData Model::ProcessMesh(aiMesh* mesh)
{
    std::vector<unsigned int> indices;
    size_t vertexCount = mesh->mNumVertices;
    std::vector<VertexData> vertices(vertexCount);

    // Base vertex attributes
    for (unsigned int i = 0; i < vertexCount; ++i)
    {
        VertexData& vertex = vertices[i];

        // Position
        vertex.position[0] = mesh->mVertices[i].x;
        vertex.position[1] = mesh->mVertices[i].y;
        vertex.position[2] = mesh->mVertices[i].z;

        // Normal
        if (mesh->HasNormals())
        {
            vertex.normal[0] = mesh->mNormals[i].x;
            vertex.normal[1] = mesh->mNormals[i].y;
            vertex.normal[2] = mesh->mNormals[i].z;
        }

        // Texture coordinates
        if (mesh->mTextureCoords[0])
        {
            vertex.texCoords[0] = mesh->mTextureCoords[0][i].x;
            vertex.texCoords[1] = mesh->mTextureCoords[0][i].y;
        }
    }

    // Process bones
    for (unsigned int i = 0; i < mesh->mNumBones; ++i)
    {
        aiBone* ai_bone = mesh->mBones[i];
        std::string boneName(ai_bone->mName.C_Str());

        int boneIndex = 0;
        if (m_BoneMapping.find(boneName) == m_BoneMapping.end())
        {
            boneIndex = static_cast<int>(m_BoneOffsets.size());
            m_BoneMapping[boneName] = boneIndex;
            m_BoneOffsets.push_back(ToGlm(ai_bone->mOffsetMatrix));
        }
        else
            boneIndex = m_BoneMapping[boneName];

        // Add bone weights to vertices
        for (unsigned int w = 0; w < ai_bone->mNumWeights; ++w)
        {
            auto vw = ai_bone->mWeights[w];
            vertices[vw.mVertexId].AddBoneData(boneIndex, vw.mWeight);
        }
    }

    // Normalize weights per vertex
    for (unsigned int i = 0; i < vertexCount; ++i)
    {
        float total = 0.0f;
        for (int j = 0; j < MAX_BONE_INFLUENCE; ++j)
            total += vertices[i].Weights[j];
        if (total > 0.0f)
            for (int j = 0; j < MAX_BONE_INFLUENCE; ++j)
                vertices[i].Weights[j] /= total;
    }

    // Build indices
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    // Create GPU buffers
    auto vao = std::make_shared<VertexArray>();
    auto vbo = std::make_shared<VertexBuffer>(vertices.data(), vertices.size() * sizeof(VertexData));
    auto ibo = std::make_shared<IndexBuffer>(indices.data(), indices.size() * sizeof(unsigned int));

    vao->Bind();
    vbo->Bind();
    ibo->Bind();

    // Vertex attributes
    vao->LinkAttribute(0, 3, GL_FLOAT, sizeof(VertexData), (void*)offsetof(VertexData, position));
    vao->LinkAttribute(1, 3, GL_FLOAT, sizeof(VertexData), (void*)offsetof(VertexData, normal));
    vao->LinkAttribute(2, 2, GL_FLOAT, sizeof(VertexData), (void*)offsetof(VertexData, texCoords));

    // Bone IDs (integer)
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, MAX_BONE_INFLUENCE, GL_INT, sizeof(VertexData), (void*)offsetof(VertexData, IDs));

    // Bone weights (float)
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, Weights));

    vao->Unbind();
    vbo->Unbind();
    ibo->Unbind();

    return MeshData{ vao, vbo, ibo, glm::mat4(1.0f) };
}

std::vector<glm::vec3> Ermine::graphics::Model::GetMeshVertices() const
{
    std::vector<glm::vec3> vertices;

    // Iterate through all meshes
    for (const auto& mesh : m_meshes)
    {
        if (!mesh.vbo) continue;

        // Assuming your VertexBuffer stores VertexData in CPU-side memory
        // If you only have GPU-side data, you need to store a CPU copy when loading the mesh.
        const VertexData* vertexData = reinterpret_cast<const VertexData*>(mesh.vbo->GetDataPointer());
        if (!vertexData) continue;

        // Number of vertices
        unsigned int numVertices = mesh.vbo->GetSize() / sizeof(VertexData);

        for (unsigned int i = 0; i < numVertices; ++i)
        {
            vertices.emplace_back(
                vertexData[i].position[0],
                vertexData[i].position[1],
                vertexData[i].position[2]
            );
        }
    }

    return vertices;
}

std::vector<glm::vec3> Model::GetSkinnedVertices() const
{
    std::vector<glm::vec3> vertices;

    for (const auto& mesh : m_meshes)
    {
        if (!mesh.vbo) continue;

        const VertexData* vertexData = reinterpret_cast<const VertexData*>(mesh.vbo->GetDataPointer());
        if (!vertexData) continue;

        unsigned int numVertices = mesh.vbo->GetSize() / sizeof(VertexData);
        vertices.reserve(vertices.size() + numVertices);

        for (unsigned int i = 0; i < numVertices; ++i)
        {
            const VertexData& v = vertexData[i];
            glm::vec4 skinnedPos = glm::vec4(0.0f);

            // Apply bone transforms
            for (int j = 0; j < MAX_BONE_INFLUENCE; ++j)
            {
                int boneID = v.IDs[j];
                float weight = v.Weights[j];

                if (boneID < 0 || boneID >= static_cast<int>(m_BoneTransforms.size()))
                    continue;

                // BoneTransform = globalTransform * offset
                glm::mat4 transform = m_BoneTransforms[boneID];
                skinnedPos += transform * glm::vec4(v.position[0], v.position[1], v.position[2], 1.0f) * weight;
            }

            vertices.emplace_back(skinnedPos.x, skinnedPos.y, skinnedPos.z);
        }
    }

    return vertices;
}

