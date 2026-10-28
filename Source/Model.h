#pragma once
#include "Mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include <vector>

class GameObject;

class Model
{
public:
    Model(const std::string& path);
    ~Model();

    void Draw(unsigned int shaderProgram) const;
    void CleanUp();

    static GameObject* LoadToGameObject(const std::string& path, GameObject* parent = nullptr);

    const std::vector<Mesh>& GetMeshes() const { return meshes; }
    const std::string& GetPath() const { return modelPath; }
    const std::vector<Texture>& GetLoadedTextures() const { return textures_loaded; }
    bool IsLoaded() const { return !meshes.empty(); }

    size_t GetTotalVertices() const;
    size_t GetTotalIndices() const;

private:
    std::string modelPath;
    std::string directory;
    std::vector<Mesh> meshes;
    std::vector<Texture> textures_loaded;

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};
