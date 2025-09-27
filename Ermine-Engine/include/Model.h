/* Start Header ************************************************************************/
/*!
\file       Model.h
\author     Lum Ko Sand, kosand.lum, 2301263, kosand.lum\@digipen.edu
\date       27/09/2025
\brief      This file contains the declaration of the Model class for loading and processing
            3D models using Assimp. Provides mesh data, bone data, and animation integration
            for rendering and animation systems.

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

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace Ermine::graphics
{
    constexpr int MAX_BONE_INFLUENCE = 4;

    /**
     * @brief Per-vertex data including bone IDs and weights for skinning.
     */
    struct VertexData
    {
        float position[3];                  // Position of the vertex
        float normal[3];                    // Normal vector
        float texCoords[2];                 // Texture coordinates
        int IDs[MAX_BONE_INFLUENCE];        // Bone IDs affecting this vertex
        float Weights[MAX_BONE_INFLUENCE];  // Bone weights corresponding to IDs

        /**
         * @brief Default constructor initializes all values to zero.
         */
        VertexData()
        {
            for (int i = 0; i < 3; ++i)
            {
                position[i] = 0.0f;
                normal[i] = 0.0f;
            }

            for (int i = 0; i < 2; ++i)
                texCoords[i] = 0.0f;

            for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
            {
                IDs[i] = 0;
                Weights[i] = 0.0f;
            }
        }

        /**
         * @brief Assigns a bone influence to the vertex.
         * @param boneID The bone index
         * @param weight Influence weight [0,1]
         */
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

    /**
     * @brief Mesh data wrapper for rendering.
     */
    struct MeshData
    {
        std::shared_ptr<VertexArray> vao;
        std::shared_ptr<VertexBuffer> vbo;
        std::shared_ptr<IndexBuffer> ibo;
        glm::mat4 localTransform{ 1.0f };
    };

    /**
     * @brief Loads and stores a 3D model and its associated mesh/bone data.
     */
    class Model
    {
    public:
        /**
         * @brief Construct a new Model by loading a file.
         * @param path Path to the 3D model file
         */
        Model(const std::string& path);

        // @return Directory of the model file
		const std::string& GetDirectory() const { return m_directory; }
		
        // @return Name of the model file
        const std::string& GetName() const { return m_name; }

        // @return List of meshes in the model
        const std::vector<MeshData>& GetMeshes() const { return m_meshes; }

        // @return Pointer to the original Assimp scene
        const aiScene* GetAssimpScene() const { return m_Scene; }

        // @return Mapping from bone name to index
        const std::unordered_map<std::string, int>& GetBoneMapping() const { return m_BoneMapping; }

        // @return Number of bones in the model
        int GetBoneCount() const { return static_cast<int>(m_BoneOffsets.size()); }

        // @return Bone offset matrices (bind-pose transforms)
        const std::vector<glm::mat4>& GetBoneOffsets() const { return m_BoneOffsets; }

        // @return Current bone transforms from animation
        const std::vector<glm::mat4>& GetBoneTransforms() const { return m_BoneTransforms; }

        // @brief Set the animated bone transforms
        void SetBoneTransforms(const std::vector<glm::mat4>& transforms) { m_BoneTransforms = transforms; }
        void LoadModel(const std::string& path);

        /**
         * @brief Convert an Assimp matrix to a glm::mat4.
         * @param from Input aiMatrix4x4
         * @return Equivalent glm::mat4
         */
        glm::mat4 ToGlm(const aiMatrix4x4& from) {
            glm::mat4 to;
            to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
            to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
            to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
            to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
            return to;
        }

    private:
        std::string m_directory;                            // Directory of the model
        std::string m_name;                                 // Name of the model
        std::vector<MeshData> m_meshes;                     // All meshes of the model

        std::unique_ptr<Assimp::Importer> m_Importer;       // Assimp importer
        const aiScene* m_Scene = nullptr;                   // Raw Assimp scene

        // Bone data
        std::unordered_map<std::string, int> m_BoneMapping; // Name-to-index bone map
        std::vector<glm::mat4> m_BoneOffsets;               // Bone offset matrices
        std::vector<glm::mat4> m_BoneTransforms;            // Final bone transforms (for rendering)

        // Load the model and process nodes
        void LoadModel(const std::string& path);

        // Recursively process Assimp nodes
        void ProcessNode(aiNode* node, const aiScene* scene, const aiMatrix4x4& parentTransform);

        // Process an Assimp mesh into engine MeshData
        MeshData ProcessMesh(aiMesh* mesh);
    };
}
