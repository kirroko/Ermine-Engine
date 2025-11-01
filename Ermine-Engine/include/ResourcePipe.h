#pragma once
//#include "PreCompile.h"
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <dxgiformat.h> 
#include "AssetDatabase.h"


struct aiScene;
struct aiMesh;
struct aiBone;
struct aiVertexWeight;
namespace Assimp {
    class Importer;
}

namespace Ermine {

    // Type GUIDs for resources (should match your offline pipeline)
    inline constexpr xresource::type_guid TEXTURE_TYPE_GUID("TEXTURE_RESOURCE_TYPE");
    inline constexpr xresource::type_guid STATIC_MESH_TYPE_GUID("STATIC_MESH_RESOURCE");
    inline constexpr xresource::type_guid SKINNED_MESH_TYPE_GUID("SKINNED_MESH_RESOURCE");

    //=============================================================================
    // Mesh Data Structures
    //=============================================================================
    struct Vertex {
        float position[3];
        float normal[3];
        float texCoord[2];
        float tangent[3];
    };

    struct SkinnedVertex {
        float position[3];
        float normal[3];
        float texCoord[2];
        float tangent[3];
        int boneIndices[4];
        float boneWeights[4];
    };

    struct BoneInfo {
        std::string name;
        float offsetMatrix[16]; // 4x4 matrix stored as flat array
        int index;
    };

    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    struct SkinnedMeshData {
        std::vector<SkinnedVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<BoneInfo> bones;
    };

    //=============================================================================
    // Import Settings
    //=============================================================================
    struct TextureImportSettings {
        bool generateMipmaps = true;
        bool useCompression = false;
        DXGI_FORMAT targetFormat;

        TextureImportSettings();
    };

    struct MeshImportSettings {
        bool generateNormals = true;
        bool generateTangents = true;
        bool optimizeVertices = true;
        bool flipUVs = true;
        bool combineMeshes = true; // For skinned meshes with multiple sub-meshes
    };

    //=============================================================================
    // Import Result
    //=============================================================================
    struct ImportResult {
        bool success = false;
        std::string errorMessage;
        std::string outputPath;
        xresource::full_guid resourceGuid;
        size_t outputSize = 0;
        float importTimeMs = 0.0f;
    };

    //=============================================================================
    // ResourcePipeline - Main runtime asset pipeline
    //=============================================================================
    class ResourcePipeline {
    public:
        ResourcePipeline();
        ~ResourcePipeline();

        // Initialize the pipeline with project paths
        bool Initialize(const std::string& projectRootPath);

        // Shutdown and cleanup
        void Shutdown();

        // Import a texture file (PNG, JPG, TGA, etc.) to DDS
        ImportResult ImportTexture(const std::string& sourcePath,
            const TextureImportSettings& settings = {});

        // Import a mesh file (FBX, OBJ, GLTF, etc.)
        ImportResult ImportMesh(const std::string& sourcePath,
            const MeshImportSettings& settings = {});

        // Check if a file needs reimporting (modified since last import)
        bool NeedsReimport(const std::string& sourcePath) const;

        // Force reimport of an asset even if unchanged
        ImportResult ReimportAsset(const std::string& sourcePath);

        // Scan project for new/modified assets and import them
        void ScanAndImportAssets(const std::string& assetsFolder = "Assets");

        // Get the asset database
        AssetDatabase& GetDatabase() { return m_Database; }
        const AssetDatabase& GetDatabase() const { return m_Database; }

        // Get paths
        std::string GetCachePath() const { return m_CachePath; }
        std::string GetProjectPath() const { return m_ProjectPath; }

        // Check if pipeline is initialized
        bool IsInitialized() const { return m_Initialized; }

    private:
        // Internal import functions
        ImportResult ImportTextureInternal(const std::string& sourcePath,
            const std::string& outputPath,
            const TextureImportSettings& settings);

        ImportResult ImportMeshInternal(const std::string& sourcePath,
            const std::string& outputPath,
            const MeshImportSettings& settings);

        // Helper functions
        bool InitializeDirectXTex();
        void CleanupDirectXTex();

        std::string GenerateCachePath(const xresource::full_guid& guid, const std::string& extension) const;
        bool IsTextureFile(const std::string& extension) const;
        bool IsMeshFile(const std::string& extension) const;

        // DirectXTex texture conversion
        bool ConvertTextureToDDS(const std::string& inputPath,
            const std::string& outputPath,
            const TextureImportSettings& settings);

        // Assimp mesh conversion helpers
        bool LoadAssimpScene(const std::string& filePath,
            const aiScene*& outScene,
            Assimp::Importer& importer,
            const MeshImportSettings& settings);

        bool HasSkinning(const aiMesh* mesh) const;

        bool ProcessStaticMesh(const aiMesh* mesh, MeshData& outData);

        bool ProcessSkinnedMeshCombined(const aiScene* scene, SkinnedMeshData& outData);

        // File writing
        bool WriteMeshFile(const std::string& outputPath, const MeshData& meshData);
        bool WriteSkinFile(const std::string& outputPath, const SkinnedMeshData& meshData);

    private:
        bool m_Initialized;
        bool m_COMInitialized;

        std::string m_ProjectPath;      // Root project directory
        std::string m_CachePath;        // Cache directory for processed assets

        AssetDatabase m_Database;       // Asset metadata database
    };

} // namespace Ermine