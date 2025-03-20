#pragma once
#include "Engine/World/Components/SkyboxComponent.h"
#include "RendererCore/Interfaces/ISceneObject.h"

struct FSceneSkybox : public ISceneObject
{
    FSceneSkybox(FSkyboxComponent* InSkybox);
    ~FSceneSkybox();

    virtual void Tick() override final { }

    // Pointer to the light in the world
    FSkyboxComponent* Skybox;

    // Cube-maps
    FRHITextureRef CubeMap;
};