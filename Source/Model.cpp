#include "Model.h"
#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include "CustomFormat.h"
#include "Globals.h"
#include <glad/glad.h>
#include <iostream>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <unordered_map>

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

    for (auto& tex : textures_loaded)
    {
        if (tex.id != 0)
        {
            glDeleteTextures(1, &tex.id);
            tex.id = 0;
        }
    }
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

// ---------------------------------------------------------------------------
// Helper: extract vertices and indices from an aiMesh with optional uniform scale.
// AABB min/max are computed in mesh-local space (after scale).
// ---------------------------------------------------------------------------
static void ExtractMeshData(const aiMesh* mesh,
                             float scaleFactor,
                             std::vector<Vertex>& outVertices,
                             std::vector<unsigned int>& outIndices,
                             glm::vec3& outMin,
                             glm::vec3& outMax)
{
    outVertices.reserve(mesh->mNumVertices);
    outMin = glm::vec3(1e9f);
    outMax = glm::vec3(-1e9f);

    for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
    {
        Vertex vertex;
        vertex.Position = glm::vec3(mesh->mVertices[v].x,
                                    mesh->mVertices[v].y,
                                    mesh->mVertices[v].z) * scaleFactor;
        vertex.Normal = mesh->HasNormals()
            ? glm::vec3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z)
            : glm::vec3(0.0f, 1.0f, 0.0f);
        vertex.TexCoords = mesh->mTextureCoords[0]
            ? glm::vec2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y)
            : glm::vec2(0.0f, 0.0f);
        outVertices.push_back(vertex);

        outMin = glm::min(outMin, vertex.Position);
        outMax = glm::max(outMax, vertex.Position);
    }

    for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
    {
        const aiFace& face = mesh->mFaces[f];
        for (unsigned int ind = 0; ind < face.mNumIndices; ++ind)
            outIndices.push_back(face.mIndices[ind]);
    }
}

GameObject* Model::LoadToGameObject(const std::string& path, GameObject* parent)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOG("[ASSIMP] Error loading %s: %s", path.c_str(), importer.GetErrorString());
        return nullptr;
    }

    size_t lastSlash = path.find_last_of("/\\");
    std::string directory = (lastSlash != std::string::npos) ? path.substr(0, lastSlash) : "";
    std::string baseFilename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
    size_t dotPos = baseFilename.find_last_of('.');
    std::string modelName = (dotPos != std::string::npos) ? baseFilename.substr(0, dotPos) : baseFilename;

    GameObject* rootGO = new GameObject(modelName, parent);

    LOG("[ASSIMP] Loading 3D Model: %s (%u meshes)", path.c_str(), scene->mNumMeshes);

    // Compute overall model bounding box to detect unit scale (e.g. centimeters vs meters)
    float minX = 1e9f, maxX = -1e9f;
    float minY = 1e9f, maxY = -1e9f;
    float minZ = 1e9f, maxZ = -1e9f;
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* m = scene->mMeshes[i];
        for (unsigned int v = 0; v < m->mNumVertices; ++v)
        {
            minX = std::min(minX, m->mVertices[v].x); maxX = std::max(maxX, m->mVertices[v].x);
            minY = std::min(minY, m->mVertices[v].y); maxY = std::max(maxY, m->mVertices[v].y);
            minZ = std::min(minZ, m->mVertices[v].z); maxZ = std::max(maxZ, m->mVertices[v].z);
        }
    }
    float maxDim = std::max({maxX - minX, maxY - minY, maxZ - minZ});
    float scaleFactor = (maxDim > 50.0f) ? 0.01f : 1.0f;
    if (scaleFactor != 1.0f)
    {
        LOG("[ASSIMP] Model '%s' was exported in centimeters (max dimension %.1f). Automatically scaled to meters.",
            modelName.c_str(), maxDim);
    }

    std::unordered_map<std::string, ComponentTexture*> loadedTextures;
    std::string cachedModelTexture = "";

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[i];
        std::string meshName = (mesh->mName.length > 0) ? mesh->mName.C_Str() : (modelName + " Mesh " + std::to_string(i));

        // Create a GameObject for each loaded mesh
        GameObject* meshGO = new GameObject(meshName, rootGO);

        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        glm::vec3 aabbMin, aabbMax;
        ExtractMeshData(mesh, scaleFactor, vertices, indices, aabbMin, aabbMax);

        ComponentMesh* compMesh = static_cast<ComponentMesh*>(meshGO->CreateComponent(ComponentType::MESH));
        if (compMesh)
        {
            compMesh->InitBuffers(vertices, indices);
            compMesh->meshName = meshName;
        }

        bool texLoaded = false;
        std::string resolvedTexPath;

        // Check if we previously found a shared texture for this model
        if (!cachedModelTexture.empty() && std::filesystem::exists(cachedModelTexture))
        {
            resolvedTexPath = cachedModelTexture;
        }

        // 1. Load diffuse texture if material specifies one
        if (resolvedTexPath.empty() && mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            aiTextureType texTypes[] = { aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR };
            for (auto tType : texTypes)
            {
                if (!resolvedTexPath.empty()) break;
                if (material->GetTextureCount(tType) > 0)
                {
                    aiString texPath;
                    material->GetTexture(tType, 0, &texPath);
                    std::string rawTex = texPath.C_Str();
                    size_t texSlash = rawTex.find_last_of("/\\");
                    std::string texBase = (texSlash != std::string::npos) ? rawTex.substr(texSlash + 1) : rawTex;

                    // Try various candidate locations
                    std::vector<std::string> candidates;
                    if (!directory.empty())
                    {
                        candidates.push_back(directory + "/" + texBase);
                        candidates.push_back(directory + "/" + rawTex);
                    }
                    candidates.push_back("assets/" + texBase);
                    candidates.push_back("assets/" + rawTex);
                    candidates.push_back(texBase);
                    candidates.push_back(rawTex);

                    for (const auto& cand : candidates)
                    {
                        if (std::filesystem::exists(cand))
                        {
                            resolvedTexPath = cand;
                            break;
                        }
                    }
                }
            }
        }

        // 2. Fallback search by modelName and meshName in directory and assets/
        if (resolvedTexPath.empty())
        {
            const std::vector<std::string> exts = { ".png", ".PNG", ".jpg", ".JPG", ".jpeg", ".JPEG", ".tga", ".TGA", ".dds", ".DDS" };
            std::vector<std::string> nameCandidates;
            for (const auto& ext : exts)
            {
                if (!directory.empty())
                {
                    nameCandidates.push_back(directory + "/" + modelName + ext);
                    nameCandidates.push_back(directory + "/" + meshName + ext);
                }
                nameCandidates.push_back("assets/" + modelName + ext);
                nameCandidates.push_back("assets/" + meshName + ext);
            }

            for (const auto& cand : nameCandidates)
            {
                if (std::filesystem::exists(cand))
                {
                    resolvedTexPath = cand;
                    break;
                }
            }
        }

        // 3. Fallback specifically for BakerHouse only
        if (resolvedTexPath.empty())
        {
            std::string lowerModel = modelName;
            for (char& c : lowerModel) c = static_cast<char>(tolower(c));
            if (lowerModel.find("baker") != std::string::npos)
            {
                if (std::filesystem::exists("assets/Baker_house.png"))
                {
                    resolvedTexPath = "assets/Baker_house.png";
                }
            }
        }

        // Now load or reuse the texture
        if (!resolvedTexPath.empty())
        {
            cachedModelTexture = resolvedTexPath;
            auto it = loadedTextures.find(resolvedTexPath);
            ComponentTexture* compTex = static_cast<ComponentTexture*>(meshGO->CreateComponent(ComponentType::TEXTURE));
            if (it != loadedTextures.end() && it->second != nullptr)
            {
                // Reuse existing loaded texture!
                compTex->SetTexture(it->second->GetTextureID(), it->second->GetPath(),
                                    it->second->GetWidth(), it->second->GetHeight());
                texLoaded = true;
            }
            else if (compTex && compTex->LoadTexture(resolvedTexPath))
            {
                loadedTextures[resolvedTexPath] = compTex;
                texLoaded = true;
            }
        }

        if (!texLoaded)
        {
            LOG("[Texture] No diffuse texture found for mesh '%s' of '%s'. Rendering with untextured material color.",
                meshName.c_str(), modelName.c_str());
        }
    }

    LOG("[ASSIMP] Successfully created GameObject hierarchy for %s (%zu meshes)",
        modelName.c_str(), rootGO->GetChildren().size());

    return rootGO;
}

bool Model::ExportToCustomFormat(const std::string& sourceModelPath,
                                const std::string& outputDir,
                                std::string& outModelFile)
{
    std::filesystem::path srcPath(sourceModelPath);
    std::string modelName = srcPath.stem().string();
    outModelFile = outputDir + "/Models/" + modelName + ".cmodel";

    // Fast path: if the custom format already exists and is newer than the source, reuse it
    if (std::filesystem::exists(outModelFile) && std::filesystem::exists(sourceModelPath))
    {
        std::error_code ec;
        auto srcTime = std::filesystem::last_write_time(sourceModelPath, ec);
        auto outTime = std::filesystem::last_write_time(outModelFile, ec);
        if (!ec && outTime >= srcTime)
        {
            return true;
        }
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(sourceModelPath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOG("ERROR::ASSIMP: %s", importer.GetErrorString());
        return false;
    }

    std::string directory = srcPath.parent_path().string();

    std::filesystem::create_directories(outputDir + "/Meshes");
    std::filesystem::create_directories(outputDir + "/Textures");
    std::filesystem::create_directories(outputDir + "/Models");

    float minX = 1e9f, maxX = -1e9f;
    float minY = 1e9f, maxY = -1e9f;
    float minZ = 1e9f, maxZ = -1e9f;
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* m = scene->mMeshes[i];
        for (unsigned int v = 0; v < m->mNumVertices; ++v)
        {
            minX = std::min(minX, m->mVertices[v].x); maxX = std::max(maxX, m->mVertices[v].x);
            minY = std::min(minY, m->mVertices[v].y); maxY = std::max(maxY, m->mVertices[v].y);
            minZ = std::min(minZ, m->mVertices[v].z); maxZ = std::max(maxZ, m->mVertices[v].z);
        }
    }
    float maxDim = std::max({maxX - minX, maxY - minY, maxZ - minZ});
    float scaleFactor = (maxDim > 50.0f) ? 0.01f : 1.0f;

    CustomModelNode rootNode;
    rootNode.name = modelName;
    rootNode.position = glm::vec3(0.0f);
    rootNode.rotationEuler = glm::vec3(0.0f);
    rootNode.scale = glm::vec3(scaleFactor); // Apply unit scale via transform, not baked into vertices

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[i];
        std::string rawMeshName = (mesh->mName.length > 0) ? mesh->mName.C_Str() : "mesh";
        std::string safeMeshName = modelName + "_" + rawMeshName + "_" + std::to_string(i);
        for (char& c : safeMeshName)
        {
            if (c == ' ' || c == ':' || c == '/' || c == '\\') c = '_';
        }

        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        glm::vec3 minP, maxP;
        // Vertices stored with raw coords; unit conversion is in rootNode.scale
        ExtractMeshData(mesh, 1.0f, vertices, indices, minP, maxP);

        std::string meshBinaryPath = outputDir + "/Meshes/" + safeMeshName + ".cmesh";
        CustomFormat::SaveMesh(meshBinaryPath, vertices, indices, minP, maxP);

        std::string texBinaryPath = "";
        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            aiString texPath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS ||
                material->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS)
            {
                std::string rawTex = texPath.C_Str();
                std::filesystem::path tp(rawTex);
                std::string texBase = tp.filename().string();
                std::string candidate = directory + "/" + texBase;
                if (!std::filesystem::exists(candidate)) candidate = directory + "/" + rawTex;
                if (!std::filesystem::exists(candidate)) candidate = "assets/" + texBase;
                if (!std::filesystem::exists(candidate)) candidate = "assets/" + rawTex;

                if (std::filesystem::exists(candidate))
                {
                    std::string safeTexName = tp.stem().string();
                    texBinaryPath = outputDir + "/Textures/" + safeTexName + ".ctex";
                    if (!std::filesystem::exists(texBinaryPath))
                    {
                        ComponentTexture tempTex(nullptr);
                        if (tempTex.LoadTexture(candidate))
                        {
                            tempTex.SaveToCustomFile(texBinaryPath);
                        }
                    }
                }
            }
        }

        // Fallback: if material has no texture path, try finding modelName image
        if (texBinaryPath.empty())
        {
            const std::vector<std::string> exts = { ".png", ".jpg", ".jpeg", ".tga", ".dds" };
            for (const auto& ext : exts)
            {
                std::string candidate = directory + "/" + modelName + ext;
                if (!std::filesystem::exists(candidate)) candidate = "assets/" + modelName + ext;
                if (std::filesystem::exists(candidate))
                {
                    texBinaryPath = outputDir + "/Textures/" + modelName + ".ctex";
                    if (!std::filesystem::exists(texBinaryPath))
                    {
                        ComponentTexture tempTex(nullptr);
                        if (tempTex.LoadTexture(candidate))
                        {
                            tempTex.SaveToCustomFile(texBinaryPath);
                        }
                    }
                    break;
                }
            }
        }

        CustomModelNode childNode;
        childNode.name = (mesh->mName.length > 0) ? mesh->mName.C_Str() : (modelName + "_Mesh_" + std::to_string(i));
        childNode.position = glm::vec3(0.0f);
        childNode.rotationEuler = glm::vec3(0.0f);
        childNode.scale = glm::vec3(1.0f);
        childNode.meshFile = meshBinaryPath;
        childNode.textureFile = texBinaryPath;
        rootNode.children.push_back(childNode);
    }

    outModelFile = outputDir + "/Models/" + modelName + ".cmodel";
    return CustomFormat::SaveModel(outModelFile, rootNode);
}

static GameObject* BuildGOFromNode(const CustomModelNode& node, GameObject* parent)
{
    GameObject* go = new GameObject(node.name, parent);
    ComponentTransform* transform = go->GetComponent<ComponentTransform>();
    if (transform)
    {
        transform->SetPosition(node.position);
        transform->SetRotation(node.rotationEuler);
        transform->SetScale(node.scale);
    }

    if (!node.meshFile.empty())
    {
        ComponentMesh* mesh = static_cast<ComponentMesh*>(go->CreateComponent(ComponentType::MESH));
        if (mesh) mesh->LoadFromCustomFile(node.meshFile);
    }
    if (!node.textureFile.empty())
    {
        ComponentTexture* texture = static_cast<ComponentTexture*>(go->CreateComponent(ComponentType::TEXTURE));
        if (texture) texture->LoadFromCustomFile(node.textureFile);
    }

    for (const auto& childNode : node.children)
    {
        BuildGOFromNode(childNode, go);
    }
    return go;
}

GameObject* Model::LoadFromCustomModel(const std::string& customModelPath, GameObject* parent)
{
    CustomModelNode rootNode;
    if (!CustomFormat::LoadModel(customModelPath, rootNode))
    {
        LOG("[Model] Failed to load custom model from %s", customModelPath.c_str());
        return nullptr;
    }
    return BuildGOFromNode(rootNode, parent);
}

