#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Frustum.h"
#include "Renderer/Scene/SceneObject.h"

class FSkyLight;

struct FSceneSkyLight : public FSceneObject
{
    FSceneSkyLight(FScene* InScene, const FRHITextureRef& InSourceCubeMap);
    ~FSceneSkyLight();

    // Filters the source into the necessary cube-maps
    void RenderThread_FilterStaticCubeMaps();

    // Source cube-map
    FRHITextureRef SourceCubeMap;

    // Cube-maps
    FRHITextureRef SpecularCubeMap;
    FRHITextureRef DiffuseCubeMap;
};