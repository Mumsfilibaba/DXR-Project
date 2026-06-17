#include "Engine/World/Lights/PointLight.h"
#include "Renderer/Scene/ScenePointLight.h"

FScenePointLight::FScenePointLight(FScene* InScene, FPointLight* InPointLight)
    : FSceneObject(InScene)
    , PointLight(InPointLight)
{
}

FScenePointLight::~FScenePointLight()
{
    PointLight = nullptr;
}

void FScenePointLight::Tick()
{
    Color      = PointLight->GetColor();
    Position   = PointLight->GetPosition();
    ShadowBias = PointLight->GetShadowBias();

    for (int32 FaceIndex = 0; FaceIndex < RHI_NUM_CUBE_FACES; FaceIndex++)
    {
        // Update ShadowData
        Matrix4 ViewProjMatrix = PointLight->GetViewProjectionMatrix(FaceIndex);
        ViewProjMatrix = ViewProjMatrix.GetTranspose();

        ShadowData[FaceIndex].ViewProjMatrix = ViewProjMatrix;
        ShadowData[FaceIndex].Position       = PointLight->GetPosition();
        ShadowData[FaceIndex].NearPlane      = PointLight->GetShadowNearPlane();
        ShadowData[FaceIndex].FarPlane       = PointLight->GetShadowFarPlane();
    }
}