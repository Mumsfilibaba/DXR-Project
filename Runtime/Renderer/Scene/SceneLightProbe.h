#pragma once
#include "Engine/World/Reflections/LightProbe.h"
#include "RendererCore/Interfaces/ISceneObject.h"

struct FSceneLightProbe : public ISceneObject
{
    FSceneLightProbe(FLightProbe* InLightProbe);
    ~FSceneLightProbe();

    virtual void Tick() override final;

    // Filters the source into the necessary cube-maps
    void FilterStaticCubeMaps();

    // Pointer to the light in the world
    FLightProbe* LightProbe;

    // Source cube-map
    FRHITextureRef SourceCubeMap;

    // Cube-maps
    FRHITextureRef SpecularCubeMap;
    FRHITextureRef DiffuseCubeMap;

    // Position
    FVector3 Origin;

    // Bounds 
    FVector3 BoxMin;
    FVector3 BoxMax;

    // Use box-projection
    bool bBoxProjection;
};