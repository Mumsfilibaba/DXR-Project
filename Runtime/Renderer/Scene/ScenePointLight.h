#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Frustum.h"
#include "Renderer/Scene/SceneObject.h"
#include "Renderer/Scene/SceneView.h"

class FPointLight;

struct FScenePointLight : public FSceneObject
{
    struct FShadowData
    {
        FMatrix4 ViewProjMatrix;
        FVector3 Position;
        float    NearPlane;
        float    FarPlane;
    };

    FScenePointLight(FScene* InScene, FPointLight* InPointLight);
    ~FScenePointLight();

    virtual void Tick() override final;

    // Pointer to the light in the world
    FPointLight* PointLight;

    // Shadow generation information
    FShadowData ShadowData[RHI_NUM_CUBE_FACES];

    // Store data for each face
    FSceneView ShadowView[RHI_NUM_CUBE_FACES];

    // Store data for a single pass cube-map
    FSceneView SinglePassShadowView;

    FVector3 Position;
    FVector3 Color;
    float    ShadowBias;
};
