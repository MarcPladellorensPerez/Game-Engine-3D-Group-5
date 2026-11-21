#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"

struct CustomModelNode {
    std::string name;
    glm::vec3 position{0.0f};
    glm::vec3 rotationEuler{0.0f};
    glm::vec3 scale{1.0f};
    std::string meshFile;       // Relative path or file name in Library/Meshes/
    std::string textureFile;    // Relative path or file name in Library/Textures/
    std::vector<CustomModelNode> children;
};

class CustomFormat {
public:
    // --- Mesh Binary Format (.cmesh) ---
    static bool SaveMesh(const std::string& filePath,
                         const std::vector<Vertex>& vertices,
                         const std::vector<unsigned int>& indices,
                         const glm::vec3& aabbMin,
                         const glm::vec3& aabbMax);

    static bool LoadMesh(const std::string& filePath,
                         std::vector<Vertex>& outVertices,
                         std::vector<unsigned int>& outIndices,
                         glm::vec3& outMin,
                         glm::vec3& outMax);

    // --- Texture Binary Format (.ctex) ---
    static bool SaveTexture(const std::string& filePath,
                            int width,
                            int height,
                            int channels,
                            const unsigned char* pixelData);

    static bool LoadTexture(const std::string& filePath,
                            int& outWidth,
                            int& outHeight,
                            int& outChannels,
                            std::vector<unsigned char>& outPixelData);

    // --- Model Binary / Metadata Format (.cmodel) ---
    static bool SaveModel(const std::string& filePath, const CustomModelNode& rootNode);
    static bool LoadModel(const std::string& filePath, CustomModelNode& outRootNode);
};
