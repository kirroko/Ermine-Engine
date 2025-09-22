/* Start Header ************************************************************************/
/*!
\file       Model.cpp
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       19/09/2025
\brief      This file contains the definition of the Model class.
            The Model class is used to load and render 3D models using Assimp.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "Model.h"
#include "AssetManager.h" // for loading textures
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <iostream>

using namespace Ermine::graphics;

glm::mat4 Model::ToGlm(const aiMatrix4x4& from)
{
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

Model::Model(const std::string& path)
{
    LoadModel(path);
}

void Model::LoadModel(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }

    m_directory = path.substr(0, path.find_last_of('/'));
    ProcessNode(scene->mRootNode, scene, aiMatrix4x4());

    // Init bone transforms to identity
    m_BoneTransforms.resize(m_BoneOffsets.size(), glm::mat4(1.0f));
}

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

    //std::cout << "Node: " << node->mName.C_Str()
    //    << " meshes: " << node->mNumMeshes
    //    << " children: " << node->mNumChildren << "\n";
}

MeshData Model::ProcessMesh(aiMesh* mesh)
{
    std::vector<unsigned int> indices;

    size_t vertexCount = mesh->mNumVertices;
    std::vector<VertexData> vertices(vertexCount);

    // Base vertex attributes
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        VertexData& vertex = vertices[i];

        // pos
        vertex.position[0] = mesh->mVertices[i].x;
        vertex.position[1] = mesh->mVertices[i].y;
        vertex.position[2] = mesh->mVertices[i].z;

        // normal
        if (mesh->HasNormals())
        {
            vertex.normal[0] = mesh->mNormals[i].x;
            vertex.normal[1] = mesh->mNormals[i].y;
            vertex.normal[2] = mesh->mNormals[i].z;
        }
        else
        {
            vertex.normal[0] = 0.f;
            vertex.normal[1] = 0.f;
            vertex.normal[2] = 0.f;
        }

        // uv
        if (mesh->mTextureCoords[0])
        {
            vertex.texCoords[0] = mesh->mTextureCoords[0][i].x;
            vertex.texCoords[1] = mesh->mTextureCoords[0][i].y;
        }
        else
        {
            vertex.texCoords[0] = 0.f;
            vertex.texCoords[1] = 0.f;
        }
    }

    // bones
    for (unsigned int i = 0; i < mesh->mNumBones; ++i)
    {
        aiBone* ai_bone = mesh->mBones[i];
        std::string boneName(ai_bone->mName.C_Str());

        int boneIndex = 0;
        if (m_BoneMapping.find(boneName) == m_BoneMapping.end())
        {
            boneIndex = (int)m_BoneOffsets.size();
            m_BoneMapping[boneName] = boneIndex;
            m_BoneOffsets.push_back(ToGlm(ai_bone->mOffsetMatrix));
        }
        else
            boneIndex = m_BoneMapping[boneName];

        for (unsigned int w = 0; w < ai_bone->mNumWeights; ++w)
        {
            auto vw = ai_bone->mWeights[w];
            vertices[vw.mVertexId].AddBoneData(boneIndex, vw.mWeight);
        }
    }

    // indices
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    // GPU buffers
    auto vao = std::make_shared<VertexArray>();
    auto vbo = std::make_shared<VertexBuffer>(vertices.data(), vertices.size() * sizeof(VertexData));
    auto ibo = std::make_shared<IndexBuffer>(indices.data(), indices.size() * sizeof(unsigned int)); // Main Issue - was using indices.size() count instead of byte size

    vao->Bind();
    vbo->Bind();

    // Attributes
    vao->LinkAttribute(0, 3, GL_FLOAT, sizeof(VertexData), (void*)offsetof(VertexData, position));
    vao->LinkAttribute(1, 3, GL_FLOAT, sizeof(VertexData), (void*)offsetof(VertexData, normal));
    vao->LinkAttribute(2, 2, GL_FLOAT, sizeof(VertexData), (void*)offsetof(VertexData, texCoords));

    // Bone IDs (int)
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, MAX_BONE_INFLUENCE, GL_INT, sizeof(VertexData), (void*)offsetof(VertexData, IDs));

    // Bone weights (float)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, Weights));

    vao->Unbind();
    vbo->Unbind();
    ibo->Unbind();

    //if (mesh->HasBones())
    //    std::cout << "Mesh " << mesh->mName.C_Str()
    //    << " has " << mesh->mNumBones << " bones\n";

    return MeshData{ vao, vbo, ibo, glm::mat4(1.0f) };
}
