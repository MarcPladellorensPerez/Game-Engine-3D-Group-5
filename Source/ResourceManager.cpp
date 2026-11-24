#include <stb_image.h>
#include "Metadata.h"
#include "ResourceManager.h"
#include "CustomFormat.h"
#include "Globals.h"
#include <glad/glad.h>
#include <filesystem>
#include <IL/il.h>
#include <IL/ilu.h>

namespace fs = std::filesystem;

static std::string NormalizePath(const std::string& p)
{
    if (p.empty()) return "";
    std::string s = fs::path(p).lexically_normal().generic_string();
    return s;
}

bool ResourceMesh::Load()
{
    if (isLoaded) return true;

    if (!CustomFormat::LoadMesh(libraryPath, vertices, indices, aabbMin, aabbMax))
    {
        LOG("[ResourceManager] Failed to load binary mesh from: %s", libraryPath.c_str());
        return false;
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    if (!indices.empty())
    {
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    }

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    glBindVertexArray(0);

    isLoaded = true;
    LOG("[ResourceManager] Loaded ResourceMesh '%s' into GPU memory (VAO: %u, Vertices: %zu)",
        name.c_str(), VAO, vertices.size());
    return true;
}

void ResourceMesh::Unload()
{
    if (isLoaded)
    {
        if (VAO != 0) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
        if (VBO != 0) { glDeleteBuffers(1, &VBO); VBO = 0; }
        if (EBO != 0) { glDeleteBuffers(1, &EBO); EBO = 0; }
        vertices.clear();
        indices.clear();
        isLoaded = false;
        LOG("[ResourceManager] Unloaded ResourceMesh '%s' from GPU memory", name.c_str());
    }
}

static bool g_rmDevILInit = false;
static void EnsureRMDevIL()
{
    if (!g_rmDevILInit)
    {
        ilInit();
        iluInit();
        g_rmDevILInit = true;
    }
}

bool ResourceTexture::Load()
{
    if (isLoaded) return true;

    // If libraryPath is a .ctex custom binary texture and exists, load it
    if (libraryPath.ends_with(".ctex") && fs::exists(libraryPath))
    {
        std::vector<unsigned char> data;
        if (CustomFormat::LoadTexture(libraryPath, width, height, channels, data) && !data.empty())
        {
            GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data.data());
            glGenerateMipmap(GL_TEXTURE_2D);

            GLint wrap = GL_REPEAT;
            GLint minFilter = GL_LINEAR_MIPMAP_LINEAR;
            GLint magFilter = GL_LINEAR;

            if (!assetsPath.empty())
            {
                std::string metaPath = MetaManager::GetMetaPath(assetsPath);
                AssetMetadata meta;
                if (meta.LoadFromFile(metaPath))
                {
                    if (meta.texOptions.wrapMode == 1) wrap = GL_CLAMP_TO_EDGE;
                    if (meta.texOptions.filterMode == 1)
                    {
                        minFilter = GL_NEAREST_MIPMAP_NEAREST;
                        magFilter = GL_NEAREST;
                    }
                }
            }

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
            glBindTexture(GL_TEXTURE_2D, 0);

            isLoaded = true;
            LOG("[ResourceManager] Loaded ResourceTexture '%s' into GPU memory (ID: %u, %dx%d)",
                name.c_str(), textureID, width, height);
            return true;
        }
    }

    // Fallback: try loading with DevIL / stb_image directly
    std::string fallbackPath = (!assetsPath.empty() && fs::exists(assetsPath)) ? assetsPath : libraryPath;
    if (!fallbackPath.empty() && fs::exists(fallbackPath))
    {
        EnsureRMDevIL();
        ILuint imgId = 0;
        ilGenImages(1, &imgId);
        ilBindImage(imgId);
        if (ilLoadImage(fallbackPath.c_str()))
        {
            ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
            width = ilGetInteger(IL_IMAGE_WIDTH);
            height = ilGetInteger(IL_IMAGE_HEIGHT);
            channels = 4;
            unsigned char* pixData = ilGetData();

            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixData);
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);

            ilDeleteImages(1, &imgId);
            isLoaded = true;
            LOG("[ResourceManager] Loaded fallback texture '%s' via DevIL (ID: %u, %dx%d)",
                fallbackPath.c_str(), textureID, width, height);
            return true;
        }
        else
        {
            ilDeleteImages(1, &imgId);
            int w = 0, h = 0, comp = 0;
            unsigned char* stbData = stbi_load(fallbackPath.c_str(), &w, &h, &comp, 4);
            if (stbData)
            {
                width = w;
                height = h;
                channels = 4;
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, stbData);
                glGenerateMipmap(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glBindTexture(GL_TEXTURE_2D, 0);

                stbi_image_free(stbData);
                isLoaded = true;
                LOG("[ResourceManager] Loaded fallback texture '%s' via stb (ID: %u, %dx%d)",
                    fallbackPath.c_str(), textureID, width, height);
                return true;
            }
        }
    }

    LOG("[ResourceManager] Failed to load ResourceTexture: %s", libraryPath.c_str());
    return false;
}

void ResourceTexture::Unload()
{
    if (isLoaded)
    {
        if (textureID != 0)
        {
            glDeleteTextures(1, &textureID);
            textureID = 0;
        }
        isLoaded = false;
        LOG("[ResourceManager] Unloaded ResourceTexture '%s' from GPU memory", name.c_str());
    }
}

ResourceManager& ResourceManager::Instance()
{
    static ResourceManager instance;
    return instance;
}

void ResourceManager::Init()
{
    LOG("[ResourceManager] Initialized Resource Manager.");
}

void ResourceManager::CleanUp()
{
    for (auto& pair : resources)
    {
        if (pair.second)
        {
            if (pair.second->type == ResourceType::MESH)
            {
                std::static_pointer_cast<ResourceMesh>(pair.second)->Unload();
            }
            else if (pair.second->type == ResourceType::TEXTURE)
            {
                std::static_pointer_cast<ResourceTexture>(pair.second)->Unload();
            }
        }
    }
    resources.clear();
    assetPathToUID.clear();
    libraryPathToUID.clear();
}

void ResourceManager::RegisterResource(uint64_t uid, ResourceType type, const std::string& name, const std::string& assetPath, const std::string& libraryPath)
{
    if (resources.find(uid) != resources.end()) return;

    if (type == ResourceType::MESH)
    {
        auto res = std::make_shared<ResourceMesh>();
        res->uid = uid;
        res->type = type;
        res->name = name;
        res->assetsPath = assetPath;
        res->libraryPath = libraryPath;
        resources[uid] = res;
    }
    else if (type == ResourceType::TEXTURE)
    {
        auto res = std::make_shared<ResourceTexture>();
        res->uid = uid;
        res->type = type;
        res->name = name;
        res->assetsPath = assetPath;
        res->libraryPath = libraryPath;
        resources[uid] = res;
    }

    if (!assetPath.empty()) assetPathToUID[NormalizePath(assetPath)] = uid;
    if (!libraryPath.empty()) libraryPathToUID[NormalizePath(libraryPath)] = uid;
}

uint64_t ResourceManager::FindUIDByAssetPath(const std::string& path) const
{
    std::string norm = NormalizePath(path);
    auto it = assetPathToUID.find(norm);
    if (it != assetPathToUID.end()) return it->second;
    // Fallback: match by filename
    std::string fn = fs::path(norm).filename().string();
    for (const auto& pair : assetPathToUID)
    {
        if (fs::path(pair.first).filename().string() == fn) return pair.second;
    }
    return 0;
}

uint64_t ResourceManager::FindUIDByLibraryPath(const std::string& path) const
{
    std::string norm = NormalizePath(path);
    auto it = libraryPathToUID.find(norm);
    if (it != libraryPathToUID.end()) return it->second;
    // Fallback: match by filename
    std::string fn = fs::path(norm).filename().string();
    for (const auto& pair : libraryPathToUID)
    {
        if (fs::path(pair.first).filename().string() == fn) return pair.second;
    }
    return 0;
}

ResourceMesh* ResourceManager::RequestMesh(uint64_t uid)
{
    auto it = resources.find(uid);
    if (it != resources.end() && it->second->type == ResourceType::MESH)
    {
        auto meshRes = std::static_pointer_cast<ResourceMesh>(it->second);
        if (!meshRes->isLoaded)
        {
            if (!meshRes->Load())
            {
                LOG("[ResourceManager] Failed to load ResourceMesh '%s'", meshRes->name.c_str());
                return nullptr;
            }
        }
        meshRes->referenceCount++;
        LOG("[ResourceManager] RequestMesh: '%s' ref count is now %u", meshRes->name.c_str(), meshRes->referenceCount);
        return meshRes.get();
    }
    return nullptr;
}

ResourceMesh* ResourceManager::RequestMeshByPath(const std::string& libraryOrAssetPath)
{
    uint64_t uid = FindUIDByLibraryPath(libraryOrAssetPath);
    if (uid == 0) uid = FindUIDByAssetPath(libraryOrAssetPath);

    if (uid == 0)
    {
        // Auto-register on the fly
        static uint64_t autoUid = 100000;
        uid = ++autoUid;
        std::string stem = fs::path(libraryOrAssetPath).stem().string();
        RegisterResource(uid, ResourceType::MESH, stem, "", libraryOrAssetPath);
    }
    return RequestMesh(uid);
}

void ResourceManager::ReleaseMesh(uint64_t uid)
{
    auto it = resources.find(uid);
    if (it != resources.end() && it->second->type == ResourceType::MESH)
    {
        auto meshRes = std::static_pointer_cast<ResourceMesh>(it->second);
        if (meshRes->referenceCount > 0)
        {
            meshRes->referenceCount--;
            LOG("[ResourceManager] ReleaseMesh: '%s' ref count is now %u", meshRes->name.c_str(), meshRes->referenceCount);
            if (meshRes->referenceCount == 0)
            {
                meshRes->Unload();
            }
        }
    }
}

ResourceTexture* ResourceManager::RequestTexture(uint64_t uid)
{
    auto it = resources.find(uid);
    if (it != resources.end() && it->second->type == ResourceType::TEXTURE)
    {
        auto texRes = std::static_pointer_cast<ResourceTexture>(it->second);
        if (!texRes->isLoaded)
        {
            if (!texRes->Load())
            {
                LOG("[ResourceManager] Failed to load ResourceTexture '%s'", texRes->name.c_str());
                return nullptr;
            }
        }
        texRes->referenceCount++;
        LOG("[ResourceManager] RequestTexture: '%s' ref count is now %u", texRes->name.c_str(), texRes->referenceCount);
        return texRes.get();
    }
    return nullptr;
}

ResourceTexture* ResourceManager::RequestTextureByPath(const std::string& libraryOrAssetPath)
{
    uint64_t uid = FindUIDByLibraryPath(libraryOrAssetPath);
    if (uid == 0) uid = FindUIDByAssetPath(libraryOrAssetPath);

    if (uid == 0)
    {
        static uint64_t autoUid = 200000;
        uid = ++autoUid;
        std::string stem = fs::path(libraryOrAssetPath).stem().generic_string();
        std::string libTex = "Library/Textures/" + stem + ".ctex";
        std::string assTex = libraryOrAssetPath.ends_with(".ctex") ? "" : libraryOrAssetPath;
        if (libraryOrAssetPath.ends_with(".ctex")) libTex = libraryOrAssetPath;

        RegisterResource(uid, ResourceType::TEXTURE, stem, assTex, libTex);
    }
    return RequestTexture(uid);
}

void ResourceManager::ReleaseTexture(uint64_t uid)
{
    auto it = resources.find(uid);
    if (it != resources.end() && it->second->type == ResourceType::TEXTURE)
    {
        auto texRes = std::static_pointer_cast<ResourceTexture>(it->second);
        if (texRes->referenceCount > 0)
        {
            texRes->referenceCount--;
            LOG("[ResourceManager] ReleaseTexture: '%s' ref count is now %u", texRes->name.c_str(), texRes->referenceCount);
            if (texRes->referenceCount == 0)
            {
                texRes->Unload();
            }
        }
    }
}


void ResourceManager::UnregisterResource(uint64_t uid)
{
    auto it = resources.find(uid);
    if (it != resources.end())
    {
        if (!it->second->assetsPath.empty())
        {
            assetPathToUID.erase(NormalizePath(it->second->assetsPath));
        }
        if (!it->second->libraryPath.empty())
        {
            libraryPathToUID.erase(NormalizePath(it->second->libraryPath));
        }
        resources.erase(it);
    }
}

void ResourceManager::UnregisterResourceByAssetPath(const std::string& assetPath)
{
    std::string normPath = NormalizePath(assetPath);
    std::vector<uint64_t> toErase;
    for (const auto& [uid, res] : resources)
    {
        if (res && NormalizePath(res->assetsPath) == normPath)
        {
            toErase.push_back(uid);
        }
    }
    for (uint64_t uid : toErase)
    {
        UnregisterResource(uid);
    }
}
