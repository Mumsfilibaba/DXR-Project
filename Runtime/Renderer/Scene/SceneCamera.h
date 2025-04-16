#pragma once
#include "Renderer/Scene/SceneObject.h"

class FCamera;

class FSceneCamera : public FSceneObject
{
public:
    FSceneCamera(FScene* InScene, FCamera* InCamera);
    virtual ~FSceneCamera();

    virtual void Tick() { }

private:
    FCamera* Camera;
};