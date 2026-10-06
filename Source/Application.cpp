#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleRender.h"

Application* App = nullptr;

Application::Application()
{
    window = new ModuleWindow();
    renderer = new ModuleRender();

    modules.push_back(window);
    modules.push_back(renderer);
}

Application::~Application()
{
    for (int i = modules.size() - 1; i >= 0; --i)
    {
        delete modules[i];
    }
    modules.clear();
}

bool Application::Init()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        LOG("SDL_Init Error: %s", SDL_GetError());
        return false;
    }

    for (Module* module : modules)
    {
        if (!module->Init()) return false;
    }

    return true;
}

UpdateStatus Application::Update()
{
    UpdateStatus ret = UPDATE_CONTINUE;

    for (Module* module : modules)
    {
        ret = module->PreUpdate();
        if (ret != UPDATE_CONTINUE) return ret;
    }

    for (Module* module : modules)
    {
        ret = module->Update();
        if (ret != UPDATE_CONTINUE) return ret;
    }

    for (Module* module : modules)
    {
        ret = module->PostUpdate();
        if (ret != UPDATE_CONTINUE) return ret;
    }

    return ret;
}

bool Application::CleanUp()
{
    bool ret = true;
    for (int i = modules.size() - 1; i >= 0; --i)
    {
        if (!modules[i]->CleanUp()) ret = false;
    }

    SDL_Quit();
    return ret;
}
