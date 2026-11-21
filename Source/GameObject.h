#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Component.h"

// Forward declarations
class ComponentTransform;
class ComponentMesh;
class ComponentTexture;
class GameObject;

class GameObject {
public:
    GameObject(const std::string& name = "GameObject", GameObject* parent = nullptr);
    virtual ~GameObject();

    void Update();
    void CleanUp();

    Component* CreateComponent(ComponentType type);
    void RemoveComponent(Component* comp);
    void RemoveComponent(ComponentType type);

    template<typename T> T* GetComponent();
    template<typename T> const T* GetComponent() const;

    ComponentTransform* GetTransform();
    const ComponentTransform* GetTransform() const;

    void AddChild(GameObject* child);
    void RemoveChild(GameObject* child);
    void InsertChild(int index, GameObject* child);
    int GetChildIndex(GameObject* child) const;

    GameObject* Clone(GameObject* newParent = nullptr, bool appendCopySuffix = true);

    const std::string& GetName() const { return name; }
    void SetName(const std::string& n) { name = n; }

    bool IsActive() const { return active; }
    void SetActive(bool act) { active = act; }

    GameObject* GetParent() const { return parent; }
    void SetParent(GameObject* p) { parent = p; }

    const std::vector<GameObject*>& GetChildren() const { return children; }
    const std::vector<Component*>& GetComponents() const { return components; }

private:
    std::string name;
    bool active = true;
    GameObject* parent = nullptr;
    std::vector<GameObject*> children;
    std::vector<Component*> components;
};

template<typename T>
T* GameObject::GetComponent()
{
    for (Component* comp : components)
    {
        if (comp && comp->GetType() == T::StaticType)
            return static_cast<T*>(comp);
    }
    return nullptr;
}

template<typename T>
const T* GameObject::GetComponent() const
{
    for (const Component* comp : components)
    {
        if (comp && comp->GetType() == T::StaticType)
            return static_cast<const T*>(comp);
    }
    return nullptr;
}
