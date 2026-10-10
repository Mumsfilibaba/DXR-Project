#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Frustum.h"
#include "Renderer/Scene/SceneObject.h"
#include "Renderer/Scene/SceneView.h"

struct FDirectionalLightProxyUpdate;

struct FSceneDirectionalLight : public FSceneObject
{
    FSceneDirectionalLight(FScene* InScene);
    ~FSceneDirectionalLight();

    // Applies a per-frame light snapshot and recomputes the cascade shadow matrix for the rendered view.
    void RenderThread_ApplyUpdate(const FDirectionalLightProxyUpdate& Update, const FMatrix4& CameraViewProjectionInverse);

    FSceneView ShadowView;
    FVector3   Color;
    FVector3   Direction;
    FVector3   UpVector;
    FMatrix4   ShadowMatrix;
    float      ShadowNearPlane;
    float      ShadowFarPlane;
    float      ShadowBias;
    float      ShadowPositionOffset;
    float      CascadeSplitLambda;
    float      LightArea;
    bool       bCastShadows;
};
