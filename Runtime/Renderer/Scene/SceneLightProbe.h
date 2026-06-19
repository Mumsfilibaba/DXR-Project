#pragma once
#include "RHI/RHITexture.h"
#include "Core/Math/Vector3.h"
#include "Renderer/Scene/SceneObject.h"

struct FLightProbeProxyUpdate;

struct FSceneLightProbe : public FSceneObject
{
    FSceneLightProbe(FScene* InScene, const FRHITextureRef& InSourceCubeMap);
    ~FSceneLightProbe();

    // Applies a per-frame probe snapshot (position + box-projection bounds).
    void RenderThread_ApplyUpdate(const FLightProbeProxyUpdate& Update);

    // Filters the source into the necessary cube-maps
    void RenderThread_FilterStaticCubeMaps();

    FRHITextureRef SourceCubeMap;   
    FRHITextureRef SpecularCubeMap; 
    FRHITextureRef DiffuseCubeMap;  
    Vector3        Origin;
    Vector3        BoxMin;          
    Vector3        BoxMax;          
    bool           bBoxProjection;  
};