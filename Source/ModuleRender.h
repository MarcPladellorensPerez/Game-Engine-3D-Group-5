#pragma once
#include "Module.h"
#include "GameObject.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Shader;
class Model;

class ModuleRender : public Module
{
public:
    ModuleRender();
    virtual ~ModuleRender();

    bool Init() override;
    UpdateStatus PreUpdate() override;
    UpdateStatus Update() override;
    UpdateStatus PostUpdate() override;
    bool CleanUp() override;

    bool LoadModel(const std::string& path);
    void ApplyTextureToSelected(const std::string& texturePath);
    GameObject* CreatePrimitive(const std::string& type);
    GameObject* CreateGameObject(const std::string& name = "GameObject", GameObject* parent = nullptr);

    const std::vector<GameObject*>& GetGameObjects() const { return gameObjects; }
    GameObject* GetSelectedGameObject() const { return selectedGameObject; }
    void SetSelectedGameObject(GameObject* go) { selectedGameObject = go; }

public:
    SDL_GLContext GetContext() const { return context; }
    Shader* GetDefaultShader() const { return defaultShader; }
    unsigned int GetCheckerTextureID() const { return checkerTextureID; }

    float clearColor[4] = { 0.1f, 0.25f, 0.5f, 1.0f };
    bool wireframe = false;
    bool depthTest = true;
    bool cullFace = false;
    bool vsync = true;
    bool showCheckerTexture = false;
    bool useLighting = true;

private:
    void CreateCheckerTexture();

private:
    SDL_GLContext context = nullptr;
    GLuint checkerTextureID = 0;
    Shader* defaultShader = nullptr;
    std::vector<GameObject*> gameObjects;
    GameObject* selectedGameObject = nullptr;
};
