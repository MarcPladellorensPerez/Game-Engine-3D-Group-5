#include "ModuleRender.h"
#include "Application.h"
#include "ModuleWindow.h"
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

ModuleRender::ModuleRender() {}
ModuleRender::~ModuleRender() {}

glm::mat4 ModuleRender::GetModelMatrix() const
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, modelPosition);
    model = glm::rotate(model, glm::radians(modelRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(modelRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(modelRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, modelScale);
    return model;
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

bool ModuleRender::LoadModel(const std::string& path)
{
    Model* newModel = new Model(path);
    if (!newModel->IsLoaded())
    {
        delete newModel;
        return false;
    }

    if (currentModel != nullptr)
    {
        delete currentModel;
    }
    currentModel = newModel;

    modelPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    modelRotation = glm::vec3(0.0f, 0.0f, 0.0f);
    modelScale = glm::vec3(1.0f, 1.0f, 1.0f);

    return true;
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

    // Fallback cube vertices (position, normal, UV)
    float vertices[] = {
        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,

        // Left face
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        // Right face
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,

        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f
    };

    // Setup VAO and VBO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Attribute 0: Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Attribute 1: Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Attribute 2: Texture Coords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Load initial 3D Model (Baker House)
    LoadModel("assets/BakerHouse.fbx");

    return true;
}

UpdateStatus ModuleRender::PreUpdate()
{
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

        // Model -> World -> Camera -> Screen transformation matrices
        glm::mat4 model = GetModelMatrix();

        glm::vec3 eye = glm::vec3(8.0f, 6.0f, 12.0f);
        glm::vec3 center = glm::vec3(0.0f, 2.5f, 0.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::mat4 view = glm::lookAt(eye, center, up);

        float aspect = 1.333f;
        if (App && App->window && App->window->GetHeight() > 0)
        {
            aspect = static_cast<float>(App->window->GetWidth()) / static_cast<float>(App->window->GetHeight());
        }
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

        defaultShader->SetMat4("model", model);
        defaultShader->SetMat4("view", view);
        defaultShader->SetMat4("projection", projection);
        defaultShader->SetBool("useLighting", useLighting);

        if (showCheckerTexture)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, checkerTextureID);
            defaultShader->SetInt("texture_diffuse1", 0);
            defaultShader->SetBool("useTexture", true);
        }

        if (currentModel != nullptr && currentModel->IsLoaded())
        {
            if (!showCheckerTexture)
            {
                defaultShader->SetBool("useTexture", true);
            }
            currentModel->Draw(defaultShader->GetProgramID());
        }
        else
        {
            if (!showCheckerTexture)
            {
                defaultShader->SetBool("useTexture", false);
            }
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
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

    if (currentModel != nullptr)
    {
        delete currentModel;
        currentModel = nullptr;
    }

    if (checkerTextureID != 0)
    {
        glDeleteTextures(1, &checkerTextureID);
        checkerTextureID = 0;
    }

    if (vao != 0)
    {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo != 0)
    {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
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
