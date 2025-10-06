#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <fstream>

// Include the xresource_pipeline header
#include "xresource_pipeline.h"
#include <DirectXTex.h>

#ifdef _MSC_VER
#pragma warning(disable : 4566)
#endif

using namespace DirectX;
// Define a texture resource type GUID (you can generate this or use a fixed one)
constexpr xresource::type_guid TEXTURE_TYPE_GUID("TEXTURE_RESOURCE_TYPE");

class ErmineResourcePipeline {
private:
    std::string configPath;
    std::string projectPath;
    std::string sourceAssetsPath;
    std::string databasePath;
    std::string projectGuid;

    // Track processed resources
    std::vector<xresource::full_guid> processedResources;

    // Track existing resources by source file path and modification time
    struct ResourceEntry {
        xresource::full_guid guid;
        std::string sourcePath;
        std::filesystem::file_time_type lastModified;
        std::string outputPath;
    };
    std::vector<ResourceEntry> existingResources;

    // DirectXTex helper methods
    bool InitializeDirectXTex() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) {
            std::cout << "Failed to initialize COM" << std::endl;
            return false;
        }
        return true;
    }

    void CleanupDirectXTex() {
        CoUninitialize();
    }

    bool ConvertPNGtoDDS(const std::string& inputPath, const std::string& outputPath) {
        using namespace DirectX;

        // Convert paths to wide strings for DirectXTex
        std::wstring wInput(inputPath.begin(), inputPath.end());
        std::wstring wOutput(outputPath.begin(), outputPath.end());

        // Load the PNG
        ScratchImage image;
        HRESULT hr = LoadFromWICFile(wInput.c_str(), WIC_FLAGS_NONE, nullptr, image);
        if (FAILED(hr))
            return false;

        TexMetadata metadata = image.GetMetadata();

        // Convert to engine-supported DXGI format
        // B8G8R8A8_UNORM is safe for your engine
        ScratchImage standardized;
        if (metadata.format != DXGI_FORMAT_B8G8R8A8_UNORM)
        {
            hr = Convert(image.GetImages(), image.GetImageCount(), metadata,
                DXGI_FORMAT_B8G8R8A8_UNORM, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, standardized);
            if (FAILED(hr))
                return false;
            metadata = standardized.GetMetadata();
        }
        else
        {
            standardized = std::move(image);
        }

        // Generate mipmaps
        ScratchImage mipChain;
        hr = GenerateMipMaps(standardized.GetImages(), standardized.GetImageCount(),
            metadata, TEX_FILTER_DEFAULT, 0, mipChain);
        if (FAILED(hr))
            mipChain = std::move(standardized);

        // Optional: Force fully opaque alpha for debug/placeholder textures
        auto imgs = mipChain.GetImages();
        for (size_t i = 0; i < mipChain.GetImageCount(); ++i)
        {
            uint32_t* pixels = reinterpret_cast<uint32_t*>(imgs[i].pixels);
            for (size_t p = 0; p < imgs[i].width * imgs[i].height; ++p)
            {
                uint8_t alpha = pixels[p] >> 24;
                if (alpha == 0)
                    pixels[p] |= 0xFF000000;
            }
        }

        // Save DDS uncompressed
        hr = SaveToDDSFile(mipChain.GetImages(), mipChain.GetImageCount(),
            mipChain.GetMetadata(), DDS_FLAGS_NONE, wOutput.c_str());

        return SUCCEEDED(hr);
    }

public:
    ErmineResourcePipeline(const std::string& configFile) : configPath(configFile) {
        LoadConfig();
        InitializeDirectXTex();
    }

    ~ErmineResourcePipeline() {
        CleanupDirectXTex();
    }

    bool LoadConfig() {
        std::cout << "Loading config from: " << configPath << std::endl;

        std::ifstream file(configPath);
        if (!file.is_open()) {
            std::cout << "ERROR: Cannot open config file!" << std::endl;
            return false;
        }

        // Get the directory where the config file is located (the project root)
        std::filesystem::path configDir = std::filesystem::path(configPath).parent_path().parent_path();
        std::cout << "Project root (config parent): " << std::filesystem::absolute(configDir) << std::endl;

        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#' || line[0] == '[') {
                continue;
            }

            if (line.find("SourceAssetsPath=") == 0) {
                sourceAssetsPath = line.substr(17);
                sourceAssetsPath.erase(0, sourceAssetsPath.find_first_not_of(" \t"));
                // Make it absolute relative to project root
                sourceAssetsPath = std::filesystem::absolute(configDir / sourceAssetsPath).string();
            }
            else if (line.find("DatabasePath=") == 0) {
                databasePath = line.substr(13);
                databasePath.erase(0, databasePath.find_first_not_of(" \t"));
                // Make it absolute relative to project root
                databasePath = std::filesystem::absolute(configDir / databasePath).string();
            }
            else if (line.find("ProjectPath=") == 0) {
                projectPath = line.substr(12);
                projectPath.erase(0, projectPath.find_first_not_of(" \t"));
                // Make it absolute relative to project root
                projectPath = std::filesystem::absolute(configDir / projectPath).string();
            }
            else if (line.find("ProjectGUID=") == 0) {
                projectGuid = line.substr(12);
                projectGuid.erase(0, projectGuid.find_first_not_of(" \t"));
                // Remove curly braces if present
                projectGuid.erase(std::remove(projectGuid.begin(), projectGuid.end(), '{'), projectGuid.end());
                projectGuid.erase(std::remove(projectGuid.begin(), projectGuid.end(), '}'), projectGuid.end());
            }
        }

        std::cout << "=== Resolved Paths ===" << std::endl;
        std::cout << "Project GUID: " << projectGuid << std::endl;
        std::cout << "Source Assets Path: " << sourceAssetsPath << std::endl;
        std::cout << "Database Path: " << databasePath << std::endl;
        std::cout << "Project Path: " << projectPath << std::endl;

        // Validate that source assets path exists
        if (!std::filesystem::exists(sourceAssetsPath)) {
            std::cout << "WARNING: Source assets path does not exist: " << sourceAssetsPath << std::endl;
            std::cout << "Please check your config file paths!" << std::endl;
        }

        return true;
    }

    bool LoadExistingResources() {
        std::cout << "\n=== DEBUG: Loading Existing Resources ===" << std::endl;

        std::string projectFolder = databasePath + "/" + projectGuid;
        std::string resourceDbPath = projectFolder + "/Browser.dbase/resource_database.txt";

        // DEBUG OUTPUT - This is what we need to see!
        std::cout << "Database Path: " << databasePath << std::endl;
        std::cout << "Project GUID: " << projectGuid << std::endl;
        std::cout << "Project Folder: " << projectFolder << std::endl;
        std::cout << "Looking for database at: " << std::filesystem::absolute(resourceDbPath) << std::endl;
        std::cout << "Database exists: " << (std::filesystem::exists(resourceDbPath) ? "YES" : "NO") << std::endl;
        std::cout << "==================================\n" << std::endl;

        if (!std::filesystem::exists(resourceDbPath)) {
            std::cout << "No existing resource database found, starting fresh." << std::endl;
            return true;
        }

        std::ifstream dbFile(resourceDbPath);
        if (!dbFile.is_open()) {
            std::cout << "Warning: Could not open resource database file." << std::endl;
            return false;
        }

        std::string line;
        ResourceEntry currentEntry;
        bool readingEntry = false;

        while (std::getline(dbFile, line)) {
            if (line.find("RESOURCE_START") == 0) {
                readingEntry = true;
                currentEntry = ResourceEntry{};
            }
            else if (line.find("RESOURCE_END") == 0) {
                if (readingEntry) {
                    existingResources.push_back(currentEntry);
                    readingEntry = false;
                }
            }
            else if (readingEntry) {
                if (line.find("InstanceGUID=") == 0) {
                    uint64_t instanceVal = std::stoull(line.substr(13), nullptr, 16);
                    currentEntry.guid.m_Instance.m_Value = instanceVal;
                }
                else if (line.find("TypeGUID=") == 0) {
                    uint64_t typeVal = std::stoull(line.substr(9), nullptr, 16);
                    currentEntry.guid.m_Type.m_Value = typeVal;
                }
                else if (line.find("SourcePath=") == 0) {
                    currentEntry.sourcePath = line.substr(11);
                }
                else if (line.find("LastModified=") == 0) {
                    auto timeVal = std::stoull(line.substr(13));
                    currentEntry.lastModified = std::filesystem::file_time_type(
                        std::chrono::duration<uint64_t>(timeVal));
                }
                else if (line.find("OutputPath=") == 0) {
                    currentEntry.outputPath = line.substr(11);
                }
            }
        }

        std::cout << "Loaded " << existingResources.size() << " existing resources." << std::endl;
        return true;
    }

    void SaveResourceDatabase() {
        std::string projectFolder = databasePath + "/" + projectGuid;
        std::string resourceDbPath = projectFolder + "/Browser.dbase/resource_database.txt";

        std::ofstream dbFile(resourceDbPath);
        if (!dbFile.is_open()) {
            std::cout << "Warning: Could not save resource database." << std::endl;
            return;
        }

        // Save all existing resources (both old and newly processed)
        auto allResources = existingResources;

        for (const auto& guid : processedResources) {
            // Find the corresponding resource entry
            auto it = std::find_if(allResources.begin(), allResources.end(),
                [&guid](const ResourceEntry& entry) {
                    return entry.guid.m_Instance.m_Value == guid.m_Instance.m_Value &&
                        entry.guid.m_Type.m_Value == guid.m_Type.m_Value;
                });

            // If not found in existing, it's a new one we need to add
            // (This case should be handled by the processing logic)
        }

        for (const auto& entry : allResources) {
            dbFile << "RESOURCE_START" << std::endl;
            dbFile << "InstanceGUID=" << std::hex << entry.guid.m_Instance.m_Value << std::endl;
            dbFile << "TypeGUID=" << std::hex << entry.guid.m_Type.m_Value << std::endl;
            dbFile << "SourcePath=" << entry.sourcePath << std::endl;
            dbFile << "LastModified=" << std::dec << entry.lastModified.time_since_epoch().count() << std::endl;
            dbFile << "OutputPath=" << entry.outputPath << std::endl;
            dbFile << "RESOURCE_END" << std::endl;
        }

        std::cout << "Saved resource database with " << allResources.size() << " entries." << std::endl;
    }

    ResourceEntry* FindExistingResource(const std::string& sourcePath) {
        auto it = std::find_if(existingResources.begin(), existingResources.end(),
            [&sourcePath](const ResourceEntry& entry) {
                return entry.sourcePath == sourcePath;
            });

        return (it != existingResources.end()) ? &(*it) : nullptr;
    }

    bool IsFileModified(const std::string& filePath, std::filesystem::file_time_type lastKnown) {
        if (!std::filesystem::exists(filePath)) {
            return false;
        }

        auto currentModTime = std::filesystem::last_write_time(filePath);
        return currentModTime > lastKnown;
    }

    void CreateDatabaseStructure() {
        std::cout << "\n=== Creating Database Structure ===" << std::endl;

        // Create main database directory
        std::filesystem::create_directories(databasePath);

        // Create project GUID folder (following the expected structure)
        std::string projectFolder = databasePath + "/" + projectGuid;
        std::filesystem::create_directories(projectFolder);

        // Create Browser.dbase for logging
        std::filesystem::create_directories(projectFolder + "/Browser.dbase");

        // Create platform-specific directories
        std::filesystem::create_directories(projectFolder + "/Windows.platform/Data");

        // Create Generated.dbase for temporary files
        std::filesystem::create_directories(projectFolder + "/Generated.dbase");

        std::cout << "✓ Created database structure at: " << projectFolder << std::endl;
        std::cout << "✓ Platform: Windows.platform/Data" << std::endl;
        std::cout << "✓ Browser: Browser.dbase" << std::endl;
        std::cout << "✓ Generated: Generated.dbase" << std::endl;
    }

    void ScanSourceAssets() {
        std::cout << "\n=== Scanning Source Assets ===" << std::endl;

        if (!std::filesystem::exists(sourceAssetsPath)) {
            std::cout << "❌ Source assets path does not exist: " << sourceAssetsPath << std::endl;
            return;
        }

        // Load existing resources first
        LoadExistingResources();

        std::vector<std::string> pngFiles;

        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceAssetsPath)) {
                if (entry.is_regular_file()) {
                    std::string extension = entry.path().extension().string();
                    if (extension == ".png" || extension == ".PNG") {
                        pngFiles.push_back(entry.path().string());
                        std::cout << "Found PNG: " << entry.path().filename().string() << std::endl;
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::cout << "Error scanning directory: " << e.what() << std::endl;
            return;
        }

        std::cout << "Total PNG files found: " << pngFiles.size() << std::endl;

        // Process each PNG file through the pipeline
        ProcessAssets(pngFiles);

        // Save the updated resource database
        SaveResourceDatabase();
    }

    void ProcessAssets(const std::vector<std::string>& assetPaths) {
        std::cout << "\n=== Processing Assets ===" << std::endl;

        std::string projectFolder = databasePath + "/" + projectGuid;
        std::string outputDir = projectFolder + "/Windows.platform/Data";

        int skippedCount = 0;
        int processedCount = 0;
        int updatedCount = 0;

        for (const auto& assetPath : assetPaths) {
            std::cout << "Checking: " << assetPath << std::endl;

            // Check if this resource already exists
            ResourceEntry* existing = FindExistingResource(assetPath);

            if (existing != nullptr) {
                // Check if the file has been modified
                if (!IsFileModified(assetPath, existing->lastModified)) {
                    std::cout << "  ⏭ Skipping (unchanged): " << std::filesystem::path(assetPath).filename().string() << std::endl;
                    skippedCount++;
                    continue;
                }
                else {
                    std::cout << "  🔄 File modified, updating..." << std::endl;
                    updatedCount++;
                }
            }
            else {
                std::cout << "  ➕ New asset, processing..." << std::endl;
                processedCount++;
            }

            xresource::full_guid resourceGuid;
            std::string guidStr;

            if (existing != nullptr) {
                // Reuse existing GUID for modified files
                resourceGuid = existing->guid;
                guidStr = std::to_string(resourceGuid.m_Instance.m_Value);
                std::cout << "  Reusing GUID: Instance=" << std::hex << resourceGuid.m_Instance.m_Value
                    << ", Type=" << resourceGuid.m_Type.m_Value << std::dec << std::endl;
            }
            else {
                // Generate new GUID for new files
                auto instanceGuid = xresource::instance_guid::GenerateGUIDCopy();
                resourceGuid = xresource::full_guid{ instanceGuid, TEXTURE_TYPE_GUID };
                guidStr = std::to_string(instanceGuid.m_Value);
                std::cout << "  Generated GUID: Instance=" << std::hex << instanceGuid.m_Value
                    << ", Type=" << TEXTURE_TYPE_GUID.m_Value << std::dec << std::endl;
            }

            // Create resource-specific folder in Generated.dbase using GUID
            std::string generatedPath = projectFolder + "/Generated.dbase/" + guidStr;
            std::filesystem::create_directories(generatedPath);

            // Copy source file to generated folder (for backup/reference)
            std::string tempFile = generatedPath + "/" + std::filesystem::path(assetPath).filename().string();
            try {
                std::filesystem::copy_file(assetPath, tempFile,
                    std::filesystem::copy_options::overwrite_existing);
                std::cout << "  ✓ Staged in Generated.dbase: " << guidStr << std::endl;
            }
            catch (const std::exception& e) {
                std::cout << "  ❌ Failed to stage: " << e.what() << std::endl;
                continue;
            }

            // Convert PNG to DDS using DirectXTex
            std::string finalOutput = outputDir + "/" + guidStr + ".dds";
            if (ConvertPNGtoDDS(assetPath, finalOutput)) {
                std::cout << "  ✓ Successfully converted to DDS: " << guidStr << ".dds" << std::endl;

                // Update or add resource entry
                if (existing != nullptr) {
                    existing->lastModified = std::filesystem::last_write_time(assetPath);
                    existing->outputPath = "Windows.platform/Data/" + guidStr + ".dds";
                }
                else {
                    ResourceEntry newEntry;
                    newEntry.guid = resourceGuid;
                    newEntry.sourcePath = assetPath;
                    newEntry.lastModified = std::filesystem::last_write_time(assetPath);
                    newEntry.outputPath = "Windows.platform/Data/" + guidStr + ".dds";
                    existingResources.push_back(newEntry);
                }

                // Store processed resource info
                processedResources.push_back(resourceGuid);

                // Log the resource info
                LogResourceInfo(resourceGuid, assetPath, finalOutput);
            }
            else {
                std::cout << "  ❌ Failed to convert PNG to DDS" << std::endl;
            }
        }

        std::cout << "\n📊 Processing Summary:" << std::endl;
        std::cout << "  New assets processed: " << processedCount << std::endl;
        std::cout << "  Existing assets updated: " << updatedCount << std::endl;
        std::cout << "  Assets skipped (unchanged): " << skippedCount << std::endl;
        std::cout << "  Total assets in pipeline: " << processedCount + updatedCount + skippedCount << std::endl;
    }

    void LogResourceInfo(const xresource::full_guid& resourceGuid, const std::string& sourcePath, const std::string& outputPath) {
        std::string projectFolder = databasePath + "/" + projectGuid;

        // Log to Browser.dbase with resource type folder structure
        std::string typeGuidStr = std::to_string(resourceGuid.m_Type.m_Value);
        std::string instanceGuidStr = std::to_string(resourceGuid.m_Instance.m_Value);

        std::string browserPath = projectFolder + "/Browser.dbase/" + typeGuidStr;
        std::filesystem::create_directories(browserPath);

        std::string resourceLogPath = browserPath + "/" + instanceGuidStr + "_info.txt";
        std::ofstream logFile(resourceLogPath);
        if (logFile.is_open()) {
            logFile << "Resource Information" << std::endl;
            logFile << "===================" << std::endl;
            logFile << "Instance GUID: 0x" << std::hex << resourceGuid.m_Instance.m_Value << std::dec << std::endl;
            logFile << "Type GUID: 0x" << std::hex << resourceGuid.m_Type.m_Value << std::dec << std::endl;
            logFile << "Source: " << sourcePath << std::endl;
            logFile << "Output: " << outputPath << std::endl;
            logFile << "Platform: WINDOWS" << std::endl;
            logFile << "Resource Type: TEXTURE" << std::endl;
            logFile << "Format: DDS (DirectX Texture)" << std::endl;
            logFile << "Generated: " << std::chrono::system_clock::now().time_since_epoch().count() << std::endl;
            logFile.close();
        }

        // Also log to main resource log
        std::string mainLogPath = projectFolder + "/Browser.dbase/resource_log.txt";
        std::ofstream mainLog(mainLogPath, std::ios::app);
        if (mainLog.is_open()) {
            mainLog << "Resource: 0x" << std::hex << resourceGuid.m_Instance.m_Value
                << " (Type: 0x" << resourceGuid.m_Type.m_Value << ")" << std::dec << std::endl;
            mainLog << "  Source: " << sourcePath << std::endl;
            mainLog << "  Output: " << outputPath << std::endl;
            mainLog << "  Format: DDS" << std::endl;
            mainLog << "  ---" << std::endl;
            mainLog.close();
        }
    }

    void GenerateReport() {
        std::cout << "\n=== Pipeline Report ===" << std::endl;
        std::string projectFolder = databasePath + "/" + projectGuid;
        std::cout << "Project Database: " << projectFolder << std::endl;

        // Count files in Windows platform data
        std::string dataPath = projectFolder + "/Windows.platform/Data";
        if (std::filesystem::exists(dataPath)) {
            int fileCount = 0;
            for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
                if (entry.is_regular_file()) {
                    fileCount++;
                    std::cout << "  Resource: " << entry.path().filename().string()
                        << " (Size: " << std::filesystem::file_size(entry.path()) << " bytes)" << std::endl;
                }
            }
            std::cout << "Total processed assets: " << fileCount << std::endl;
        }

        // Show processed resources with their GUIDs
        std::cout << "\n=== Resource GUIDs ===" << std::endl;
        for (const auto& guid : processedResources) {
            std::cout << "Resource GUID: Instance=0x" << std::hex << guid.m_Instance.m_Value
                << ", Type=0x" << guid.m_Type.m_Value << std::dec << std::endl;
        }

        // Show log file if it exists
        std::string logPath = projectFolder + "/Browser.dbase/resource_log.txt";
        if (std::filesystem::exists(logPath)) {
            std::cout << "\nMain resource log: " << logPath << std::endl;
        }

        // Show Browser.dbase structure
        std::string browserPath = projectFolder + "/Browser.dbase";
        if (std::filesystem::exists(browserPath)) {
            std::cout << "Resource type folders in Browser.dbase:" << std::endl;
            for (const auto& entry : std::filesystem::directory_iterator(browserPath)) {
                if (entry.is_directory()) {
                    std::cout << "  Type: " << entry.path().filename().string() << std::endl;
                }
            }
        }
    }
};

int main() {
    std::cout << "=== Ermine Resource Pipeline Runner ===" << std::endl;

    // Get the directory where the exe is located
    std::filesystem::path exePath = std::filesystem::current_path();
    std::cout << "Working directory: " << exePath << std::endl;

    // Look for config in multiple locations
    std::vector<std::string> configPaths = {
        "Ermine-ResourcePipeline/Config/ResourcePipeline.config",  // From solution root
        "./Config/ResourcePipeline.config",  // From project root (if run from there)
        "../../Config/ResourcePipeline.config",  // From x64/Debug
        "../../../Config/ResourcePipeline.config"  // Alternative
    };

    std::string foundConfigPath;
    for (const auto& path : configPaths) {
        std::filesystem::path absolutePath = std::filesystem::absolute(path);
        std::cout << "Checking: " << absolutePath << std::endl;

        if (std::filesystem::exists(absolutePath)) {
            foundConfigPath = absolutePath.string();
            std::cout << "✓ Found config at: " << foundConfigPath << std::endl;
            break;
        }
    }

    if (foundConfigPath.empty()) {
        std::cout << "ERROR: Could not find ResourcePipeline.config in any expected location!" << std::endl;
        std::cout << "Please ensure Config/ResourcePipeline.config exists." << std::endl;
        return 1;
    }

    ErmineResourcePipeline pipeline(foundConfigPath);

    pipeline.CreateDatabaseStructure();
    pipeline.ScanSourceAssets();
    pipeline.GenerateReport();

    std::cout << "\n🎉 Pipeline execution complete!" << std::endl;

    return 0;
}