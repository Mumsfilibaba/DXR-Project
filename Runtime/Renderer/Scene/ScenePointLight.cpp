#include "Renderer/Scene/ScenePointLight.h"

FScenePointLight::FScenePointLight(FScene* InScene, FPointLight* InPointLight)
    : FSceneObject(InScene)
    , PointLight(InPointLight)
{
}

FScenePointLight::~FScenePointLight()
{
    PointLight = nullptr;
}