#pragma once
#include "Module.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

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
    Model* GetCurrentModel() const { return currentModel; }

public:
    SDL_GLContext GetContext() const { return context; }
    Shader* GetDefaultShader() const { return defaultShader; }
    glm::mat4 GetModelMatrix() const;

    float clearColor[4] = { 0.1f, 0.25f, 0.5f, 1.0f };
    bool wireframe = false;
    bool depthTest = true;
    bool cullFace = false;
    bool vsync = true;
    bool showCheckerTexture = false;
    bool useLighting = true;

    // Transform for 3D model
    glm::vec3 modelPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 modelRotation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 modelScale = glm::vec3(1.0f, 1.0f, 1.0f);

private:
    void CreateCheckerTexture();

private:
    SDL_GLContext context = nullptr;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint checkerTextureID = 0;
    Shader* defaultShader = nullptr;
    Model* currentModel = nullptr;
};
