/* Start Header ************************************************************************/
/*!
\file       Model.cpp
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       10/09/2025
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
        MeshData meshData = ProcessMesh(mesh, scene);
        meshData.localTransform = ToGlm(nodeTransform);
        m_meshes.push_back(meshData);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, nodeTransform);
    }

    //std::cout << "Node: " << node->mName.C_Str()
    //    << " meshes: " << node->mNumMeshes
    //    << " children: " << node->mNumChildren << "\n";
}

MeshData Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<float> vertexData;
    std::vector<unsigned int> indices;

    size_t vertexCount = mesh->mNumVertices;
    std::vector<VertexBoneData> bones(vertexCount);

    // Base vertex attributes
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        // pos
        vertexData.push_back(mesh->mVertices[i].x);
        vertexData.push_back(mesh->mVertices[i].y);
        vertexData.push_back(mesh->mVertices[i].z);

        // normal
        if (mesh->HasNormals())
        {
            vertexData.push_back(mesh->mNormals[i].x);
            vertexData.push_back(mesh->mNormals[i].y);
            vertexData.push_back(mesh->mNormals[i].z);
        }
        else
        {
            vertexData.push_back(0.f); vertexData.push_back(0.f); vertexData.push_back(0.f);
        }

        // uv
        if (mesh->mTextureCoords[0])
        {
            vertexData.push_back(mesh->mTextureCoords[0][i].x);
            vertexData.push_back(mesh->mTextureCoords[0][i].y);
        }
        else
        {
            vertexData.push_back(0.f); vertexData.push_back(0.f);
        }

        // reserve space for 4 bone IDs + 4 weights
        for (int k = 0; k < MAX_BONE_INFLUENCE; ++k) vertexData.push_back(0.0f); // IDs as floats
        for (int k = 0; k < MAX_BONE_INFLUENCE; ++k) vertexData.push_back(0.0f); // weights
    }

    // indices
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
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
        {
            boneIndex = m_BoneMapping[boneName];
        }

        for (unsigned int w = 0; w < ai_bone->mNumWeights; ++w)
        {
            auto vw = ai_bone->mWeights[w];
            bones[vw.mVertexId].AddBoneData(boneIndex, vw.mWeight);
        }
    }

    // interleave bone data into vertexData
    const int floatsPerVertex = 3 + 3 + 2 + MAX_BONE_INFLUENCE + MAX_BONE_INFLUENCE;
    for (size_t v = 0; v < vertexCount; ++v)
    {
        int base = (int)v * floatsPerVertex + 8; // skip pos(3)+norm(3)+uv(2)

        for (int k = 0; k < MAX_BONE_INFLUENCE; ++k)
            vertexData[base + k] = (float)bones[v].IDs[k];

        int weightOffset = base + MAX_BONE_INFLUENCE;
        for (int k = 0; k < MAX_BONE_INFLUENCE; ++k)
            vertexData[weightOffset + k] = bones[v].Weights[k];
    }

    // GPU buffers
    auto vao = std::make_shared<VertexArray>();
    auto vbo = std::make_shared<VertexBuffer>(vertexData.data(), vertexData.size() * sizeof(float));
    auto ibo = std::make_shared<IndexBuffer>(indices.data(), indices.size());

    vao->Bind();
    vbo->Bind();

    // Attributes
    vao->LinkAttribute(0, 3, GL_FLOAT, floatsPerVertex * sizeof(float), (void*)(0));
    vao->LinkAttribute(1, 3, GL_FLOAT, floatsPerVertex * sizeof(float), (void*)(3 * sizeof(float)));
    vao->LinkAttribute(2, 2, GL_FLOAT, floatsPerVertex * sizeof(float), (void*)(6 * sizeof(float)));

    // Bone IDs (int)
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 4, GL_INT, floatsPerVertex * sizeof(float), (void*)(8 * sizeof(float)));

    // Bone weights (float)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, floatsPerVertex * sizeof(float), (void*)((8 + MAX_BONE_INFLUENCE) * sizeof(float)));

    vao->Unbind();
    vbo->Unbind();
    ibo->Unbind();

    MeshData meshData{ vao, vbo, ibo, nullptr, glm::mat4(1.0f) };

    // load diffuse texture if available
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        meshData.texture = LoadMaterialTexture(material, aiTextureType_DIFFUSE);
    }

    //if (mesh->HasBones())
    //    std::cout << "Mesh " << mesh->mName.C_Str()
    //    << " has " << mesh->mNumBones << " bones\n";

    return meshData;
}

std::shared_ptr<Texture> Model::LoadMaterialTexture(aiMaterial* mat, aiTextureType type)
{
    if (mat->GetTextureCount(type) > 0)
    {
        aiString str;
        mat->GetTexture(type, 0, &str);
        std::string texPath = m_directory + "/" + std::string(str.C_Str());
        return AssetManager::GetInstance().LoadTexture(texPath);
    }
    return nullptr;
}
