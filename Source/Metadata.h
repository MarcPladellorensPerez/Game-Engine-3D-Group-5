#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct TextureImportOptions {
    bool flipY = false;
    bool flipX = false;
    int filterMode = 0; // 0 = Linear, 1 = Nearest
    int wrapMode = 0;   // 0 = Repeat, 1 = Clamp
};

struct MeshImportOptions {
    float globalScale = 1.0f;
    bool ignoreCameras = true;
    bool ignoreLights = true;
    int axisConversion = 0; // 0 = Default, 1 = Y-Up, 2 = Z-Up
};

enum class AssetType {
    UNKNOWN = 0,
    TEXTURE,
    MODEL,
    SCENE
};

struct AssetMetadata {
    uint64_t uid = 0;
    AssetType type = AssetType::UNKNOWN;
    std::string assetPath;
    uint64_t lastModified = 0;
    uint64_t optionsHash = 0;
    TextureImportOptions texOptions;
    MeshImportOptions meshOptions;
    std::vector<std::string> generatedFiles; // Paths in Library/

    uint64_t GetOptionsHash() const;
    bool SaveToFile(const std::string& metaFilePath) const;
    bool LoadFromFile(const std::string& metaFilePath);
};

class MetaManager {
public:
    static uint64_t GenerateUID();
    static AssetType DetectAssetType(const std::string& filePath);
    static std::string GetMetaPath(const std::string& assetPath);

    static bool CreateOrLoadMeta(const std::string& assetPath, AssetMetadata& outMeta);
    static bool ScanAssetsAndSyncLibrary(const std::string& assetsDir = "assets", const std::string& libraryDir = "Library");
    static bool RegenerateLibrary(const std::string& assetsDir = "assets", const std::string& libraryDir = "Library");
    static bool DeleteAsset(const std::string& assetPath, const std::string& libraryDir = "Library");
    static bool ImportAssetToLibrary(const std::string& assetPath, AssetMetadata& meta, const std::string& libraryDir = "Library");
};
