#include "Application.h"
#include "ModuleRender.h"
#include <SDL3/SDL_main.h>
#include <imgui_impl_sdl3.h>

int main(int argc, char* argv[])
{
    App = new Application();

    if (!App->Init())
    {
        delete App;
        return -1;
    }

    UpdateStatus status = UPDATE_CONTINUE;
    bool running = true;

    while (running && status == UPDATE_CONTINUE)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            ImGui_ImplSDL3_ProcessEvent(&e);
            if (e.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            else if (e.type == SDL_EVENT_DROP_FILE)
            {
                if (e.drop.data && App && App->renderer)
                {
                    App->renderer->LoadModel(e.drop.data);
                }
            }
        }

        status = App->Update();
    }

    App->CleanUp();
    delete App;

    return 0;
}
