#pragma once
#include "RHI/RHITexture.h"
#include "Renderer/Scene/SceneObject.h"

struct FSkyboxProxyUpdate;

struct FSceneSkybox : public FSceneObject
{
    FSceneSkybox(FScene* InScene);
    ~FSceneSkybox();

    void RenderThread_ApplyUpdate(const FSkyboxProxyUpdate& Update);

    FRHITextureRef CubeMap;
    float          Intensity;
    float          Rotation;
};
