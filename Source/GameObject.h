#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Component.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentTexture.h"

class GameObject {
public:
    GameObject(const std::string& name = "GameObject", GameObject* parent = nullptr);
    virtual ~GameObject();

    void Update();
    void Draw(unsigned int shaderProgram, unsigned int checkerTextureID = 0);
    void CleanUp();

    Component* CreateComponent(ComponentType type);
    template<typename T> T* GetComponent();

    void AddChild(GameObject* child);
    void RemoveChild(GameObject* child);
    void InsertChild(int index, GameObject* child);
    int GetChildIndex(GameObject* child) const;

    GameObject* Clone(GameObject* newParent = nullptr, bool appendCopySuffix = true);

    void SetTexture(const std::string& texturePath);

    const std::string& GetName() const { return name; }
    void SetName(const std::string& n) { name = n; }

    bool IsActive() const { return active; }
    void SetActive(bool act) { active = act; }

    GameObject* GetParent() const { return parent; }
    void SetParent(GameObject* p) { parent = p; }

    const std::vector<GameObject*>& GetChildren() const { return children; }
    const std::vector<Component*>& GetComponents() const { return components; }

    ComponentTransform* transform = nullptr;
    ComponentMesh* mesh = nullptr;
    ComponentTexture* texture = nullptr;

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
        T* casted = dynamic_cast<T*>(comp);
        if (casted) return casted;
    }
    return nullptr;
}
