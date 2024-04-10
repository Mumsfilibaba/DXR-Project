#pragma once

class FCamera;
class FLight;
class FProxySceneComponent;

struct IScene
{
    virtual ~IScene() = default;

    // Update the scene this frame
    virtual void Tick() = 0;

    // Adds a Renderer version of a camera
    virtual void AddCamera(FCamera* InCamera) = 0;

    // Adds a light to the scene
    virtual void AddLight(FLight* InLight) = 0;

    // Add a Renderer version of a component
    virtual void AddProxyComponent(FProxySceneComponent* InComponent) = 0;
};