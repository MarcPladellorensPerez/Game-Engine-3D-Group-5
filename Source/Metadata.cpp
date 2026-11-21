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
#include <fstream>
#include <sstream>
#include <filesystem>
#include <random>
#include <chrono>

namespace fs = std::filesystem;

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
                generatedFiles.push_back(arr[i].GetString());
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
    outMeta.assetPath = assetPath;
    outMeta.lastModified = 0;
    outMeta.generatedFiles.clear();

    return outMeta.SaveToFile(metaPath);
}

bool MetaManager::ImportAssetToLibrary(const std::string& assetPath, AssetMetadata& meta, const std::string& libraryDir)
{
    fs::create_directories(libraryDir + "/Meshes");
    fs::create_directories(libraryDir + "/Textures");
    fs::create_directories(libraryDir + "/Models");

    meta.generatedFiles.clear();

    if (meta.type == AssetType::MODEL)
    {
        std::string outModelFile;
        if (Model::ExportToCustomFormat(assetPath, libraryDir, outModelFile))
        {
            meta.generatedFiles.push_back(outModelFile);

            // Collect all generated mesh and texture files directly from the exported model
            CustomModelNode rootNode;
            if (CustomFormat::LoadModel(outModelFile, rootNode))
            {
                std::function<void(const CustomModelNode&)> collect = [&](const CustomModelNode& node) {
                    if (!node.meshFile.empty())
                    {
                        meta.generatedFiles.push_back(node.meshFile);
                    }
                    if (!node.textureFile.empty())
                    {
                        meta.generatedFiles.push_back(node.textureFile);
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
        std::string stem = fs::path(assetPath).stem().string();
        std::string outTexFile = libraryDir + "/Textures/" + stem + ".ctex";

        ComponentTexture tempTex(nullptr);
        if (tempTex.LoadTexture(assetPath))
        {
            if (tempTex.SaveToCustomFile(outTexFile))
            {
                meta.generatedFiles.push_back(outTexFile);
                LOG("[MetaManager] Imported texture '%s' to '%s'", assetPath.c_str(), outTexFile.c_str());
            }
            else
            {
                LOG("[MetaManager] Failed to save custom texture '%s'", outTexFile.c_str());
                return false;
            }
        }
        else
        {
            LOG("[MetaManager] Failed to load source texture '%s'", assetPath.c_str());
            return false;
        }
    }

    // Update last modified timestamp
    if (fs::exists(assetPath))
    {
        auto ftime = fs::last_write_time(assetPath);
        meta.lastModified = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()).count());
    }

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

            auto ftime = fs::last_write_time(entry.path());
            uint64_t currentMod = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()).count());

            bool needsReimport = (meta.lastModified == 0 || currentMod > meta.lastModified || meta.generatedFiles.empty());
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

            // Register into ResourceManager so resources are visible and indexed
            std::string stem = fs::path(pathStr).stem().string();
            if (type == AssetType::TEXTURE)
            {
                std::string libTex = libraryDir + "/Textures/" + stem + ".ctex";
                ResourceManager::Instance().RegisterResource(meta.uid, ResourceType::TEXTURE, stem, pathStr, libTex);
            }
            else if (type == AssetType::MODEL)
            {
                for (const auto& genFile : meta.generatedFiles)
                {
                    std::string genStem = fs::path(genFile).stem().string();
                    if (genFile.ends_with(".cmesh"))
                    {
                        uint64_t subUid = meta.uid ^ (std::hash<std::string>{}(genFile) + 0x9e3779b97f4a7c15ULL);
                        ResourceManager::Instance().RegisterResource(subUid, ResourceType::MESH, genStem, pathStr, genFile);
                    }
                    else if (genFile.ends_with(".ctex"))
                    {
                        uint64_t subUid = meta.uid ^ (std::hash<std::string>{}(genFile) + 0x517cc1b727220a95ULL);
                        ResourceManager::Instance().RegisterResource(subUid, ResourceType::TEXTURE, genStem, pathStr, genFile);
                    }
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

    fs::create_directories(libraryDir + "/Meshes");
    fs::create_directories(libraryDir + "/Textures");
    fs::create_directories(libraryDir + "/Models");

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
            ImportAssetToLibrary(pathStr, meta, libraryDir);
        }
    }

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
