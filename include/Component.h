#ifndef COMPONENT_H
#define COMPONENT_H


class IComponent {
public:
    virtual ~IComponent() = default;
    virtual void Tick() = 0;
    bool active = true;
};

enum renderQueue {
    OPAQUE = 0,
    SKYBOX = 1,
    TRANSPARENT = 2
    
};

class RenderComponent : public IComponent {
public:
    virtual void Render() = 0;
    virtual void Tick() override {
        return;
    }
    // render priority, higher value means it will be rendered later
    // default value is 0, 1 for skybox, 2 for transparent objects
    renderQueue renderPriority = OPAQUE; 
};


















#endif

