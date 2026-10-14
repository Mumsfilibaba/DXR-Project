#pragma once
#include "Core/Math/Vector3.h"
#include "RHI/RHITexture.h"
#include "Renderer/Scene/SceneObject.h"

struct FSkyLightProxyUpdate;

struct FSceneSkyLight : public FSceneObject
{
    FSceneSkyLight(FScene* InScene);
    ~FSceneSkyLight();

    void RenderThread_ApplyUpdate(const FSkyLightProxyUpdate& Update);

    // The prebaked cubes of the environment it lights with, null while it has none
    FRHITextureRef SpecularCubeMap;
    FRHITextureRef DiffuseCubeMap;

    FVector3 Color;
    float    Rotation;
};
