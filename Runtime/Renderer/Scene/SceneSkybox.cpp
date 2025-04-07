#include "Renderer/Scene/SceneSkybox.h"

FSceneSkybox::FSceneSkybox(FScene* InScene, FSkyboxComponent* InSkybox)
    : FSceneObject(InScene)
    , Skybox(InSkybox)
{
    if (InSkybox)
    {
        CubeMap = InSkybox->GetCubeMap();
    }
}

FSceneSkybox::~FSceneSkybox()
{
    Skybox = nullptr;
}
