#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Frustum.h"
#include "Renderer/Scene/SceneObject.h"
#include "Renderer/Scene/SceneView.h"

class FPointLight;
struct FPointLightProxyUpdate;

struct FScenePointLight : public FSceneObject
{
    struct FShadowData
    {
        Matrix4 ViewProjMatrix;
        Vector3 Position;
        float   NearPlane;
        float   FarPlane;
    };

    FScenePointLight(FScene* InScene);
    ~FScenePointLight();

    // Applies a per-frame light snapshot (shadow data + per-face view/projection matrices).
    void RenderThread_ApplyUpdate(const FPointLightProxyUpdate& Update);

    FShadowData  ShadowData[RHI_NUM_CUBE_FACES]; // Shadow generation information
    FSceneView   ShadowView[RHI_NUM_CUBE_FACES]; // Store data for each face
    FSceneView   SinglePassShadowView;           // Store data for a single pass cube-map
    Matrix4      ViewMatrix[RHI_NUM_CUBE_FACES]; // Per-face view matrices (for frustum culling)
    Matrix4      ProjMatrix[RHI_NUM_CUBE_FACES]; // Per-face projection matrices (for frustum culling)
    Vector3      Position;
    Vector3      Color;            // Pre-multiplied by intensity.
    float        ShadowBias;
    float        ShadowFarPlane;   // Doubles as the light radius for shading.
};
