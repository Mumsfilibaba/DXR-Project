#include "Renderer/Scene/SceneDirectionalLight.h"

FSceneDirectionalLight::FSceneDirectionalLight(FScene* InScene, FDirectionalLight* InDirectionalLight)
    : FSceneObject(InScene)
    , DirectionalLight(InDirectionalLight)
{
}

FSceneDirectionalLight::~FSceneDirectionalLight()
{
    DirectionalLight = nullptr;
}

void FSceneDirectionalLight::Tick()
{
}