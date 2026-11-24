#include "GameObject.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"
#include <glad/glad.h>
#include <algorithm>

GameObject::GameObject(const std::string& name, GameObject* parent)
    : name(name), parent(parent), active(true)
{
    // Empty GameObjects only have Transform by default.
    // Mesh and Texture components are added on demand.
    ComponentTransform* transform = new ComponentTransform(this);
    components.push_back(transform);

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
    if (components.empty() && children.empty()) return; // Guard against double CleanUp

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
        if (comp)
        {
            comp->CleanUp();
            delete comp;
        }
    }
    components.clear();
}

ComponentTransform* GameObject::GetTransform()
{
    return GetComponent<ComponentTransform>();
}

const ComponentTransform* GameObject::GetTransform() const
{
    return GetComponent<ComponentTransform>();
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

GameObject* GameObject::Clone(GameObject* newParent, bool appendCopySuffix, bool acquireResources)
{
    std::string cloneName = name;
    if (appendCopySuffix)
    {
        cloneName += " (Copy)";
    }

    GameObject* copy = new GameObject(cloneName, newParent);
    copy->active = this->active;

    ComponentTransform* srcTransform = GetComponent<ComponentTransform>();
    ComponentTransform* copyTransform = copy->GetComponent<ComponentTransform>();
    if (srcTransform && copyTransform)
    {
        copyTransform->position = srcTransform->position;
        copyTransform->rotation = srcTransform->rotation;
        copyTransform->scale = srcTransform->scale;
    }

    ComponentMesh* srcMesh = GetComponent<ComponentMesh>();
    if (srcMesh)
    {
        ComponentMesh* copyMesh = static_cast<ComponentMesh*>(copy->CreateComponent(ComponentType::MESH));
        if (copyMesh)
        {
            copyMesh->meshName = srcMesh->meshName;
            copyMesh->showNormalsPerFace = srcMesh->showNormalsPerFace;
            copyMesh->showNormalsPerVertex = srcMesh->showNormalsPerVertex;
            if (srcMesh->GetMeshUID() != 0)
            {
                if (acquireResources)
                {
                    copyMesh->SetResourceMesh(srcMesh->GetMeshUID());
                }
                else
                {
                    copyMesh->SetDeferredResourceUID(srcMesh->GetMeshUID(), srcMesh->meshName);
                }
            }
            else if (srcMesh->GetVertexCount() > 0)
            {
                copyMesh->InitBuffers(srcMesh->GetVertices(), srcMesh->GetIndices());
            }
        }
    }

    ComponentTexture* srcTexture = GetComponent<ComponentTexture>();
    if (srcTexture)
    {
        ComponentTexture* copyTexture = static_cast<ComponentTexture*>(copy->CreateComponent(ComponentType::TEXTURE));
        if (copyTexture)
        {
            copyTexture->showChecker = srcTexture->showChecker;
            if (srcTexture->GetTextureUID() != 0)
            {
                if (acquireResources)
                {
                    copyTexture->SetResourceTexture(srcTexture->GetTextureUID());
                }
                else
                {
                    copyTexture->SetDeferredResourceUID(srcTexture->GetTextureUID(), srcTexture->GetPath(),
                                                        srcTexture->GetWidth(), srcTexture->GetHeight(), srcTexture->GetChannels());
                }
            }
            else if (srcTexture->GetPath() != "None" && !srcTexture->GetPath().empty())
            {
                copyTexture->LoadTexture(srcTexture->GetPath());
            }
            else if (srcTexture->GetTextureID() != 0)
            {
                copyTexture->SetTexture(srcTexture->GetTextureID(), srcTexture->GetPath(),
                                        srcTexture->GetWidth(), srcTexture->GetHeight());
            }
        }
    }

    for (GameObject* child : children)
    {
        if (child)
        {
            child->Clone(copy, false, acquireResources);
        }
    }

    return copy;
}

void GameObject::ReleaseResourceReferences()
{
    if (ComponentMesh* mesh = GetComponent<ComponentMesh>())
    {
        mesh->ReleaseResourceReference();
    }
    if (ComponentTexture* texture = GetComponent<ComponentTexture>())
    {
        texture->ReleaseResourceReference();
    }
    for (GameObject* child : children)
    {
        if (child)
        {
            child->ReleaseResourceReferences();
        }
    }
}

void GameObject::AcquireResourceReferences()
{
    if (ComponentMesh* mesh = GetComponent<ComponentMesh>())
    {
        mesh->AcquireResourceReference();
    }
    if (ComponentTexture* texture = GetComponent<ComponentTexture>())
    {
        texture->AcquireResourceReference();
    }
    for (GameObject* child : children)
    {
        if (child)
        {
            child->AcquireResourceReferences();
        }
    }
}

Component* GameObject::CreateComponent(ComponentType type)
{
    Component* newComp = nullptr;
    switch (type)
    {
    case ComponentType::TRANSFORM:
        if (!GetComponent<ComponentTransform>())
        {
            newComp = new ComponentTransform(this);
        }
        break;
    case ComponentType::MESH:
        if (!GetComponent<ComponentMesh>())
        {
            newComp = new ComponentMesh(this);
        }
        break;
    case ComponentType::TEXTURE:
        if (!GetComponent<ComponentTexture>())
        {
            newComp = new ComponentTexture(this);
        }
        break;
    }

    if (newComp)
    {
        components.push_back(newComp);
    }
    return newComp;
}

void GameObject::RemoveComponent(Component* comp)
{
    if (!comp) return;

    // Do not allow removing the root transform component
    if (comp->GetType() == ComponentType::TRANSFORM) return;

    auto it = std::find(components.begin(), components.end(), comp);
    if (it != components.end())
    {
        comp->CleanUp();
        delete comp;
        components.erase(it);
    }
}

void GameObject::RemoveComponent(ComponentType type)
{
    if (type == ComponentType::TRANSFORM) return;

    for (auto it = components.begin(); it != components.end(); ++it)
    {
        if ((*it) && (*it)->GetType() == type)
        {
            (*it)->CleanUp();
            delete *it;
            components.erase(it);
            return;
        }
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
