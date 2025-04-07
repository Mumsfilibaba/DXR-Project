#include "Renderer/Scene/SceneObject.h"

FSceneObject::FSceneObject(FScene* InScene)
    : Scene(InScene)
{
}

FSceneObject::~FSceneObject()
{
    Scene = nullptr;
}