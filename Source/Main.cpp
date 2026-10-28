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
                    std::string droppedPath = e.drop.data;
                    size_t dotPos = droppedPath.find_last_of('.');
                    std::string ext = "";
                    if (dotPos != std::string::npos)
                    {
                        ext = droppedPath.substr(dotPos + 1);
                        for (auto& c : ext) c = static_cast<char>(tolower(c));
                    }

                    if (ext == "fbx" || ext == "obj" || ext == "dae" || ext == "gltf" || ext == "glb")
                    {
                        LOG("[DRAG&DROP] 3D Model: %s", droppedPath.c_str());
                        App->renderer->LoadModel(droppedPath);
                    }
                    else if (ext == "dds" || ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "tga" || ext == "bmp")
                    {
                        LOG("[DRAG&DROP] Texture: %s", droppedPath.c_str());
                        App->renderer->ApplyTextureToSelected(droppedPath);
                    }
                    else
                    {
                        LOG("[DRAG&DROP] Unsupported file type: %s", droppedPath.c_str());
                    }
                }
            }
        }

        status = App->Update();
    }

    App->CleanUp();
    delete App;

    return 0;
}
