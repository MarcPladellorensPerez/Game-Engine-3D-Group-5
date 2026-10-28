#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleCamera.h"
#include "ModuleRender.h"
#include "ModuleEditor.h"
#include <vector>
#include <string>
#include <cstdio>
#include <cstdarg>

Application* App = nullptr;
static std::vector<std::string> pendingLogs;

void Log(const char* format, ...)
{
    static char tmp_string[4096];
    va_list ap;
    va_start(ap, format);
    vsnprintf(tmp_string, sizeof(tmp_string), format, ap);
    va_end(ap);

    SDL_Log("%s", tmp_string);

    if (App && App->editor)
    {
        for (const auto& pending : pendingLogs)
        {
            App->editor->AddLog("%s\n", pending.c_str());
        }
        pendingLogs.clear();

        App->editor->AddLog("%s\n", tmp_string);
    }
    else
    {
        pendingLogs.push_back(tmp_string);
    }
}

Application::Application()
{
    window = new ModuleWindow();
    camera = new ModuleCamera();
    renderer = new ModuleRender();
    editor = new ModuleEditor();

    modules.push_back(window);
    modules.push_back(camera);
    modules.push_back(renderer);
    modules.push_back(editor);
}

Application::~Application()
{
    for (int i = static_cast<int>(modules.size()) - 1; i >= 0; --i)
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
    for (int i = static_cast<int>(modules.size()) - 1; i >= 0; --i)
    {
        if (!modules[i]->CleanUp()) ret = false;
    }

    SDL_Quit();
    return ret;
}
