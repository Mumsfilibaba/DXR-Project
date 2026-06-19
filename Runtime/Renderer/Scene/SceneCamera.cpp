#include "Renderer/Scene/SceneCamera.h"

FSceneCamera::FSceneCamera(FScene* InScene)
    : FSceneObject(InScene)
    , Snapshot()
{
}

FSceneCamera::~FSceneCamera() = default;
