#include <PreCompile.h>
#include "ResourcePipe.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <algorithm>

// DirectXTex
#include <DirectXTex.h>

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Ermine {

    //=============================================================================
    // TextureImportSettings
    //=============================================================================
    TextureImportSettings::TextureImportSettings()
        : targetFormat(DXGI_FORMAT_B8G8R8A8_UNORM) {
    }

    //=============================================================================
    // ResourcePipeline
    //=============================================================================
    ResourcePipeline::ResourcePipeline()
        : m_Initialized(false)
        , m_COMInitialized(false) {
    }

    ResourcePipeline::~ResourcePipeline() {
        Shutdown();
    }

    bool ResourcePipeline::Initialize(const std::string& projectRootPath) {
        if (m_Initialized) {
            std::cout << "[ResourcePipeline] Already initialized." << std::endl;
            return true;
        }

        m_ProjectPath = std::filesystem::absolute(projectRootPath).string();
        m_CachePath = (std::filesystem::path(m_ProjectPath) / "Cache").string();

        std::cout << "[ResourcePipeline] Initializing..." << std::endl;
        std::cout << "  Project Path: " << m_ProjectPath << std::endl;
        std::cout << "  Cache Path: " << m_CachePath << std::endl;

        // Create cache directory
        std::filesystem::create_directories(m_CachePath);

        // Initialize DirectXTex (COM)
        if (!InitializeDirectXTex()) {
            std::cerr << "[ResourcePipeline] Failed to initialize DirectXTex." << std::endl;
            return false;
        }

        // Initialize asset database
        if (!m_Database.Initialize(m_ProjectPath)) {
            std::cerr << "[ResourcePipeline] Failed to initialize asset database." << std::endl;
            CleanupDirectXTex();
            return false;
        }

        m_Initialized = true;
        std::cout << "[ResourcePipeline] Initialization complete!" << std::endl;

        return true;
    }

    void ResourcePipeline::Shutdown() {
        if (!m_Initialized) return;

        std::cout << "[ResourcePipeline] Shutting down..." << std::endl;

        // Save database before shutdown
        m_Database.Save();

        // Cleanup DirectXTex
        CleanupDirectXTex();

        m_Initialized = false;
    }

    bool ResourcePipeline::InitializeDirectXTex() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
            std::cerr << "[ResourcePipeline] Failed to initialize COM for DirectXTex." << std::endl;
            return false;
        }
        m_COMInitialized = true;
        return true;
    }

    void ResourcePipeline::CleanupDirectXTex() {
        if (m_COMInitialized) {
            CoUninitialize();
            m_COMInitialized = false;
        }
    }

    ImportResult ResourcePipeline::ImportTexture(const std::string& sourcePath,
        const TextureImportSettings& settings) {
        if (!m_Initialized) {
            return { false, "Pipeline not initialized" };
        }

        auto startTime = std::chrono::high_resolution_clock::now();

        // Convert to absolute path
        std::string absoluteSource = std::filesystem::absolute(sourcePath).string();

        if (!std::filesystem::exists(absoluteSource)) {
            return { false, "Source file does not exist: " + absoluteSource };
        }

        // Check if asset already exists in database
        AssetEntry* existingEntry = m_Database.FindBySourcePath(absoluteSource);
        xresource::full_guid guid;

        if (existingEntry) {
            // Reuse existing GUID
            guid = existingEntry->guid;
            std::cout << "[ResourcePipeline] Reimporting texture: " << std::filesystem::path(sourcePath).filename() << std::endl;
        }
        else {
            // Generate new GUID
            auto instanceGuid = xresource::instance_guid::GenerateGUIDCopy();
            guid = xresource::full_guid{ instanceGuid, TEXTURE_TYPE_GUID };
            std::cout << "[ResourcePipeline] Importing new texture: " << std::filesystem::path(sourcePath).filename() << std::endl;
        }

        // Generate output path
        std::string outputPath = GenerateCachePath(guid, ".dds");

        // Import the texture
        ImportResult result = ImportTextureInternal(absoluteSource, outputPath, settings);

        if (result.success) {
            // Update database
            AssetEntry entry;
            entry.guid = guid;
            entry.sourcePath = absoluteSource;
            entry.cachedPath = outputPath;
            entry.lastModified = std::filesystem::last_write_time(absoluteSource);
            entry.resourceType = "TEXTURE";

            if (existingEntry) {
                m_Database.UpdateAsset(absoluteSource, entry);
            }
            else {
                m_Database.RegisterAsset(entry);
            }

            // Save database
            m_Database.Save();

            result.resourceGuid = guid;
            result.outputPath = outputPath;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.importTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

        return result;
    }

    ImportResult ResourcePipeline::ImportMesh(const std::string& sourcePath,
        const MeshImportSettings& settings) {
        if (!m_Initialized) {
            return { false, "Pipeline not initialized" };
        }

        auto startTime = std::chrono::high_resolution_clock::now();

        // Convert to absolute path
        std::string absoluteSource = std::filesystem::absolute(sourcePath).string();

        if (!std::filesystem::exists(absoluteSource)) {
            return { false, "Source file does not exist: " + absoluteSource };
        }

        // Load scene to determine if it's skinned or static
        Assimp::Importer importer;
        const aiScene* scene = nullptr;
        if (!LoadAssimpScene(absoluteSource, scene, importer, settings)) {
            return { false, "Failed to load mesh with Assimp" };
        }

        if (scene->mNumMeshes == 0) {
            return { false, "No meshes found in file" };
        }

        // Determine if skinned or static
        bool isSkinned = HasSkinning(scene->mMeshes[0]);
        xresource::type_guid typeGuid = isSkinned ? SKINNED_MESH_TYPE_GUID : STATIC_MESH_TYPE_GUID;
        std::string resourceType = isSkinned ? "SKINNED_MESH" : "STATIC_MESH";
        std::string extension = isSkinned ? ".skin" : ".mesh";

        // Check if asset already exists in database
        AssetEntry* existingEntry = m_Database.FindBySourcePath(absoluteSource);
        xresource::full_guid guid;

        if (existingEntry) {
            guid = existingEntry->guid;
            std::cout << "[ResourcePipeline] Reimporting " << resourceType << ": "
                << std::filesystem::path(sourcePath).filename() << std::endl;
        }
        else {
            auto instanceGuid = xresource::instance_guid::GenerateGUIDCopy();
            guid = xresource::full_guid{ instanceGuid, typeGuid };
            std::cout << "[ResourcePipeline] Importing new " << resourceType << ": "
                << std::filesystem::path(sourcePath).filename() << std::endl;
        }

        // Generate output path
        std::string outputPath = GenerateCachePath(guid, extension);

        // Import the mesh
        ImportResult result = ImportMeshInternal(absoluteSource, outputPath, settings);

        if (result.success) {
            // Update database
            AssetEntry entry;
            entry.guid = guid;
            entry.sourcePath = absoluteSource;
            entry.cachedPath = outputPath;
            entry.lastModified = std::filesystem::last_write_time(absoluteSource);
            entry.resourceType = resourceType;

            if (existingEntry) {
                m_Database.UpdateAsset(absoluteSource, entry);
            }
            else {
                m_Database.RegisterAsset(entry);
            }

            m_Database.Save();

            result.resourceGuid = guid;
            result.outputPath = outputPath;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.importTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

        return result;
    }

    bool ResourcePipeline::NeedsReimport(const std::string& sourcePath) const {
        return m_Database.NeedsReimport(sourcePath);
    }

    ImportResult ResourcePipeline::ReimportAsset(const std::string& sourcePath) {
        std::string extension = std::filesystem::path(sourcePath).extension().string();

        if (IsTextureFile(extension)) {
            return ImportTexture(sourcePath);
        }
        else if (IsMeshFile(extension)) {
            return ImportMesh(sourcePath);
        }

        return { false, "Unknown file type: " + extension };
    }

    void ResourcePipeline::ScanAndImportAssets(const std::string& assetsFolder) {
        if (!m_Initialized) {
            std::cerr << "[ResourcePipeline] Cannot scan - pipeline not initialized." << std::endl;
            return;
        }

        std::string fullAssetsPath = (std::filesystem::path(m_ProjectPath) / assetsFolder).string();

        if (!std::filesystem::exists(fullAssetsPath)) {
            std::cerr << "[ResourcePipeline] Assets folder does not exist: " << fullAssetsPath << std::endl;
            return;
        }

        std::cout << "[ResourcePipeline] Scanning for assets in: " << fullAssetsPath << std::endl;

        int importedCount = 0;
        int skippedCount = 0;
        int failedCount = 0;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(fullAssetsPath)) {
            if (!entry.is_regular_file()) continue;

            std::string extension = entry.path().extension().string();
            std::string filePath = entry.path().string();

            // Skip if not a supported asset type
            if (!IsTextureFile(extension) && !IsMeshFile(extension)) {
                continue;
            }

            // Check if needs reimport
            if (!NeedsReimport(filePath)) {
                std::cout << "  ⏭ Skipping (up-to-date): " << entry.path().filename() << std::endl;
                skippedCount++;
                continue;
            }

            // Import the asset
            ImportResult result = ReimportAsset(filePath);

            if (result.success) {
                std::cout << "  ✓ Imported: " << entry.path().filename()
                    << " (" << result.importTimeMs << "ms)" << std::endl;
                importedCount++;
            }
            else {
                std::cerr << "  ✗ Failed: " << entry.path().filename()
                    << " - " << result.errorMessage << std::endl;
                failedCount++;
            }
        }

        std::cout << "[ResourcePipeline] Scan complete: " << importedCount << " imported, "
            << skippedCount << " skipped, " << failedCount << " failed" << std::endl;
    }

    //=============================================================================
    // Helper Functions
    //=============================================================================
    std::string ResourcePipeline::GenerateCachePath(const xresource::full_guid& guid,
        const std::string& extension) const {
        std::string guidStr = std::to_string(guid.m_Instance.m_Value);
        return (std::filesystem::path(m_CachePath) / (guidStr + extension)).string();
    }

    bool ResourcePipeline::IsTextureFile(const std::string& extension) const {
        std::string ext = extension;
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
            ext == ".tga" || ext == ".bmp" || ext == ".dds";
    }

    bool ResourcePipeline::IsMeshFile(const std::string& extension) const {
        std::string ext = extension;
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".fbx" || ext == ".obj" || ext == ".gltf" ||
            ext == ".glb" || ext == ".dae";
    }

    //=============================================================================
    // Texture Import Implementation
    //=============================================================================
    ImportResult ResourcePipeline::ImportTextureInternal(const std::string& sourcePath,
        const std::string& outputPath,
        const TextureImportSettings& settings) {
        if (!ConvertTextureToDDS(sourcePath, outputPath, settings)) {
            return { false, "Failed to convert texture to DDS" };
        }

        size_t fileSize = std::filesystem::file_size(outputPath);

        return {
            true,
            "",
            outputPath,
            {},
            fileSize
        };
    }

    bool ResourcePipeline::ConvertTextureToDDS(const std::string& inputPath,
        const std::string& outputPath,
        const TextureImportSettings& settings) {
        using namespace DirectX;

        // Convert paths to wide strings
        std::wstring wInput(inputPath.begin(), inputPath.end());
        std::wstring wOutput(outputPath.begin(), outputPath.end());

        // Load the image
        ScratchImage image;
        HRESULT hr = LoadFromWICFile(wInput.c_str(), WIC_FLAGS_NONE, nullptr, image);
        if (FAILED(hr)) {
            std::cerr << "    Failed to load texture with WIC" << std::endl;
            return false;
        }

        TexMetadata metadata = image.GetMetadata();

        // Convert to target format if needed
        ScratchImage converted;
        if (metadata.format != settings.targetFormat) {
            hr = Convert(image.GetImages(), image.GetImageCount(), metadata,
                settings.targetFormat, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, converted);
            if (FAILED(hr)) {
                std::cerr << "    Failed to convert texture format" << std::endl;
                return false;
            }
            metadata = converted.GetMetadata();
        }
        else {
            converted = std::move(image);
        }

        // Generate mipmaps if requested
        ScratchImage mipChain;
        if (settings.generateMipmaps) {
            hr = GenerateMipMaps(converted.GetImages(), converted.GetImageCount(),
                metadata, TEX_FILTER_DEFAULT, 0, mipChain);
            if (FAILED(hr)) {
                mipChain = std::move(converted);
            }
        }
        else {
            mipChain = std::move(converted);
        }

        // Save to DDS
        hr = SaveToDDSFile(mipChain.GetImages(), mipChain.GetImageCount(),
            mipChain.GetMetadata(), DDS_FLAGS_NONE, wOutput.c_str());

        if (FAILED(hr)) {
            std::cerr << "    Failed to save DDS file" << std::endl;
            return false;
        }

        return true;
    }

    //=============================================================================
    // Mesh Import Implementation
    //=============================================================================
    ImportResult ResourcePipeline::ImportMeshInternal(const std::string& sourcePath,
        const std::string& outputPath,
        const MeshImportSettings& settings) {
        // Load scene
        Assimp::Importer importer;
        const aiScene* scene = nullptr;

        if (!LoadAssimpScene(sourcePath, scene, importer, settings)) {
            return { false, "Failed to load scene with Assimp" };
        }

        if (scene->mNumMeshes == 0) {
            return { false, "No meshes found in scene" };
        }

        // Determine if skinned
        bool isSkinned = HasSkinning(scene->mMeshes[0]);

        bool success = false;
        if (isSkinned) {
            // Process as skinned mesh
            SkinnedMeshData skinnedData;
            if (ProcessSkinnedMeshCombined(scene, skinnedData)) {
                success = WriteSkinFile(outputPath, skinnedData);
            }
        }
        else {
            // Process as static mesh
            MeshData meshData;
            if (ProcessStaticMesh(scene->mMeshes[0], meshData)) {
                success = WriteMeshFile(outputPath, meshData);
            }
        }

        if (!success) {
            return { false, "Failed to process and write mesh data" };
        }

        size_t fileSize = std::filesystem::file_size(outputPath);

        return {
            true,
            "",
            outputPath,
            {},
            fileSize
        };
    }

    bool ResourcePipeline::LoadAssimpScene(const std::string& filePath,
        const aiScene*& outScene,
        Assimp::Importer& importer,
        const MeshImportSettings& settings) {
        unsigned int importFlags =
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType |
            aiProcess_LimitBoneWeights;

        if (settings.generateNormals) {
            importFlags |= aiProcess_GenNormals;
        }

        if (settings.generateTangents) {
            importFlags |= aiProcess_CalcTangentSpace;
        }

        if (settings.flipUVs) {
            importFlags |= aiProcess_FlipUVs;
        }

        if (settings.optimizeVertices) {
            importFlags |= aiProcess_ImproveCacheLocality;
        }

        outScene = importer.ReadFile(filePath, importFlags);

        if (!outScene || outScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !outScene->mRootNode) {
            std::cerr << "    Assimp error: " << importer.GetErrorString() << std::endl;
            return false;
        }

        return true;
    }

    bool ResourcePipeline::HasSkinning(const aiMesh* mesh) const {
        return mesh->HasBones();
    }

    bool ResourcePipeline::ProcessStaticMesh(const aiMesh* mesh, MeshData& outData) {
        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex = {};

            // Position
            vertex.position[0] = mesh->mVertices[i].x;
            vertex.position[1] = mesh->mVertices[i].y;
            vertex.position[2] = mesh->mVertices[i].z;

            // Normal
            if (mesh->HasNormals()) {
                vertex.normal[0] = mesh->mNormals[i].x;
                vertex.normal[1] = mesh->mNormals[i].y;
                vertex.normal[2] = mesh->mNormals[i].z;
            }

            // Texture coordinates
            if (mesh->HasTextureCoords(0)) {
                vertex.texCoord[0] = mesh->mTextureCoords[0][i].x;
                vertex.texCoord[1] = mesh->mTextureCoords[0][i].y;
            }

            // Tangent
            if (mesh->HasTangentsAndBitangents()) {
                vertex.tangent[0] = mesh->mTangents[i].x;
                vertex.tangent[1] = mesh->mTangents[i].y;
                vertex.tangent[2] = mesh->mTangents[i].z;
            }

            outData.vertices.push_back(vertex);
        }

        // Process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                outData.indices.push_back(face.mIndices[j]);
            }
        }

        return !outData.vertices.empty();
    }

    bool ResourcePipeline::ProcessSkinnedMeshCombined(const aiScene* scene, SkinnedMeshData& outData) {
        // Build bone mapping from ALL meshes
        std::map<std::string, int> boneMapping;

        // First pass: collect all unique bones
        for (unsigned int meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
            const aiMesh* sceneMesh = scene->mMeshes[meshIdx];
            if (!sceneMesh->HasBones()) continue;

            for (unsigned int i = 0; i < sceneMesh->mNumBones; i++) {
                aiBone* bone = sceneMesh->mBones[i];
                std::string boneName = bone->mName.C_Str();

                if (boneMapping.find(boneName) == boneMapping.end()) {
                    BoneInfo boneInfo;
                    boneInfo.name = boneName;

                    // Convert aiMatrix4x4 to flat array
                    for (int row = 0; row < 4; row++) {
                        for (int col = 0; col < 4; col++) {
                            boneInfo.offsetMatrix[row * 4 + col] = bone->mOffsetMatrix[row][col];
                        }
                    }

                    boneInfo.index = (int)outData.bones.size();

                    outData.bones.push_back(boneInfo);
                    boneMapping[boneName] = boneInfo.index;
                }
            }
        }

        // Process all meshes
        for (unsigned int meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
            const aiMesh* mesh = scene->mMeshes[meshIdx];

            if (!mesh->HasBones()) {
                std::cout << "    ⚠ Skipping non-skinned mesh: " << mesh->mName.C_Str() << std::endl;
                continue;
            }

            uint32_t baseVertex = (uint32_t)outData.vertices.size();

            // Initialize vertex weights
            std::vector<std::vector<std::pair<int, float>>> vertexWeights(mesh->mNumVertices);

            // Process bone weights
            for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
                aiBone* bone = mesh->mBones[boneIndex];
                int globalBoneIndex = boneMapping[bone->mName.C_Str()];

                for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; weightIndex++) {
                    aiVertexWeight weight = bone->mWeights[weightIndex];
                    vertexWeights[weight.mVertexId].push_back({ globalBoneIndex, weight.mWeight });
                }
            }

            // Process vertices
            for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
                SkinnedVertex vertex = {};

                // Position
                vertex.position[0] = mesh->mVertices[i].x;
                vertex.position[1] = mesh->mVertices[i].y;
                vertex.position[2] = mesh->mVertices[i].z;

                // Normal
                if (mesh->HasNormals()) {
                    vertex.normal[0] = mesh->mNormals[i].x;
                    vertex.normal[1] = mesh->mNormals[i].y;
                    vertex.normal[2] = mesh->mNormals[i].z;
                }

                // Texture coordinates
                if (mesh->HasTextureCoords(0)) {
                    vertex.texCoord[0] = mesh->mTextureCoords[0][i].x;
                    vertex.texCoord[1] = mesh->mTextureCoords[0][i].y;
                }

                // Tangent
                if (mesh->HasTangentsAndBitangents()) {
                    vertex.tangent[0] = mesh->mTangents[i].x;
                    vertex.tangent[1] = mesh->mTangents[i].y;
                    vertex.tangent[2] = mesh->mTangents[i].z;
                }

                // Bone weights (limit to 4, sorted by weight)
                auto& weights = vertexWeights[i];
                std::sort(weights.begin(), weights.end(),
                    [](const auto& a, const auto& b) { return a.second > b.second; });

                // Initialize with defaults
                for (int j = 0; j < 4; j++) {
                    vertex.boneIndices[j] = 0;
                    vertex.boneWeights[j] = 0.0f;
                }

                // Fill in actual weights (up to 4)
                float totalWeight = 0.0f;
                int weightCount = std::min(4, (int)weights.size());

                for (int j = 0; j < weightCount; j++) {
                    vertex.boneIndices[j] = weights[j].first;
                    vertex.boneWeights[j] = weights[j].second;
                    totalWeight += weights[j].second;
                }

                // Normalize weights to sum to 1.0
                if (totalWeight > 0.0f) {
                    for (int j = 0; j < 4; j++) {
                        vertex.boneWeights[j] /= totalWeight;
                    }
                }
                else {
                    // Vertex has no weights - assign to bone 0
                    vertex.boneIndices[0] = 0;
                    vertex.boneWeights[0] = 1.0f;
                }

                outData.vertices.push_back(vertex);
            }

            // Process indices (offset by base vertex)
            for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
                aiFace face = mesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; j++) {
                    outData.indices.push_back(baseVertex + face.mIndices[j]);
                }
            }
        }

        return !outData.vertices.empty();
    }

    bool ResourcePipeline::WriteMeshFile(const std::string& outputPath, const MeshData& meshData) {
        std::ofstream file(outputPath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "    Failed to open output file: " << outputPath << std::endl;
            return false;
        }

        // Write header
        file.write("MESH", 4);
        uint32_t version = 1;
        file.write((char*)&version, sizeof(version));

        // Write vertex count and data
        uint32_t vertexCount = (uint32_t)meshData.vertices.size();
        file.write((char*)&vertexCount, sizeof(vertexCount));
        file.write((char*)meshData.vertices.data(), vertexCount * sizeof(Vertex));

        // Write index count and data
        uint32_t indexCount = (uint32_t)meshData.indices.size();
        file.write((char*)&indexCount, sizeof(indexCount));
        file.write((char*)meshData.indices.data(), indexCount * sizeof(uint32_t));

        return true;
    }

    bool ResourcePipeline::WriteSkinFile(const std::string& outputPath, const SkinnedMeshData& meshData) {
        std::ofstream file(outputPath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "    Failed to open output file: " << outputPath << std::endl;
            return false;
        }

        // Write header
        file.write("SKIN", 4);
        uint32_t version = 1;
        file.write((char*)&version, sizeof(version));

        // Write vertex count and data
        uint32_t vertexCount = (uint32_t)meshData.vertices.size();
        file.write((char*)&vertexCount, sizeof(vertexCount));
        file.write((char*)meshData.vertices.data(), vertexCount * sizeof(SkinnedVertex));

        // Write index count and data
        uint32_t indexCount = (uint32_t)meshData.indices.size();
        file.write((char*)&indexCount, sizeof(indexCount));
        file.write((char*)meshData.indices.data(), indexCount * sizeof(uint32_t));

        // Write bone count
        uint32_t boneCount = (uint32_t)meshData.bones.size();
        file.write((char*)&boneCount, sizeof(boneCount));

        // Write bone data
        for (const auto& bone : meshData.bones) {
            // Write bone name
            uint32_t nameLen = (uint32_t)bone.name.length();
            file.write((char*)&nameLen, sizeof(nameLen));
            file.write(bone.name.c_str(), nameLen);

            // Write bone index
            file.write((char*)&bone.index, sizeof(bone.index));

            // Write offset matrix
            file.write((char*)bone.offsetMatrix, sizeof(bone.offsetMatrix));
        }

        return true;
    }

} // namespace Ermine