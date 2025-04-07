#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Frustum.h"
#include "Renderer/Scene/SceneObject.h"

class FSkyLight;

struct FSceneSkyLight : public FSceneObject
{
    FSceneSkyLight(FScene* InScene, FSkyLight* InSkyLight);
    ~FSceneSkyLight();

    virtual void Tick() override final { }

    // Filters the source into the necessary cube-maps
    void FilterStaticCubeMaps();

    // Pointer to the light in the world
    FSkyLight* SkyLight;

    // Source cube-map
    FRHITextureRef SourceCubeMap;

    // Cube-maps
    FRHITextureRef SpecularCubeMap;
    FRHITextureRef DiffuseCubeMap;
};