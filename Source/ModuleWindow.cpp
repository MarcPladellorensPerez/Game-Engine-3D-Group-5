#include "ModuleWindow.h"
#include <SDL3/SDL.h>

ModuleWindow::ModuleWindow() {}
ModuleWindow::~ModuleWindow() {}

bool ModuleWindow::Init()
{
    LOG("Init SDL Window");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow("MotorVJ - Entrega 1", width, height, flags);

    if (window == nullptr)
    {
        LOG("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

bool ModuleWindow::CleanUp()
{
    LOG("Destroying SDL Window");
    if (window != nullptr)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    return true;
}

void ModuleWindow::SetFullscreen(bool enable)
{
    fullscreen = enable;
    if (window != nullptr)
    {
        SDL_SetWindowFullscreen(window, fullscreen);
    }
}

void ModuleWindow::SetResizable(bool enable)
{
    resizable = enable;
    if (window != nullptr)
    {
        SDL_SetWindowResizable(window, resizable);
    }
}

void ModuleWindow::SetSize(int w, int h)
{
    width = w;
    height = h;
    if (window != nullptr)
    {
        SDL_SetWindowSize(window, width, height);
    }
}
