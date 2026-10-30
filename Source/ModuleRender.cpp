#include "ModuleRender.h"
#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleCamera.h"
#include "Model.h"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>

// Fallback shader sources
static const char* fallbackVertexShader = R"(#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model = mat4(1.0);
uniform mat4 view = mat4(1.0);
uniform mat4 projection = mat4(1.0);

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

static const char* fallbackFragmentShader = R"(#version 460 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform bool useTexture = false;
uniform bool useLighting = true;
uniform vec3 objectColor = vec3(0.8, 0.8, 0.8);
uniform vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));

void main()
{
    vec4 baseColor = useTexture ? texture(texture_diffuse1, TexCoords) : vec4(objectColor, 1.0);

    if (useLighting)
    {
        vec3 norm = normalize(Normal);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 ambient = 0.4 * vec3(1.0);
        vec3 diffuse = diff * vec3(0.8);
        vec3 result = (ambient + diffuse) * baseColor.rgb;
        FragColor = vec4(result, baseColor.a);
    }
    else
    {
        FragColor = baseColor;
    }
}
)";

#include "GameObject.h"

ModuleRender::ModuleRender() {}
ModuleRender::~ModuleRender() {}

bool ModuleRender::IsDescendantOf(GameObject* candidate, GameObject* potentialAncestor) const
{
    if (!candidate || !potentialAncestor) return false;
    GameObject* curr = candidate->GetParent();
    while (curr)
    {
        if (curr == potentialAncestor) return true;
        curr = curr->GetParent();
    }
    return false;
}

int ModuleRender::GetRootGameObjectIndex(GameObject* go) const
{
    for (size_t i = 0; i < gameObjects.size(); ++i)
    {
        if (gameObjects[i] == go) return static_cast<int>(i);
    }
    return -1;
}

void ModuleRender::AddGameObject(GameObject* go, GameObject* parent, int index)
{
    if (!go) return;

    if (parent)
    {
        parent->InsertChild(index, go);
    }
    else
    {
        go->SetParent(nullptr);
        if (std::find(gameObjects.begin(), gameObjects.end(), go) == gameObjects.end())
        {
            if (index >= 0 && index <= static_cast<int>(gameObjects.size()))
            {
                gameObjects.insert(gameObjects.begin() + index, go);
            }
            else
            {
                gameObjects.push_back(go);
            }
        }
    }
}

void ModuleRender::DetachGameObject(GameObject* go)
{
    if (!go) return;

    if (selectedGameObject == go || IsDescendantOf(selectedGameObject, go))
    {
        selectedGameObject = nullptr;
    }

    if (go->GetParent())
    {
        go->GetParent()->RemoveChild(go);
    }
    else
    {
        auto it = std::find(gameObjects.begin(), gameObjects.end(), go);
        if (it != gameObjects.end())
        {
            gameObjects.erase(it);
        }
    }
}

void ModuleRender::DeleteGameObject(GameObject* go)
{
    if (!go) return;
    DetachGameObject(go);
    delete go;
}

GameObject* ModuleRender::CreateGameObject(const std::string& name, GameObject* parent)
{
    GameObject* go = new GameObject(name, parent);
    if (!parent)
    {
        gameObjects.push_back(go);
    }
    selectedGameObject = go;
    return go;
}

GameObject* ModuleRender::CreatePrimitive(const std::string& type, GameObject* parent)
{
    std::string name = type + " " + std::to_string(gameObjects.size() + 1);
    GameObject* go = new GameObject(name, parent);
    if (type == "Cube") go->mesh->CreateCube();
    else if (type == "Plane" || type == "Plane / Quad") go->mesh->CreatePlane();
    else if (type == "Sphere") go->mesh->CreateSphere();
    else if (type == "Cylinder") go->mesh->CreateCylinder();
    else if (type == "Pyramid") go->mesh->CreatePyramid();
    else if (type == "Triangle") go->mesh->CreateTriangle();

    if (!parent)
    {
        gameObjects.push_back(go);
    }
    selectedGameObject = go;
    LOG("Created basic geometry: %s", type.c_str());
    return go;
}

GameObject* ModuleRender::LoadModel(const std::string& path)
{
    GameObject* newObj = Model::LoadToGameObject(path);
    if (!newObj)
    {
        return nullptr;
    }

    gameObjects.push_back(newObj);
    selectedGameObject = newObj;

    return newObj;
}

void ModuleRender::ApplyTextureToSelected(const std::string& texturePath)
{
    GameObject* target = selectedGameObject;
    if (!target && !gameObjects.empty())
    {
        target = gameObjects.back();
    }

    if (!target)
    {
        LOG("[WARNING] Cannot apply texture: No GameObject in scene.");
        return;
    }

    target->SetTexture(texturePath);
    for (GameObject* child : target->GetChildren())
    {
        child->SetTexture(texturePath);
    }
    LOG("Applied texture '%s' to GameObject '%s'", texturePath.c_str(), target->GetName().c_str());
}

bool ModuleRender::Init()
{
    LOG("Creating 3D Renderer context");

    context = SDL_GL_CreateContext(App->window->GetWindow());
    if (!context) return false;

    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)) == 0) return false;

    glViewport(0, 0, App->window->GetWidth(), App->window->GetHeight());
    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    glEnable(GL_DEPTH_TEST);

    // Initialize default shader
    defaultShader = new Shader();
    if (!defaultShader->LoadFromFile("assets/shaders/default.vert", "assets/shaders/default.frag"))
    {
        LOG("Loading fallback shader from memory");
        if (!defaultShader->LoadFromSource(fallbackVertexShader, fallbackFragmentShader))
        {
            LOG("Failed to compile default shader");
            return false;
        }
    }

    CreateCheckerTexture();

    // Baker_house loaded automatically at startup
    LoadModel("assets/BakerHouse.fbx");

    return true;
}

void ModuleRender::CreateCheckerTexture()
{
    const int checkSize = 64;
    unsigned char checkerImage[checkSize][checkSize][4];
    for (int i = 0; i < checkSize; i++)
    {
        for (int j = 0; j < checkSize; j++)
        {
            int c = ((((i & 0x8) == 0) ^ ((j & 0x8) == 0))) * 255;
            checkerImage[i][j][0] = (unsigned char)c;
            checkerImage[i][j][1] = (unsigned char)c;
            checkerImage[i][j][2] = (unsigned char)c;
            checkerImage[i][j][3] = (unsigned char)255;
        }
    }

    glGenTextures(1, &checkerTextureID);
    glBindTexture(GL_TEXTURE_2D, checkerTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, checkSize, checkSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerImage);
    glBindTexture(GL_TEXTURE_2D, 0);
}

UpdateStatus ModuleRender::PreUpdate()
{
    if (App && App->window)
    {
        glViewport(0, 0, App->window->GetWidth(), App->window->GetHeight());
    }

    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (depthTest)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    if (cullFace)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

    return UPDATE_CONTINUE;
}

UpdateStatus ModuleRender::Update()
{
    if (defaultShader != nullptr)
    {
        defaultShader->Use();

        // Camera View & Projection transformation matrices
        glm::mat4 view = (App && App->camera) ? App->camera->GetViewMatrix() : glm::mat4(1.0f);
        glm::mat4 projection = (App && App->camera) ? App->camera->GetProjectionMatrix() : glm::mat4(1.0f);

        defaultShader->SetMat4("view", view);
        defaultShader->SetMat4("projection", projection);
        defaultShader->SetBool("useLighting", useLighting);

        // Render all GameObjects in scene
        for (GameObject* go : gameObjects)
        {
            if (go)
            {
                go->Draw(defaultShader->GetProgramID(), showCheckerTexture ? checkerTextureID : 0);
            }
        }

        defaultShader->Unuse();
    }

    return UPDATE_CONTINUE;
}

UpdateStatus ModuleRender::PostUpdate()
{
    return UPDATE_CONTINUE;
}

bool ModuleRender::CleanUp()
{
    LOG("Destroying 3D Renderer");

    for (GameObject* go : gameObjects)
    {
        if (go) delete go;
    }
    gameObjects.clear();
    selectedGameObject = nullptr;

    if (checkerTextureID != 0)
    {
        glDeleteTextures(1, &checkerTextureID);
        checkerTextureID = 0;
    }

    if (defaultShader != nullptr)
    {
        defaultShader->CleanUp();
        delete defaultShader;
        defaultShader = nullptr;
    }

    if (context != nullptr)
    {
        SDL_GL_DestroyContext(context);
        context = nullptr;
    }

    return true;
}
