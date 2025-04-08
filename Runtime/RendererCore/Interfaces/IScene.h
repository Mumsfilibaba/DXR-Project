#pragma once

class FCamera;
class FLight;
class FStaticMeshComponent;
class FSkyboxComponent;
class FLightProbe;

struct IScene
{
    virtual ~IScene() = default;

    // Update the scene this frame
    virtual void Tick() = 0;

    // Adds a Renderer version of a camera
    virtual void AddCamera(FCamera* InCamera) = 0;

    // Adds a light to the scene
    virtual void AddLight(FLight* InLight) = 0;

    // Adds a light-probe to the scene
    virtual void AddLightProbe(FLightProbe* InLightProbe) = 0;

    // Adds a Skybox to the light
    virtual void AddSkybox(FSkyboxComponent* InSkyboxComponent) = 0;

    // Add a static Mesh
    virtual void AddStaticMesh(FStaticMeshComponent* InMeshComponent) = 0;
};