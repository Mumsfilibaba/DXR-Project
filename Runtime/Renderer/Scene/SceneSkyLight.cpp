#include "Renderer/Scene/SceneProxyData.h"
#include "Renderer/Scene/SceneSkyLight.h"

FSceneSkyLight::FSceneSkyLight(FScene* InScene)
    : FSceneObject(InScene)
    , SpecularCubeMap(nullptr)
    , DiffuseCubeMap(nullptr)
    , Color(1.0f, 1.0f, 1.0f)
    , Rotation(0.0f)
{
}

FSceneSkyLight::~FSceneSkyLight() = default;

void FSceneSkyLight::RenderThread_ApplyUpdate(const FSkyLightProxyUpdate& Update)
{
    SpecularCubeMap = Update.SpecularCubeMap;
    DiffuseCubeMap  = Update.DiffuseCubeMap;
    Color           = Update.Color;
    Rotation        = Update.Rotation;
}
