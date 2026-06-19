#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Frustum.h"
#include "Renderer/Scene/SceneObject.h"
#include "Renderer/Scene/SceneView.h"

class FDirectionalLight;
struct FDirectionalLightProxyUpdate;

struct FSceneDirectionalLight : public FSceneObject
{
    FSceneDirectionalLight(FScene* InScene);
    ~FSceneDirectionalLight();

    // Applies a per-frame light snapshot (recomputes the cascade shadow matrix).
    void RenderThread_ApplyUpdate(const FDirectionalLightProxyUpdate& Update);

    FSceneView ShadowView;
    Vector3    Color;
    Vector3    Direction;
    Vector3    UpVector;
    Matrix4    ShadowMatrix;
    float      ShadowNearPlane;
    float      ShadowFarPlane;
    float      ShadowBias;
    float      ShadowPositionOffset;
    float      CascadeSplitLambda;
    float      LightArea;
};
