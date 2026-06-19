#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "Renderer/Scene/SceneObject.h"
#include "Renderer/Scene/SceneProxyData.h"

struct FSceneCamera : public FSceneObject
{
    FSceneCamera(FScene* InScene);
    virtual ~FSceneCamera();

    FCameraSnapshot Snapshot;
};
