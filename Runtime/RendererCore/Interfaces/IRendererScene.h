#pragma once

class FCamera;
class FLight;
class FProxyRendererComponent;

struct IRendererScene
{
    virtual ~IRendererScene() = default;

    // Adds a Renderer version of a camera
    virtual void AddCamera(FCamera* InCamera) = 0;

    // Adds a light to the scene
    virtual void AddLight(FLight* InLight) = 0;

    // Add a Renderer version of a component
    virtual void AddProxyComponent(FProxyRendererComponent* InComponent) = 0;
};