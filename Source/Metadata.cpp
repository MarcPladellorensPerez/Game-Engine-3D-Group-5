#include "ResourceManager.h"
#include <functional>
#include "Metadata.h"
#include "Globals.h"
#include "Model.h"
#include "CustomFormat.h"
#include "ComponentTexture.h"
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <IL/il.h>
#include <IL/ilu.h>
#include <stb_image.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <random>
#include <chrono>

namespace fs = std::filesystem;

static bool g_devILInitialized = false;
static void EnsureDevIL()
{
    if (!g_devILInitialized)
    {
        ilInit();
        iluInit();
        g_devILInitialized = true;
    }
}

static uint64_t GetFileTimestamp(const std::string& path)
{
    if (!fs::exists(path)) return 0;
    std::error_code ec;
    auto ftime = fs::last_write_time(path, ec);
    if (ec) return 0;
    auto s_time = std::chrono::file_clock::to_sys(ftime);
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(s_time.time_since_epoch()).count();
    return static_cast<uint64_t>(secs > 0 ? secs : 0);
}

uint64_t AssetMetadata::GetOptionsHash() const
{
    uint64_t h = 0x9e3779b97f4a7c15ULL;
    h ^= (uint64_t)texOptions.flipY + 0x517cc1b727220a95ULL + (h << 6) + (h >> 2);
    h ^= (uint64_t)texOptions.flipX + 0x517cc1b727220a95ULL + (h << 6) + (h >> 2);
    h ^= (uint64_t)texOptions.filterMode + 0x517cc1b727220a95ULL + (h << 6) + (h >> 2);
    h ^= (uint64_t)texOptions.wrapMode + 0x517cc1b727220a95ULL + (h << 6) + (h >> 2);
    union { float f; uint32_t u; } u;
    u.f = meshOptions.globalScale;
    h ^= (uint64_t)u.u + 0x517cc1b727220a95ULL + (h << 6) + (h >> 2);
    h ^= (uint64_t)meshOptions.ignoreCameras + 0x517cc1b727220a95ULL + (h << 6) + (h >> 2);
    h ^= (uint64_t)meshOptions.ignoreLights + 0x517cc1b727220a95ULL + (h << 6) + (h >> 2);
    h ^= (uint64_t)meshOptions.axisConversion + 0x517cc1b727220a95ULL + (h << 6) + (h >> 2);
    return h;
}

bool AssetMetadata::SaveToFile(const std::string& metaFilePath) const
{
    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);

    writer.StartObject();

    writer.Key("uid");
    writer.Uint64(uid);

    writer.Key("type");
    writer.Int(static_cast<int>(type));

    writer.Key("assetPath");
    writer.String(assetPath.c_str());

    writer.Key("lastModified");
    writer.Uint64(lastModified);

    writer.Key("optionsHash");
    writer.Uint64(GetOptionsHash());

    // Texture options
    writer.Key("textureOptions");
    writer.StartObject();
    writer.Key("flipY");
    writer.Bool(texOptions.flipY);
    writer.Key("flipX");
    writer.Bool(texOptions.flipX);
    writer.Key("filterMode");
    writer.Int(texOptions.filterMode);
    writer.Key("wrapMode");
    writer.Int(texOptions.wrapMode);
    writer.EndObject();

    // Mesh options
    writer.Key("meshOptions");
    writer.StartObject();
    writer.Key("globalScale");
    writer.Double(static_cast<double>(meshOptions.globalScale));
    writer.Key("ignoreCameras");
    writer.Bool(meshOptions.ignoreCameras);
    writer.Key("ignoreLights");
    writer.Bool(meshOptions.ignoreLights);
    writer.Key("axisConversion");
    writer.Int(meshOptions.axisConversion);
    writer.EndObject();

    // Generated Library files
    writer.Key("generatedFiles");
    writer.StartArray();
    for (const auto& file : generatedFiles)
    {
        writer.String(file.c_str());
    }
    writer.EndArray();

    writer.EndObject();

    std::ofstream out(metaFilePath);
    if (!out.is_open()) return false;
    out << sb.GetString();
    return true;
}

bool AssetMetadata::LoadFromFile(const std::string& metaFilePath)
{
    std::ifstream in(metaFilePath);
    if (!in.is_open()) return false;

    std::stringstream ss;
    ss << in.rdbuf();
    std::string jsonStr = ss.str();

    rapidjson::Document doc;
    if (doc.Parse(jsonStr.c_str()).HasParseError()) return false;

    if (doc.HasMember("uid") && doc["uid"].IsUint64())
        uid = doc["uid"].GetUint64();

    if (doc.HasMember("type") && doc["type"].IsInt())
        type = static_cast<AssetType>(doc["type"].GetInt());

    if (doc.HasMember("assetPath") && doc["assetPath"].IsString())
        assetPath = doc["assetPath"].GetString();

    if (doc.HasMember("lastModified") && doc["lastModified"].IsUint64())
        lastModified = doc["lastModified"].GetUint64();

    if (doc.HasMember("optionsHash") && doc["optionsHash"].IsUint64())
        optionsHash = doc["optionsHash"].GetUint64();

    if (doc.HasMember("textureOptions") && doc["textureOptions"].IsObject())
    {
        const auto& to = doc["textureOptions"];
        if (to.HasMember("flipY") && to["flipY"].IsBool()) texOptions.flipY = to["flipY"].GetBool();
        if (to.HasMember("flipX") && to["flipX"].IsBool()) texOptions.flipX = to["flipX"].GetBool();
        if (to.HasMember("filterMode") && to["filterMode"].IsInt()) texOptions.filterMode = to["filterMode"].GetInt();
        if (to.HasMember("wrapMode") && to["wrapMode"].IsInt()) texOptions.wrapMode = to["wrapMode"].GetInt();
    }

    if (doc.HasMember("meshOptions") && doc["meshOptions"].IsObject())
    {
        const auto& mo = doc["meshOptions"];
        if (mo.HasMember("globalScale") && mo["globalScale"].IsNumber()) meshOptions.globalScale = static_cast<float>(mo["globalScale"].GetDouble());
        if (mo.HasMember("ignoreCameras") && mo["ignoreCameras"].IsBool()) meshOptions.ignoreCameras = mo["ignoreCameras"].GetBool();
        if (mo.HasMember("ignoreLights") && mo["ignoreLights"].IsBool()) meshOptions.ignoreLights = mo["ignoreLights"].GetBool();
        if (mo.HasMember("axisConversion") && mo["axisConversion"].IsInt()) meshOptions.axisConversion = mo["axisConversion"].GetInt();
    }

    generatedFiles.clear();
    if (doc.HasMember("generatedFiles") && doc["generatedFiles"].IsArray())
    {
        const auto& arr = doc["generatedFiles"];
        for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
        {
            if (arr[i].IsString())
            {
                generatedFiles.push_back(fs::path(arr[i].GetString()).generic_string());
            }
        }
    }

    return true;
}

uint64_t MetaManager::GenerateUID()
{
    static std::mt19937_64 rng(std::random_device{}());
    return rng();
}

AssetType MetaManager::DetectAssetType(const std::string& filePath)
{
    std::string ext = fs::path(filePath).extension().string();
    for (char& c : ext) c = static_cast<char>(tolower(c));

    if (ext == ".fbx" || ext == ".obj" || ext == ".dae" || ext == ".gltf" || ext == ".blend")
        return AssetType::MODEL;
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".dds" || ext == ".bmp")
        return AssetType::TEXTURE;
    if (ext == ".scene" || ext == ".json")
        return AssetType::SCENE;

    return AssetType::UNKNOWN;
}

std::string MetaManager::GetMetaPath(const std::string& assetPath)
{
    return assetPath + ".meta";
}

bool MetaManager::CreateOrLoadMeta(const std::string& assetPath, AssetMetadata& outMeta)
{
    std::string metaPath = GetMetaPath(assetPath);
    if (fs::exists(metaPath))
    {
        if (outMeta.LoadFromFile(metaPath))
        {
            return true;
        }
    }

    outMeta.uid = GenerateUID();
    outMeta.type = DetectAssetType(assetPath);
    outMeta.assetPath = fs::path(assetPath).generic_string();
    outMeta.lastModified = 0;
    outMeta.generatedFiles.clear();

    return outMeta.SaveToFile(metaPath);
}

bool MetaManager::ImportAssetToLibrary(const std::string& assetPath, AssetMetadata& meta, const std::string& libraryDir)
{
    fs::create_directories(fs::path(libraryDir) / "Meshes");
    fs::create_directories(fs::path(libraryDir) / "Textures");
    fs::create_directories(fs::path(libraryDir) / "Models");

    meta.generatedFiles.clear();

    if (meta.type == AssetType::MODEL)
    {
        std::string outModelFile;
        if (Model::ExportToCustomFormat(assetPath, libraryDir, outModelFile, meta.meshOptions))
        {
            meta.generatedFiles.push_back(fs::path(outModelFile).generic_string());

            // Collect all generated mesh and texture files directly from the exported model
            CustomModelNode rootNode;
            if (CustomFormat::LoadModel(outModelFile, rootNode))
            {
                std::function<void(const CustomModelNode&)> collect = [&](const CustomModelNode& node) {
                    if (!node.meshFile.empty())
                    {
                        std::string p = fs::path(node.meshFile).generic_string();
                        if (std::find(meta.generatedFiles.begin(), meta.generatedFiles.end(), p) == meta.generatedFiles.end())
                        {
                            meta.generatedFiles.push_back(p);
                        }
                    }
                    if (!node.textureFile.empty())
                    {
                        std::string p = fs::path(node.textureFile).generic_string();
                        if (std::find(meta.generatedFiles.begin(), meta.generatedFiles.end(), p) == meta.generatedFiles.end())
                        {
                            meta.generatedFiles.push_back(p);
                        }
                    }
                    for (const auto& child : node.children)
                    {
                        collect(child);
                    }
                };
                collect(rootNode);
            }
            LOG("[MetaManager] Imported model '%s' to '%s'", assetPath.c_str(), outModelFile.c_str());
        }
        else
        {
            LOG("[MetaManager] Failed to import model '%s'", assetPath.c_str());
            return false;
        }
    }
    else if (meta.type == AssetType::TEXTURE)
    {
        std::string stem = fs::path(assetPath).stem().generic_string();
        std::string outTexFile = (fs::path(libraryDir) / "Textures" / (stem + ".ctex")).generic_string();

        EnsureDevIL();
        ILuint imgId = 0;
        ilGenImages(1, &imgId);
        ilBindImage(imgId);

        bool saved = false;
        if (ilLoadImage(assetPath.c_str()))
        {
            ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
            int w = ilGetInteger(IL_IMAGE_WIDTH);
            int h = ilGetInteger(IL_IMAGE_HEIGHT);
            int comp = 4;
            if (meta.texOptions.flipY) iluFlipImage();
            if (meta.texOptions.flipX) iluMirror();

            unsigned char* data = ilGetData();
            if (data && CustomFormat::SaveTexture(outTexFile, w, h, comp, data))
            {
                meta.generatedFiles.push_back(outTexFile);
                saved = true;
                LOG("[MetaManager] Imported texture '%s' to '%s' (DevIL)", assetPath.c_str(), outTexFile.c_str());
            }
            ilDeleteImages(1, &imgId);
        }
        else
        {
            ilDeleteImages(1, &imgId);
            // Fallback to stb_image
            stbi_set_flip_vertically_on_load(meta.texOptions.flipY);
            int w = 0, h = 0, comp = 0;
            unsigned char* stbData = stbi_load(assetPath.c_str(), &w, &h, &comp, 4);
            if (stbData)
            {
                if (meta.texOptions.flipX)
                {
                    for (int y = 0; y < h; ++y)
                    {
                        for (int x = 0; x < w / 2; ++x)
                        {
                            for (int c = 0; c < 4; ++c)
                            {
                                std::swap(stbData[(y * w + x) * 4 + c], stbData[(y * w + (w - 1 - x)) * 4 + c]);
                            }
                        }
                    }
                }
                if (CustomFormat::SaveTexture(outTexFile, w, h, 4, stbData))
                {
                    meta.generatedFiles.push_back(outTexFile);
                    saved = true;
                    LOG("[MetaManager] Imported texture '%s' to '%s' (stb)", assetPath.c_str(), outTexFile.c_str());
                }
                stbi_image_free(stbData);
            }
        }

        if (!saved)
        {
            LOG("[MetaManager] Failed to import texture '%s'", assetPath.c_str());
            return false;
        }
    }

    // Update last modified timestamp
    meta.lastModified = GetFileTimestamp(assetPath);
    meta.optionsHash = meta.GetOptionsHash();
    meta.SaveToFile(GetMetaPath(assetPath));
    return true;
}

bool MetaManager::ScanAssetsAndSyncLibrary(const std::string& assetsDir, const std::string& libraryDir)
{
    if (!fs::exists(assetsDir))
    {
        fs::create_directories(assetsDir);
        return true;
    }

    LOG("[MetaManager] Scanning assets directory: %s", assetsDir.c_str());

    // 1. Remove orphaned .meta files whose source asset no longer exists
    for (const auto& entry : fs::recursive_directory_iterator(assetsDir))
    {
        if (entry.is_regular_file())
        {
            std::string pathStr = entry.path().generic_string();
            if (pathStr.ends_with(".meta"))
            {
                std::string sourceAsset = pathStr.substr(0, pathStr.length() - 5);
                if (!fs::exists(sourceAsset))
                {
                    LOG("[MetaManager] Removing orphaned meta file: %s", pathStr.c_str());
                    std::error_code ec;
                    fs::remove(pathStr, ec);
                }
            }
        }
    }

    struct ScannedAsset
    {
        std::string pathStr;
        AssetType type;
        AssetMetadata meta;
    };
    std::vector<ScannedAsset> scannedAssets;

    // 2. Scan regular assets and re-import if needed
    for (const auto& entry : fs::recursive_directory_iterator(assetsDir))
    {
        if (entry.is_regular_file())
        {
            std::string pathStr = entry.path().generic_string();
            if (pathStr.ends_with(".meta")) continue;

            AssetType type = DetectAssetType(pathStr);
            if (type == AssetType::UNKNOWN) continue;

            AssetMetadata meta;
            CreateOrLoadMeta(pathStr, meta);

            uint64_t currentMod = GetFileTimestamp(pathStr);
            uint64_t currentOptHash = meta.GetOptionsHash();
            bool optionsChanged = (meta.optionsHash == 0 || meta.optionsHash != currentOptHash);

            // Reimport if never imported, legacy overflowed timestamp (>10000000000), modified asset, options changed, or missing generated file
            bool needsReimport = (meta.lastModified == 0 || meta.lastModified > 10000000000ULL ||
                                  currentMod != meta.lastModified || optionsChanged || meta.generatedFiles.empty());
            if (!needsReimport)
            {
                for (const auto& genFile : meta.generatedFiles)
                {
                    if (!fs::exists(genFile))
                    {
                        needsReimport = true;
                        break;
                    }
                }
            }

            if (needsReimport)
            {
                LOG("[MetaManager] Re-importing asset: %s", pathStr.c_str());
                ImportAssetToLibrary(pathStr, meta, libraryDir);
            }

            scannedAssets.push_back({pathStr, type, meta});
        }
    }

    // 3. Register standalone textures first so models sharing the same .ctex do not create duplicate entries
    for (const auto& item : scannedAssets)
    {
        if (item.type == AssetType::TEXTURE)
        {
            std::string stem = fs::path(item.pathStr).stem().generic_string();
            std::string libTex = (fs::path(libraryDir) / "Textures" / (stem + ".ctex")).generic_string();
            ResourceManager::Instance().RegisterResource(item.meta.uid, ResourceType::TEXTURE, stem, item.pathStr, libTex);
        }
    }

    for (const auto& item : scannedAssets)
    {
        if (item.type == AssetType::MODEL)
        {
            for (const auto& genFile : item.meta.generatedFiles)
            {
                std::string genStem = fs::path(genFile).stem().generic_string();
                if (genFile.ends_with(".cmesh"))
                {
                    uint64_t subUid = item.meta.uid ^ (std::hash<std::string>{}(genFile) + 0x9e3779b97f4a7c15ULL);
                    ResourceManager::Instance().RegisterResource(subUid, ResourceType::MESH, genStem, item.pathStr, genFile);
                }
                else if (genFile.ends_with(".ctex"))
                {
                    uint64_t subUid = item.meta.uid ^ (std::hash<std::string>{}(genFile) + 0x517cc1b727220a95ULL);
                    ResourceManager::Instance().RegisterResource(subUid, ResourceType::TEXTURE, genStem, item.pathStr, genFile);
                }
            }
        }
    }

    return true;
}

bool MetaManager::RegenerateLibrary(const std::string& assetsDir, const std::string& libraryDir)
{
    LOG("[MetaManager] Regenerating entire Library from Assets and .meta files...");

    if (fs::exists(libraryDir))
    {
        std::error_code ec;
        fs::remove_all(libraryDir, ec);
    }

    fs::create_directories(fs::path(libraryDir) / "Meshes");
    fs::create_directories(fs::path(libraryDir) / "Textures");
    fs::create_directories(fs::path(libraryDir) / "Models");

    // Clear generatedFiles and reset timestamp on .meta files so all assets are cleanly re-imported
    for (const auto& entry : fs::recursive_directory_iterator(assetsDir))
    {
        if (entry.is_regular_file())
        {
            std::string pathStr = entry.path().generic_string();
            if (pathStr.ends_with(".meta"))
            {
                AssetMetadata meta;
                if (meta.LoadFromFile(pathStr))
                {
                    meta.generatedFiles.clear();
                    meta.lastModified = 0;
                    meta.SaveToFile(pathStr);
                }
            }
        }
    }

    // Scan, re-import all assets and register them into ResourceManager
    ScanAssetsAndSyncLibrary(assetsDir, libraryDir);

    LOG("[MetaManager] Library regenerated successfully.");
    return true;
}

bool MetaManager::DeleteAsset(const std::string& assetPath, const std::string& libraryDir)
{
    std::string metaPath = GetMetaPath(assetPath);
    AssetMetadata meta;
    if (meta.LoadFromFile(metaPath))
    {
        for (const auto& genFile : meta.generatedFiles)
        {
            if (fs::exists(genFile))
            {
                std::error_code ec;
                fs::remove(genFile, ec);
                LOG("[MetaManager] Deleted Library file: %s", genFile.c_str());
            }
        }
    }

    ResourceManager::Instance().UnregisterResourceByAssetPath(assetPath);
    if (meta.uid != 0)
    {
        ResourceManager::Instance().UnregisterResource(meta.uid);
    }

    if (fs::exists(assetPath))
    {
        std::error_code ec;
        fs::remove(assetPath, ec);
        LOG("[MetaManager] Deleted Asset file: %s", assetPath.c_str());
    }

    if (fs::exists(metaPath))
    {
        std::error_code ec;
        fs::remove(metaPath, ec);
        LOG("[MetaManager] Deleted Meta file: %s", metaPath.c_str());
    }

    return true;
}
