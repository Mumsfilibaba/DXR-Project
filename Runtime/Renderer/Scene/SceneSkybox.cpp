#include "Renderer/Scene/SceneSkybox.h"

FSceneSkybox::FSceneSkybox(FSkyboxComponent* InSkybox)
    : Skybox(InSkybox)
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
