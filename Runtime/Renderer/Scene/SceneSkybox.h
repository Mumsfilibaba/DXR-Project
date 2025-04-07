#pragma once
#include "Engine/World/Components/SkyboxComponent.h"
#include "Renderer/Scene/SceneObject.h"

struct FSceneSkybox : public FSceneObject
{
    FSceneSkybox(FScene* InScene, FSkyboxComponent* InSkybox);
    ~FSceneSkybox();

    virtual void Tick() override final { }

    // Pointer to the light in the world
    FSkyboxComponent* Skybox;

    // Cube-maps
    FRHITextureRef CubeMap;
};