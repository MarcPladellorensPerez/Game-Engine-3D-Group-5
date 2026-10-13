#pragma once
#include "Module.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>

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

public:
    SDL_GLContext GetContext() const { return context; }

    float clearColor[4] = { 0.1f, 0.25f, 0.5f, 1.0f };
    bool wireframe = false;
    bool depthTest = true;
    bool cullFace = false;
    bool vsync = true;

private:
    SDL_GLContext context;
    GLuint vBuff;
};
