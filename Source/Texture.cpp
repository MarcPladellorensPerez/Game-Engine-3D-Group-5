#include "Texture.h"
#include "Globals.h"
#include <glad/glad.h>
#include <stb_image.h>

unsigned int Texture::TextureFromFile(const char* path, const std::string& directory)
{
    std::string filename = std::string(path);

    // Extract base filename if path contains directory separators
    size_t lastSlash = filename.find_last_of("/\\");
    std::string baseName = (lastSlash != std::string::npos) ? filename.substr(lastSlash + 1) : filename;

    std::string fullPath = directory.empty() ? filename : (directory + '/' + baseName);

    int width = 0, height = 0, nrComponents = 0;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, 0);

    // Try fallback search paths
    if (!data)
    {
        fullPath = "assets/" + baseName;
        data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, 0);
    }

    if (!data)
    {
        LOG("ERROR::TEXTURE: Failed to load at path %s", path);
        return 0;
    }

    unsigned int textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    GLenum format = GL_RGB;
    if (nrComponents == 1)
        format = GL_RED;
    else if (nrComponents == 3)
        format = GL_RGB;
    else if (nrComponents == 4)
        format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    LOG("Loaded texture: %s (%dx%d, %d channels)", fullPath.c_str(), width, height, nrComponents);

    return textureID;
}
