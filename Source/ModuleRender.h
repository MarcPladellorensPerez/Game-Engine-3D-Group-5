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

    GameObject* LoadModel(const std::string& path);
    void ApplyTextureToSelected(const std::string& texturePath);
    GameObject* CreatePrimitive(const std::string& type, GameObject* parent = nullptr);
    GameObject* CreateGameObject(const std::string& name = "GameObject", GameObject* parent = nullptr);

    int GetRootGameObjectIndex(GameObject* go) const;
    void AddGameObject(GameObject* go, GameObject* parent = nullptr, int index = -1);
    void DetachGameObject(GameObject* go);
    void DeleteGameObject(GameObject* go);
    void ClearScene();
    bool IsDescendantOf(GameObject* candidate, GameObject* potentialAncestor) const;

    const std::vector<GameObject*>& GetGameObjects() const { return gameObjects; }
    GameObject* GetSelectedGameObject() const { return selectedGameObject; }
    void SetSelectedGameObject(GameObject* go) { selectedGameObject = go; }

public:
    SDL_GLContext GetContext() const { return context; }
    Shader* GetDefaultShader() const { return defaultShader; }
    unsigned int GetCheckerTextureID() const { return checkerTextureID; }

    float clearColor[4] = { 0.15f, 0.15f, 0.15f, 1.0f };
    bool wireframe = false;
    bool depthTest = true;
    bool cullFace = false;
    bool vsync = true;
    bool showCheckerTexture = false;
    bool useLighting = true;

    // 3D Grid
    bool showGrid = true;
    bool showGridAxes = true;
    float gridSize = 64.0f;
    float gridStep = 1.0f;

    void CreateGridBuffers();
    void DrawGrid();
    void DestroyGridBuffers();

private:
    void CreateCheckerTexture();

private:
    SDL_GLContext context = nullptr;
    GLuint checkerTextureID = 0;
    Shader* defaultShader = nullptr;
    std::vector<GameObject*> gameObjects;
    GameObject* selectedGameObject = nullptr;

    // Grid buffers
    unsigned int gridVAO = 0;
    unsigned int gridVBO = 0;
    size_t gridVertexCount = 0;

    unsigned int gridAxisVAO = 0;
    unsigned int gridAxisVBO = 0;
    size_t gridAxisVertexCount = 0;
};
