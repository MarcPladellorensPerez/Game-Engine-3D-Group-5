#include "GameObject.h"
#include <glad/glad.h>
#include <algorithm>

GameObject::GameObject(const std::string& name, GameObject* parent)
    : name(name), parent(parent), active(true)
{
    // Guarantee the 3 required components: Transform, Mesh, Texture
    transform = new ComponentTransform(this);
    components.push_back(transform);

    mesh = new ComponentMesh(this);
    components.push_back(mesh);

    texture = new ComponentTexture(this);
    components.push_back(texture);

    if (parent)
    {
        parent->AddChild(this);
    }
}

GameObject::~GameObject()
{
    CleanUp();
}

void GameObject::CleanUp()
{
    for (GameObject* child : children)
    {
        delete child;
    }
    children.clear();

    for (Component* comp : components)
    {
        comp->CleanUp();
        delete comp;
    }
    components.clear();

    transform = nullptr;
    mesh = nullptr;
    texture = nullptr;
}

void GameObject::AddChild(GameObject* child)
{
    if (child && std::find(children.begin(), children.end(), child) == children.end())
    {
        children.push_back(child);
        child->SetParent(this);
    }
}

void GameObject::RemoveChild(GameObject* child)
{
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end())
    {
        children.erase(it);
        child->SetParent(nullptr);
    }
}

Component* GameObject::CreateComponent(ComponentType type)
{
    Component* newComp = nullptr;
    switch (type)
    {
    case ComponentType::TRANSFORM:
        if (!transform)
        {
            transform = new ComponentTransform(this);
            newComp = transform;
        }
        break;
    case ComponentType::MESH:
        if (!mesh)
        {
            mesh = new ComponentMesh(this);
            newComp = mesh;
        }
        break;
    case ComponentType::TEXTURE:
        if (!texture)
        {
            texture = new ComponentTexture(this);
            newComp = texture;
        }
        break;
    }

    if (newComp)
    {
        components.push_back(newComp);
    }
    return newComp;
}

void GameObject::SetTexture(const std::string& texturePath)
{
    if (texture)
    {
        texture->LoadTexture(texturePath);
    }
}

void GameObject::Update()
{
    if (!active) return;

    for (Component* comp : components)
    {
        if (comp && comp->IsActive())
        {
            comp->Update();
        }
    }

    for (GameObject* child : children)
    {
        if (child)
        {
            child->Update();
        }
    }
}

void GameObject::Draw(unsigned int shaderProgram, unsigned int checkerTextureID)
{
    if (!active) return;

    if (transform && mesh && mesh->GetVertexCount() > 0)
    {
        glm::mat4 modelMatrix = transform->GetGlobalMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &modelMatrix[0][0]);

        bool useTexture = false;
        if (texture && texture->showChecker && checkerTextureID != 0)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, checkerTextureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse1"), 0);
            useTexture = true;
        }
        else if (texture && texture->GetTextureID() != 0)
        {
            texture->Bind(0);
            glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse1"), 0);
            useTexture = true;
        }

        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), useTexture ? 1 : 0);

        mesh->Draw(shaderProgram);

        // Draw normals visualization if enabled
        mesh->DrawNormals(shaderProgram, modelMatrix);
    }

    // Draw children recursively
    for (GameObject* child : children)
    {
        if (child)
        {
            child->Draw(shaderProgram, checkerTextureID);
        }
    }
}
