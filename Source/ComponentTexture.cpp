#include "ComponentTexture.h"
#include "Globals.h"
#include <glad/glad.h>
#include <IL/il.h>
#include <IL/ilu.h>
#include <stb_image.h>
#include <iostream>

static bool devILInitialized = false;

static void EnsureDevILInit()
{
    if (!devILInitialized)
    {
        ilInit();
        iluInit();
        devILInitialized = true;
        LOG("Initialized DevIL image library");
    }
}

ComponentTexture::ComponentTexture(GameObject* owner)
    : Component(owner, ComponentType::TEXTURE)
{
}

ComponentTexture::~ComponentTexture()
{
    CleanUp();
}

void ComponentTexture::CleanUp()
{
    if (ownsTexture && textureID != 0)
    {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
    ownsTexture = false;
}

void ComponentTexture::SetTexture(unsigned int id, const std::string& filepath, int w, int h)
{
    CleanUp();
    textureID = id;
    path = filepath;
    width = w;
    height = h;
    ownsTexture = false;
}

bool ComponentTexture::LoadTexture(const std::string& filepath)
{
    EnsureDevILInit();
    CleanUp();

    std::string finalPath = filepath;

    // Check if file exists, or check fallback in assets/
    FILE* f = fopen(finalPath.c_str(), "rb");
    if (!f)
    {
        size_t lastSlash = filepath.find_last_of("/\\");
        std::string baseName = (lastSlash != std::string::npos) ? filepath.substr(lastSlash + 1) : filepath;
        finalPath = "assets/" + baseName;
        f = fopen(finalPath.c_str(), "rb");
    }
    if (f) fclose(f);

    bool loaded = false;
    unsigned char* data = nullptr;
    int w = 0, h = 0, comp = 0;

    // 1. Try loading with DevIL (Supports DDS, PNG, JPG, TGA, BMP, etc.)
    ILuint imgId = 0;
    ilGenImages(1, &imgId);
    ilBindImage(imgId);

    if (ilLoadImage(finalPath.c_str()))
    {
        ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
        w = ilGetInteger(IL_IMAGE_WIDTH);
        h = ilGetInteger(IL_IMAGE_HEIGHT);
        comp = 4;
        data = ilGetData();

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        ilDeleteImages(1, &imgId);

        width = w;
        height = h;
        channels = comp;
        path = finalPath;
        ownsTexture = true;
        loaded = true;

        LOG("[DevIL] Loaded texture successfully: %s (%dx%d, %d channels)", finalPath.c_str(), width, height, channels);
        return true;
    }
    else
    {
        ilDeleteImages(1, &imgId);
    }

    // 2. Fallback to STB Image if DevIL could not load the specific file
    stbi_set_flip_vertically_on_load(false);
    data = stbi_load(finalPath.c_str(), &w, &h, &comp, 4);
    if (data)
    {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(data);

        width = w;
        height = h;
        channels = comp;
        path = finalPath;
        ownsTexture = true;

        LOG("[STB] Loaded texture successfully: %s (%dx%d)", finalPath.c_str(), width, height);
        return true;
    }

    LOG("ERROR::TEXTURE: Failed to load texture at %s", filepath.c_str());
    return false;
}

void ComponentTexture::Bind(unsigned int slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void ComponentTexture::Unbind(unsigned int slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, 0);
}
