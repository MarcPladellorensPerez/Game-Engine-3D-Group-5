#pragma once
#include "Component.h"
#include <string>
#include <cstdint>

class ComponentTexture : public Component {
public:
    static constexpr ComponentType StaticType = ComponentType::TEXTURE;

    ComponentTexture(GameObject* owner);
    virtual ~ComponentTexture();

    bool LoadTexture(const std::string& filepath);
    bool LoadFromCustomFile(const std::string& filepath);
    bool SaveToCustomFile(const std::string& filepath) const;
    void SetTexture(unsigned int id, const std::string& filepath, int w, int h);
    void SetResourceTexture(uint64_t uid);
    void SetResourceTexture(const std::string& libraryOrAssetPath);
    void SetDeferredResourceUID(uint64_t uid, const std::string& texPath, int w, int h, int ch);
    void ReleaseResourceReference();
    void AcquireResourceReference();
    uint64_t GetTextureUID() const { return textureUID; }
    void Bind(unsigned int slot = 0);
    void Unbind(unsigned int slot = 0);
    void CleanUp() override;

    unsigned int GetTextureID() const { return textureID; }
    const std::string& GetPath() const { return path; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    int GetChannels() const { return channels; }

    bool showChecker = false;

private:
    unsigned int textureID = 0;
    uint64_t textureUID = 0;
    bool hasResourceRef = false;
    std::string path = "None";
    int width = 0;
    int height = 0;
    int channels = 0;
    bool ownsTexture = false;
};
