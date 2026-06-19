#include "Renderer/Scene/SceneSkybox.h"

FSceneSkybox::FSceneSkybox(FScene* InScene, const FRHITextureRef& InCubeMap)
    : FSceneObject(InScene)
    , CubeMap(InCubeMap)
{
}

FSceneSkybox::~FSceneSkybox() = default;
