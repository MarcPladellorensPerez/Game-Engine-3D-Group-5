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

private:
    SDL_GLContext context;
    GLuint vBuff;
};
