#include "CustomFormat.h"
#include "Globals.h"
#include <fstream>
#include <cstring>

static const char MESH_MAGIC[4] = {'C', 'M', 'S', 'H'};
static const char TEX_MAGIC[4]  = {'C', 'T', 'E', 'X'};
static const char MDL_MAGIC[4]  = {'C', 'M', 'D', 'L'};
static const uint32_t FORMAT_VERSION = 1;

static void WriteString(std::ofstream& stream, const std::string& str)
{
    uint32_t len = static_cast<uint32_t>(str.size());
    stream.write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0)
    {
        stream.write(str.data(), len);
    }
}

static bool ReadString(std::ifstream& stream, std::string& str)
{
    uint32_t len = 0;
    if (!stream.read(reinterpret_cast<char*>(&len), sizeof(len))) return false;
    str.resize(len);
    if (len > 0)
    {
        if (!stream.read(&str[0], len)) return false;
    }
    return true;
}

static void WriteNode(std::ofstream& stream, const CustomModelNode& node)
{
    WriteString(stream, node.name);
    stream.write(reinterpret_cast<const char*>(&node.position), sizeof(glm::vec3));
    stream.write(reinterpret_cast<const char*>(&node.rotationEuler), sizeof(glm::vec3));
    stream.write(reinterpret_cast<const char*>(&node.scale), sizeof(glm::vec3));
    WriteString(stream, node.meshFile);
    WriteString(stream, node.textureFile);

    uint32_t childCount = static_cast<uint32_t>(node.children.size());
    stream.write(reinterpret_cast<const char*>(&childCount), sizeof(childCount));
    for (const auto& child : node.children)
    {
        WriteNode(stream, child);
    }
}

static bool ReadNode(std::ifstream& stream, CustomModelNode& node)
{
    if (!ReadString(stream, node.name)) return false;
    if (!stream.read(reinterpret_cast<char*>(&node.position), sizeof(glm::vec3))) return false;
    if (!stream.read(reinterpret_cast<char*>(&node.rotationEuler), sizeof(glm::vec3))) return false;
    if (!stream.read(reinterpret_cast<char*>(&node.scale), sizeof(glm::vec3))) return false;
    if (!ReadString(stream, node.meshFile)) return false;
    if (!ReadString(stream, node.textureFile)) return false;

    uint32_t childCount = 0;
    if (!stream.read(reinterpret_cast<char*>(&childCount), sizeof(childCount))) return false;
    node.children.resize(childCount);
    for (uint32_t i = 0; i < childCount; ++i)
    {
        if (!ReadNode(stream, node.children[i])) return false;
    }
    return true;
}

bool CustomFormat::SaveMesh(const std::string& filePath,
                           const std::vector<Vertex>& vertices,
                           const std::vector<unsigned int>& indices,
                           const glm::vec3& aabbMin,
                           const glm::vec3& aabbMax)
{
    std::ofstream stream(filePath, std::ios::binary);
    if (!stream.is_open())
    {
        LOG("[CustomFormat] Failed to open mesh file for writing: %s", filePath.c_str());
        return false;
    }

    stream.write(MESH_MAGIC, 4);
    stream.write(reinterpret_cast<const char*>(&FORMAT_VERSION), sizeof(FORMAT_VERSION));

    uint32_t vCount = static_cast<uint32_t>(vertices.size());
    uint32_t iCount = static_cast<uint32_t>(indices.size());
    stream.write(reinterpret_cast<const char*>(&vCount), sizeof(vCount));
    stream.write(reinterpret_cast<const char*>(&iCount), sizeof(iCount));

    stream.write(reinterpret_cast<const char*>(&aabbMin), sizeof(glm::vec3));
    stream.write(reinterpret_cast<const char*>(&aabbMax), sizeof(glm::vec3));

    if (vCount > 0)
    {
        stream.write(reinterpret_cast<const char*>(vertices.data()), vCount * sizeof(Vertex));
    }
    if (iCount > 0)
    {
        stream.write(reinterpret_cast<const char*>(indices.data()), iCount * sizeof(unsigned int));
    }

    return true;
}

bool CustomFormat::LoadMesh(const std::string& filePath,
                           std::vector<Vertex>& outVertices,
                           std::vector<unsigned int>& outIndices,
                           glm::vec3& outMin,
                           glm::vec3& outMax)
{
    std::ifstream stream(filePath, std::ios::binary);
    if (!stream.is_open())
    {
        LOG("[CustomFormat] Failed to open mesh file for reading: %s", filePath.c_str());
        return false;
    }

    char magic[4];
    stream.read(magic, 4);
    if (std::memcmp(magic, MESH_MAGIC, 4) != 0)
    {
        LOG("[CustomFormat] Invalid mesh magic in %s", filePath.c_str());
        return false;
    }

    uint32_t version = 0;
    stream.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != FORMAT_VERSION)
    {
        LOG("[CustomFormat] Unsupported mesh format version: %u", version);
        return false;
    }

    uint32_t vCount = 0;
    uint32_t iCount = 0;
    stream.read(reinterpret_cast<char*>(&vCount), sizeof(vCount));
    stream.read(reinterpret_cast<char*>(&iCount), sizeof(iCount));

    stream.read(reinterpret_cast<char*>(&outMin), sizeof(glm::vec3));
    stream.read(reinterpret_cast<char*>(&outMax), sizeof(glm::vec3));

    outVertices.resize(vCount);
    if (vCount > 0)
    {
        stream.read(reinterpret_cast<char*>(outVertices.data()), vCount * sizeof(Vertex));
    }

    outIndices.resize(iCount);
    if (iCount > 0)
    {
        stream.read(reinterpret_cast<char*>(outIndices.data()), iCount * sizeof(unsigned int));
    }

    return true;
}

bool CustomFormat::SaveTexture(const std::string& filePath,
                              int width,
                              int height,
                              int channels,
                              const unsigned char* pixelData)
{
    std::ofstream stream(filePath, std::ios::binary);
    if (!stream.is_open())
    {
        LOG("[CustomFormat] Failed to open texture file for writing: %s", filePath.c_str());
        return false;
    }

    stream.write(TEX_MAGIC, 4);
    stream.write(reinterpret_cast<const char*>(&FORMAT_VERSION), sizeof(FORMAT_VERSION));

    uint32_t w = static_cast<uint32_t>(width);
    uint32_t h = static_cast<uint32_t>(height);
    uint32_t c = static_cast<uint32_t>(channels);
    uint32_t dataSize = w * h * c;

    stream.write(reinterpret_cast<const char*>(&w), sizeof(w));
    stream.write(reinterpret_cast<const char*>(&h), sizeof(h));
    stream.write(reinterpret_cast<const char*>(&c), sizeof(c));
    stream.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));

    if (dataSize > 0 && pixelData)
    {
        stream.write(reinterpret_cast<const char*>(pixelData), dataSize);
    }

    return true;
}

bool CustomFormat::LoadTexture(const std::string& filePath,
                              int& outWidth,
                              int& outHeight,
                              int& outChannels,
                              std::vector<unsigned char>& outPixelData)
{
    std::ifstream stream(filePath, std::ios::binary);
    if (!stream.is_open())
    {
        LOG("[CustomFormat] Failed to open texture file for reading: %s", filePath.c_str());
        return false;
    }

    char magic[4];
    stream.read(magic, 4);
    if (std::memcmp(magic, TEX_MAGIC, 4) != 0)
    {
        LOG("[CustomFormat] Invalid texture magic in %s", filePath.c_str());
        return false;
    }

    uint32_t version = 0;
    stream.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != FORMAT_VERSION)
    {
        LOG("[CustomFormat] Unsupported texture format version: %u", version);
        return false;
    }

    uint32_t w = 0, h = 0, c = 0, dataSize = 0;
    stream.read(reinterpret_cast<char*>(&w), sizeof(w));
    stream.read(reinterpret_cast<char*>(&h), sizeof(h));
    stream.read(reinterpret_cast<char*>(&c), sizeof(c));
    stream.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));

    outWidth = static_cast<int>(w);
    outHeight = static_cast<int>(h);
    outChannels = static_cast<int>(c);

    outPixelData.resize(dataSize);
    if (dataSize > 0)
    {
        stream.read(reinterpret_cast<char*>(outPixelData.data()), dataSize);
    }

    return true;
}

bool CustomFormat::SaveModel(const std::string& filePath, const CustomModelNode& rootNode)
{
    std::ofstream stream(filePath, std::ios::binary);
    if (!stream.is_open())
    {
        LOG("[CustomFormat] Failed to open model file for writing: %s", filePath.c_str());
        return false;
    }

    stream.write(MDL_MAGIC, 4);
    stream.write(reinterpret_cast<const char*>(&FORMAT_VERSION), sizeof(FORMAT_VERSION));

    WriteNode(stream, rootNode);
    return true;
}

bool CustomFormat::LoadModel(const std::string& filePath, CustomModelNode& outRootNode)
{
    std::ifstream stream(filePath, std::ios::binary);
    if (!stream.is_open())
    {
        LOG("[CustomFormat] Failed to open model file for reading: %s", filePath.c_str());
        return false;
    }

    char magic[4];
    stream.read(magic, 4);
    if (std::memcmp(magic, MDL_MAGIC, 4) != 0)
    {
        LOG("[CustomFormat] Invalid model magic in %s", filePath.c_str());
        return false;
    }

    uint32_t version = 0;
    stream.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != FORMAT_VERSION)
    {
        LOG("[CustomFormat] Unsupported model format version: %u", version);
        return false;
    }

    return ReadNode(stream, outRootNode);
}
