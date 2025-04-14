#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Frustum.h"
#include "Renderer/Scene/SceneObject.h"
#include "Renderer/Scene/SceneView.h"

class FDirectionalLight;

struct FSceneDirectionalLight : public FSceneObject
{
    FSceneDirectionalLight(FScene* InScene, FDirectionalLight* InDirectionalLight);
    ~FSceneDirectionalLight();

    virtual void Tick() override final;

    // Pointer to the light in the world
    FDirectionalLight* DirectionalLight;

    // View for shadow rendering
    FSceneView ShadowView;

    // Light properties
    FVector3 Color;
    FVector3 Direction;
    FVector3 UpVector;
    FMatrix4 ShadowMatrix;
    float    ShadowNearPlane;
    float    ShadowFarPlane;
    float    ShadowBias;
    float    ShadowPositionOffset;
    float    CascadeSplitLambda;
    float    LightArea;
};
