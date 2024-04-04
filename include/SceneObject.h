#ifndef SCENE_OBJECT_H
#define SCENE_OBJECT_H
#include <vector>
#include <Component.h>
#include <memory>


class SceneObject {
public:
    void addComponent(std::shared_ptr<IComponent> component) {
        components.push_back(component);
    }

    void Tick() {
        if (!active) return;
        for (auto& component : components) {
            component->Tick();
        }
    }

    virtual void Active() {
        this->active = true;
        for (auto& component : components) {
            component->active = true;
        }
    }

    virtual void Deactive() {
        this->active = false;
        for (auto& component : components) {
            component->active = false;
        }
    }

    bool isActive() {
        return active;
    }

protected:
    std::vector<std::shared_ptr<IComponent>> components;
    bool active = true;
};

class RenderableSceneObject : public SceneObject {
public:

    void attachToSceneRenderList(std::vector<std::shared_ptr<RenderComponent>>& renderQueue) {
        renderQueue.push_back(this->renderComponent);
    }
    void detachFromSceneRenderList(std::vector<std::shared_ptr<RenderComponent>>& renderQueue) {
        auto it = std::find(renderQueue.begin(), renderQueue.end(), this->renderComponent);
        if (it != renderQueue.end()) {
            renderQueue.erase(it);
        }
    }

    void Active() override {
        SceneObject::Active();
        this->renderComponent->active = true;
    }

    void Deactive() override {
        SceneObject::Deactive();
        this->renderComponent->active = false;
    }

protected:
    std::shared_ptr<RenderComponent> renderComponent;
};
















#endif