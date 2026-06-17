#pragma once
#include "Engine/World/Reflections/LightProbe.h"
#include "Renderer/Scene/SceneObject.h"

struct FSceneLightProbe : public FSceneObject
{
    FSceneLightProbe(FScene* InScene, FLightProbe* InLightProbe);
    ~FSceneLightProbe();

    virtual void Tick() override final;

    // Filters the source into the necessary cube-maps
    void FilterStaticCubeMaps();

    FLightProbe*   LightProbe;      // Pointer to the light in the world    
    FRHITextureRef SourceCubeMap;   // Source cube-map
    FRHITextureRef SpecularCubeMap; // Cube-maps for specular reflections
    FRHITextureRef DiffuseCubeMap;  // Cube-maps for diffuse reflections
    Vector3        Origin;          // Position of the probe in world space
    Vector3        BoxMin;          // Minimum bounds of the probe's box
    Vector3        BoxMax;          // Maximum bounds of the probe's box
    bool           bBoxProjection;  // Use box-projection
};