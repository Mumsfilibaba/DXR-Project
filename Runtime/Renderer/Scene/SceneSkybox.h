#pragma once
#include "Engine/World/Components/SkyboxComponent.h"
#include "Renderer/Scene/SceneObject.h"

class FSceneSkybox : public FSceneObject
{
public:
    FSceneSkybox(FScene* InScene, FSkyboxComponent* InSkybox);
    ~FSceneSkybox();

    // FSceneObject Interface
    virtual void Tick() override final { }

    FRHITextureRef GetCubeMap() const { return CubeMap; }

private:

    // Pointer to the light in the world
    FSkyboxComponent* Skybox;

    // Cube-map
    FRHITextureRef CubeMap;
};