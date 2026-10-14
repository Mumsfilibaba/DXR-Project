#include "Renderer/Scene/SceneProxyData.h"
#include "Renderer/Scene/SceneSkybox.h"

FSceneSkybox::FSceneSkybox(FScene* InScene)
    : FSceneObject(InScene)
    , CubeMap(nullptr)
    , Intensity(1.0f)
    , Rotation(0.0f)
{
}

FSceneSkybox::~FSceneSkybox() = default;

void FSceneSkybox::RenderThread_ApplyUpdate(const FSkyboxProxyUpdate& Update)
{
    CubeMap   = Update.CubeMap;
    Intensity = Update.Intensity;
    Rotation  = Update.Rotation;
}
