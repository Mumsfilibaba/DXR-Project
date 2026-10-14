#pragma once
#include "RHI/RHITexture.h"
#include "Core/Math/Vector3.h"
#include "Renderer/Scene/SceneObject.h"

struct FLightProbeProxyUpdate;

struct FSceneLightProbe : public FSceneObject
{
    FSceneLightProbe(FScene* InScene);
    ~FSceneLightProbe();

    // Applies a per-frame probe snapshot (environment, position and box-projection bounds).
    void RenderThread_ApplyUpdate(const FLightProbeProxyUpdate& Update);

    FRHITextureRef SpecularCubeMap; 
    FRHITextureRef DiffuseCubeMap;  
    FVector3       Origin;
    FVector3       BoxMin;          
    FVector3       BoxMax;          
    float          Intensity;
    float          Rotation;
    bool           bBoxProjection;  
};