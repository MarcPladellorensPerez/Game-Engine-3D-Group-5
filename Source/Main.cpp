#include "Application.h"
#include <SDL3/SDL_main.h>

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
            if (e.type == SDL_EVENT_QUIT)
                running = false;
        }

        status = App->Update();
    }

    App->CleanUp();
    delete App;

    return 0;
}
