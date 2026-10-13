#pragma once
#include "Module.h"

struct SDL_Window;

class ModuleWindow : public Module
{
public:
    ModuleWindow();
    virtual ~ModuleWindow();

    bool Init() override;
    bool CleanUp() override;

    SDL_Window* GetWindow() const { return window; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

    void SetFullscreen(bool enable);
    void SetResizable(bool enable);
    void SetSize(int w, int h);
    bool IsFullscreen() const { return fullscreen; }
    bool IsResizable() const { return resizable; }

private:
    SDL_Window* window = nullptr;
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool resizable = true;
};
