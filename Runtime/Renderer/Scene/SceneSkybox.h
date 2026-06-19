#pragma once
#include "RHI/RHITexture.h"
#include "Renderer/Scene/SceneObject.h"

struct FSceneSkybox : public FSceneObject
{
    FSceneSkybox(FScene* InScene, const FRHITextureRef& InCubeMap);
    ~FSceneSkybox();

    FRHITextureRef CubeMap;
};