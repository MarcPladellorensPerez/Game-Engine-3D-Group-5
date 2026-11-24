#include "ComponentTexture.h"
#include "CustomFormat.h"
#include "ResourceManager.h"
#include "Globals.h"
#include <glad/glad.h>
#include <IL/il.h>
#include <IL/ilu.h>
#include <stb_image.h>
#include <iostream>
#include <filesystem>

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
    if (textureUID != 0)
    {
        if (hasResourceRef)
        {
            ResourceManager::Instance().ReleaseTexture(textureUID);
            hasResourceRef = false;
        }
        textureUID = 0;
        textureID = 0;
    }
    else if (ownsTexture && textureID != 0)
    {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
    ownsTexture = false;
}

void ComponentTexture::SetResourceTexture(uint64_t uid)
{
    if (textureUID == uid && uid != 0 && hasResourceRef) return;
    CleanUp();
    ResourceTexture* res = ResourceManager::Instance().RequestTexture(uid);
    if (res)
    {
        textureUID = res->uid;
        hasResourceRef = true;
        textureID = res->textureID;
        width = res->width;
        height = res->height;
        channels = res->channels;
        path = res->assetsPath.empty() ? res->libraryPath : res->assetsPath;
    }
}

void ComponentTexture::SetResourceTexture(const std::string& libraryOrAssetPath)
{
    CleanUp();
    ResourceTexture* res = ResourceManager::Instance().RequestTextureByPath(libraryOrAssetPath);
    if (res)
    {
        textureUID = res->uid;
        hasResourceRef = true;
        textureID = res->textureID;
        width = res->width;
        height = res->height;
        channels = res->channels;
        path = res->assetsPath.empty() ? res->libraryPath : res->assetsPath;
    }
}

void ComponentTexture::SetDeferredResourceUID(uint64_t uid, const std::string& texPath, int w, int h, int ch)
{
    CleanUp();
    textureUID = uid;
    path = texPath;
    width = w;
    height = h;
    channels = ch;
    hasResourceRef = false;
}

void ComponentTexture::ReleaseResourceReference()
{
    if (textureUID != 0 && hasResourceRef)
    {
        ResourceManager::Instance().ReleaseTexture(textureUID);
        hasResourceRef = false;
        textureID = 0;
    }
}

void ComponentTexture::AcquireResourceReference()
{
    if (textureUID != 0 && !hasResourceRef)
    {
        ResourceTexture* res = ResourceManager::Instance().RequestTexture(textureUID);
        if (res)
        {
            hasResourceRef = true;
            textureID = res->textureID;
            width = res->width;
            height = res->height;
            channels = res->channels;
            path = res->assetsPath.empty() ? res->libraryPath : res->assetsPath;
        }
    }
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
    std::string finalPath = filepath;
    if (!std::filesystem::exists(finalPath))
    {
        size_t lastSlash = filepath.find_last_of("/\\");
        std::string baseName = (lastSlash != std::string::npos) ? filepath.substr(lastSlash + 1) : filepath;
        std::string alt = "assets/" + baseName;
        if (std::filesystem::exists(alt))
        {
            finalPath = alt;
        }
    }

    // Check ResourceManager cache first to reuse existing GPU texture and ref count
    ResourceTexture* res = ResourceManager::Instance().RequestTextureByPath(finalPath);
    if (res && res->textureID != 0)
    {
        CleanUp();
        textureUID = res->uid;
        hasResourceRef = true;
        textureID = res->textureID;
        width = res->width;
        height = res->height;
        channels = res->channels;
        path = res->assetsPath.empty() ? res->libraryPath : res->assetsPath;
        ownsTexture = false;
        return true;
    }
    else if (res)
    {
        ResourceManager::Instance().ReleaseTexture(res->uid);
    }

    EnsureDevILInit();
    CleanUp();

    // Load with DevIL (supports DDS, PNG, TGA, JPG, etc.)
    ILuint imgId = 0;
    ilGenImages(1, &imgId);
    ilBindImage(imgId);

    if (ilLoadImage(finalPath.c_str()))
    {
        ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
        int w = ilGetInteger(IL_IMAGE_WIDTH);
        int h = ilGetInteger(IL_IMAGE_HEIGHT);
        int comp = 4;
        unsigned char* data = ilGetData();

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
        channels = 4; // stbi_load was called with req_comp=4, output is always RGBA
        path = finalPath;
        ownsTexture = true;

        LOG("[DevIL] Loaded texture successfully: %s (%dx%d, %d channels)", finalPath.c_str(), width, height, channels);
        return true;
    }
    else
    {
        ilDeleteImages(1, &imgId);
    }

    // Fallback to stb_image
    stbi_set_flip_vertically_on_load(false);
    int w = 0, h = 0, comp = 0;
    unsigned char* data = stbi_load(finalPath.c_str(), &w, &h, &comp, 4);
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
        channels = 4; // stbi_load was called with req_comp=4, output is always RGBA
        path = finalPath;
        ownsTexture = true;

        LOG("[STB] Loaded texture: %s (%dx%d)", finalPath.c_str(), width, height);
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

bool ComponentTexture::LoadFromCustomFile(const std::string& filepath)
{
    SetResourceTexture(filepath);
    if (textureUID != 0) return true;

    CleanUp();

    int w = 0, h = 0, c = 0;
    std::vector<unsigned char> data;
    if (!CustomFormat::LoadTexture(filepath, w, h, c, data) || data.empty())
    {
        LOG("[ComponentTexture] Failed to load custom texture: %s", filepath.c_str());
        return false;
    }

    GLenum format = (c == 4) ? GL_RGBA : GL_RGB;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    width = w;
    height = h;
    channels = c;
    path = filepath;
    ownsTexture = true;

    LOG("[ComponentTexture] Loaded custom binary texture: %s (%dx%d, %d channels)",
        filepath.c_str(), width, height, channels);
    return true;
}

bool ComponentTexture::SaveToCustomFile(const std::string& filepath) const
{
    if (textureID == 0 || width <= 0 || height <= 0) return false;

    std::vector<unsigned char> pixels(width * height * 4);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    return CustomFormat::SaveTexture(filepath, width, height, 4, pixels.data());
}
