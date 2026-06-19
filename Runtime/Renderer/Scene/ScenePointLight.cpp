#include "Renderer/Scene/ScenePointLight.h"
#include "Renderer/Scene/SceneProxyData.h"

FScenePointLight::FScenePointLight(FScene* InScene)
    : FSceneObject(InScene)
    , Position()
    , Color()
    , ShadowBias(0.0f)
    , ShadowFarPlane(0.0f)
{
}

FScenePointLight::~FScenePointLight() = default;

void FScenePointLight::RenderThread_ApplyUpdate(const FPointLightProxyUpdate& Update)
{
    Color          = Update.Color;
    Position       = Update.Position;
    ShadowBias     = Update.ShadowBias;
    ShadowFarPlane = Update.ShadowFarPlane;

    for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
    {
        // ViewProjMatrix is consumed on the GPU, so it is stored transposed.
        Matrix4 ViewProjMatrix = Update.ViewProjMatrix[FaceIndex].GetTranspose();

        ShadowData[FaceIndex].ViewProjMatrix = ViewProjMatrix;
        ShadowData[FaceIndex].Position       = Update.Position;
        ShadowData[FaceIndex].NearPlane      = Update.ShadowNearPlane;
        ShadowData[FaceIndex].FarPlane       = Update.ShadowFarPlane;

        // Non-transposed matrices used for CPU-side frustum culling in PrepareViewsForRendering.
        ViewMatrix[FaceIndex] = Update.ViewMatrix[FaceIndex];
        ProjMatrix[FaceIndex] = Update.ProjMatrix[FaceIndex];
    }
}
