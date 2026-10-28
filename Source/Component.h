#pragma once
#include <string>

class GameObject;

enum class ComponentType {
    TRANSFORM = 0,
    MESH,
    TEXTURE
};

class Component {
public:
    Component(GameObject* owner, ComponentType type)
        : owner(owner), type(type), active(true) {}
    virtual ~Component() {}

    virtual void Update() {}
    virtual void CleanUp() {}

    GameObject* GetOwner() const { return owner; }
    ComponentType GetType() const { return type; }
    bool IsActive() const { return active; }
    void SetActive(bool act) { active = act; }

protected:
    GameObject* owner = nullptr;
    ComponentType type = ComponentType::TRANSFORM;
    bool active = true;
};
