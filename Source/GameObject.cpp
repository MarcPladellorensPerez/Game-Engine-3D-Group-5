#include "GameObject.h"
#include <glad/glad.h>
#include <algorithm>

GameObject::GameObject(const std::string& name, GameObject* parent)
    : name(name), parent(parent), active(true)
{
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
    if (parent)
    {
        parent->RemoveChild(this);
        parent = nullptr;
    }
    CleanUp();
}

void GameObject::CleanUp()
{
    for (GameObject* child : children)
    {
        if (child)
        {
            child->parent = nullptr;
            delete child;
        }
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

void GameObject::InsertChild(int index, GameObject* child)
{
    if (child && std::find(children.begin(), children.end(), child) == children.end())
    {
        if (index < 0 || index >= static_cast<int>(children.size()))
        {
            children.push_back(child);
        }
        else
        {
            children.insert(children.begin() + index, child);
        }
        child->SetParent(this);
    }
}

int GameObject::GetChildIndex(GameObject* child) const
{
    for (size_t i = 0; i < children.size(); ++i)
    {
        if (children[i] == child) return static_cast<int>(i);
    }
    return -1;
}

GameObject* GameObject::Clone(GameObject* newParent, bool appendCopySuffix)
{
    std::string cloneName = name;
    if (appendCopySuffix)
    {
        cloneName += " (Copy)";
    }

    GameObject* copy = new GameObject(cloneName, newParent);
    copy->active = this->active;

    if (this->transform && copy->transform)
    {
        copy->transform->position = this->transform->position;
        copy->transform->rotation = this->transform->rotation;
        copy->transform->scale = this->transform->scale;
    }

    if (this->mesh && copy->mesh)
    {
        copy->mesh->meshName = this->mesh->meshName;
        copy->mesh->showNormalsPerFace = this->mesh->showNormalsPerFace;
        copy->mesh->showNormalsPerVertex = this->mesh->showNormalsPerVertex;
        if (this->mesh->GetVertexCount() > 0)
        {
            copy->mesh->InitBuffers(this->mesh->GetVertices(), this->mesh->GetIndices());
        }
    }

    if (this->texture && copy->texture)
    {
        copy->texture->showChecker = this->texture->showChecker;
        if (this->texture->GetPath() != "None" && !this->texture->GetPath().empty())
        {
            copy->texture->LoadTexture(this->texture->GetPath());
        }
        else if (this->texture->GetTextureID() != 0)
        {
            copy->texture->SetTexture(this->texture->GetTextureID(), this->texture->GetPath(),
                                     this->texture->GetWidth(), this->texture->GetHeight());
        }
    }

    for (GameObject* child : children)
    {
        if (child)
        {
            child->Clone(copy, false);
        }
    }

    return copy;
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
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.75f, 0.75f, 0.75f);

        mesh->Draw(shaderProgram);
        mesh->DrawNormals(shaderProgram, modelMatrix);
    }

    for (GameObject* child : children)
    {
        if (child)
        {
            child->Draw(shaderProgram, checkerTextureID);
        }
    }
}
