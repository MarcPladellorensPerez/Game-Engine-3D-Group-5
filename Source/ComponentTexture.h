#pragma once
#include "Component.h"
#include <string>

class ComponentTexture : public Component {
public:
    ComponentTexture(GameObject* owner);
    virtual ~ComponentTexture();

    bool LoadTexture(const std::string& filepath);
    void SetTexture(unsigned int id, const std::string& filepath, int w, int h);
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
    std::string path = "None";
    int width = 0;
    int height = 0;
    int channels = 0;
    bool ownsTexture = false;
};
