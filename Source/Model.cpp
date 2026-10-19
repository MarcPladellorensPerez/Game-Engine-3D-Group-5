#include "Model.h"
#include "Globals.h"
#include <iostream>
#include <cstring>

Model::Model(const std::string& path)
    : modelPath(path)
{
    loadModel(path);
}

Model::~Model()
{
    CleanUp();
}

void Model::Draw(unsigned int shaderProgram) const
{
    for (const auto& mesh : meshes)
    {
        mesh.Draw(shaderProgram);
    }
}

void Model::CleanUp()
{
    for (auto& mesh : meshes)
    {
        mesh.CleanUp();
    }
    meshes.clear();
    textures_loaded.clear();
}

size_t Model::GetTotalVertices() const
{
    size_t count = 0;
    for (const auto& mesh : meshes)
    {
        count += mesh.vertices.size();
    }
    return count;
}

size_t Model::GetTotalIndices() const
{
    size_t count = 0;
    for (const auto& mesh : meshes)
    {
        count += mesh.indices.size();
    }
    return count;
}

void Model::loadModel(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOG("ERROR::ASSIMP: %s", importer.GetErrorString());
        return;
    }

    size_t lastSlash = path.find_last_of("/\\");
    directory = (lastSlash != std::string::npos) ? path.substr(0, lastSlash) : "";

    processNode(scene->mRootNode, scene);

    LOG("Loaded 3D Model: %s (%zu meshes, %zu vertices, %zu triangles)",
        path.c_str(), meshes.size(), GetTotalVertices(), GetTotalIndices() / 3);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;

        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        if (mesh->HasNormals())
        {
            vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }
        else
        {
            vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (mesh->mTextureCoords[0])
        {
            vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        else
        {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    }

    return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        std::string rawPath = str.C_Str();
        size_t lastSlash = rawPath.find_last_of("/\\");
        std::string baseName = (lastSlash != std::string::npos) ? rawPath.substr(lastSlash + 1) : rawPath;

        bool skip = false;
        for (const auto& loaded : textures_loaded)
        {
            if (loaded.path == baseName || loaded.path == rawPath)
            {
                textures.push_back(loaded);
                skip = true;
                break;
            }
        }

        if (!skip)
        {
            Texture texture;
            texture.id = Texture::TextureFromFile(baseName.c_str(), directory);
            texture.type = typeName;
            texture.path = baseName;
            if (texture.id != 0)
            {
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
    }
    return textures;
}
