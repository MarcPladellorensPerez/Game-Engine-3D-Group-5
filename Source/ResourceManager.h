#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"

enum class ResourceType {
    UNKNOWN = 0,
    MESH,
    TEXTURE,
    MODEL
};

struct Resource {
    uint64_t uid = 0;
    ResourceType type = ResourceType::UNKNOWN;
    std::string name;
    std::string assetsPath;
    std::string libraryPath;
    uint32_t referenceCount = 0;
    bool isLoaded = false;

    virtual ~Resource() = default;
};

struct ResourceMesh : public Resource {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    glm::vec3 aabbMin{0.0f};
    glm::vec3 aabbMax{0.0f};
    unsigned int VAO = 0, VBO = 0, EBO = 0;

    bool Load();
    void Unload();
    ~ResourceMesh() override { Unload(); }
};

struct ResourceTexture : public Resource {
    unsigned int textureID = 0;
    int width = 0;
    int height = 0;
    int channels = 0;

    bool Load();
    void Unload();
    ~ResourceTexture() override { Unload(); }
};

class ResourceManager {
public:
    static ResourceManager& Instance();

    void Init();
    void CleanUp();

    ResourceMesh* RequestMesh(uint64_t uid);
    ResourceMesh* RequestMeshByPath(const std::string& libraryOrAssetPath);
    void ReleaseMesh(uint64_t uid);

    ResourceTexture* RequestTexture(uint64_t uid);
    ResourceTexture* RequestTextureByPath(const std::string& libraryOrAssetPath);
    void ReleaseTexture(uint64_t uid);

    const std::unordered_map<uint64_t, std::shared_ptr<Resource>>& GetAllResources() const { return resources; }

    uint64_t FindUIDByAssetPath(const std::string& path) const;
    uint64_t FindUIDByLibraryPath(const std::string& path) const;

    void RegisterResource(uint64_t uid, ResourceType type, const std::string& name, const std::string& assetPath, const std::string& libraryPath);

private:
    ResourceManager() = default;
    ~ResourceManager() = default;

    std::unordered_map<uint64_t, std::shared_ptr<Resource>> resources;
    std::unordered_map<std::string, uint64_t> assetPathToUID;
    std::unordered_map<std::string, uint64_t> libraryPathToUID;
};
