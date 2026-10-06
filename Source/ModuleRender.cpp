#include "ModuleRender.h"
#include "Application.h"
#include "ModuleWindow.h"

ModuleRender::ModuleRender() {}
ModuleRender::~ModuleRender() {}

bool ModuleRender::Init()
{
    LOG("Creating 3D Renderer context");

    context = SDL_GL_CreateContext(App->window->GetWindow());
    if (!context) return false;

    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)) == 0) return false;

    glViewport(0, 0, App->window->GetWidth(), App->window->GetHeight());
    glClearColor(0.1f, 0.25f, 0.5f, 1.0f);

    float vertices[] = {
        -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 
         0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 
         0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 
        -0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f  
    };
    
    glGenBuffers(1, &vBuff);
    glBindBuffer(GL_ARRAY_BUFFER, vBuff);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    return true;
}

UpdateStatus ModuleRender::PreUpdate()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return UPDATE_CONTINUE;
}

UpdateStatus ModuleRender::Update()
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, vBuff);
    glVertexPointer(3, GL_FLOAT, sizeof(float) * 6, NULL);
    glColorPointer(3, GL_FLOAT, sizeof(float) * 6, reinterpret_cast<void*>(sizeof(float) * 3));

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    return UPDATE_CONTINUE;
}

UpdateStatus ModuleRender::PostUpdate()
{
    SDL_GL_SwapWindow(App->window->GetWindow());
    return UPDATE_CONTINUE;
}

bool ModuleRender::CleanUp()
{
    LOG("Destroying 3D Renderer");
    glDeleteBuffers(1, &vBuff);
    SDL_GL_DestroyContext(context);
    return true;
}
