/* Start Header ************************************************************************/
/*!
\file       Model.h
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       10/09/2025
\brief      This file contains the declaration of the Model class.
            The Model class is used to load and render 3D models using Assimp.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"

#include "VertexArray.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Shader.h"
#include "Texture.h"
#include <string>
#include <vector>
#include <memory>
#include <assimp/scene.h>

namespace Ermine::graphics
{
    constexpr int MAX_BONE_INFLUENCE = 4;

    // Vertex Bone Data
    struct VertexData
    {
        float position[3];
        float normal[3];
        float texCoords[2];
        int IDs[MAX_BONE_INFLUENCE] = { 0,0,0,0 };
        float Weights[MAX_BONE_INFLUENCE] = { 0,0,0,0 };

        void AddBoneData(int boneID, float weight)
        {
            for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
            {
                if (Weights[i] == 0.0f)
                {
                    IDs[i] = boneID;
                    Weights[i] = weight;
                    return;
                }
            }
        }
    };

    // Mesh Data
    struct MeshData
    {
        std::shared_ptr<VertexArray> vao;
        std::shared_ptr<VertexBuffer> vbo;
        std::shared_ptr<IndexBuffer> ibo;
        std::shared_ptr<Texture> texture;
        glm::mat4 localTransform{ 1.0f };
    };

    // Model Class
    class Model
    {
    public:
        Model(const std::string& path);

        const std::vector<MeshData>& GetMeshes() const { return m_meshes; }

        int GetBoneCount() const { return static_cast<int>(m_BoneOffsets.size()); }
        const std::vector<glm::mat4>& GetBoneOffsets() const { return m_BoneOffsets; }
        const std::vector<glm::mat4>& GetBoneTransforms() const { return m_BoneTransforms; }

        void SetBoneTransforms(const std::vector<glm::mat4>& transforms) { m_BoneTransforms = transforms; }

    private:
        std::string m_directory; // directory for resolving textures
        std::vector<MeshData> m_meshes; // all meshes in this model

        // Bone data
        std::unordered_map<std::string, int> m_BoneMapping;
        std::vector<glm::mat4> m_BoneOffsets;
        std::vector<glm::mat4> m_BoneTransforms;

        glm::mat4 ToGlm(const aiMatrix4x4& from);

        void LoadModel(const std::string& path);
        void ProcessNode(aiNode* node, const aiScene* scene, const aiMatrix4x4& parentTransform);
        MeshData ProcessMesh(aiMesh* mesh, const aiScene* scene);
        std::shared_ptr<Texture> LoadMaterialTexture(aiMaterial* mat, aiTextureType type);
    };
}
