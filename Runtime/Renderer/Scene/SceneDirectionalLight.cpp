#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneDirectionalLight.h"
#include "Renderer/Scene/SceneProxyData.h"

FSceneDirectionalLight::FSceneDirectionalLight(FScene* InScene)
    : FSceneObject(InScene)
    , ShadowView()
    , Color(1.0f, 1.0f, 1.0f)
    , Direction(-FVector3::Up)
    , UpVector(FVector3::Up)
    , ShadowMatrix()
    , ShadowNearPlane(0.0f)
    , ShadowFarPlane(0.0f)
    , ShadowBias(0.0f)
    , LightArea(0.05f)
    , bCastShadows(true)
{
    ShadowMatrix.SetIdentity();
}

FSceneDirectionalLight::~FSceneDirectionalLight() = default;

void FSceneDirectionalLight::RenderThread_ApplyUpdate(const FDirectionalLightProxyUpdate& Update, const FMatrix4& CameraViewProjectionInverse)
{
    Color                = Update.Color;
    Direction            = Update.Direction;
    ShadowNearPlane      = Update.ShadowNearPlane;
    ShadowFarPlane       = Update.ShadowFarPlane;
    ShadowBias           = Update.ShadowBias;
    ShadowPositionOffset = Update.ShadowPositionOffset;
    CascadeSplitLambda   = Update.CascadeSplitLambda;
    LightArea            = Update.LightArea;
    bCastShadows         = Update.bCastShadows;

    // Update ShadowMatrix
    FVector3 FrustumCorners[8] =
    {
        FVector3(-1.0f, 1.0f, 0.0f),
        FVector3( 1.0f, 1.0f, 0.0f),
        FVector3( 1.0f, -1.0f, 0.0f),
        FVector3(-1.0f, -1.0f, 0.0f),
        FVector3(-1.0f, 1.0f, 1.0f),
        FVector3( 1.0f, 1.0f, 1.0f),
        FVector3( 1.0f, -1.0f, 1.0f),
        FVector3(-1.0f, -1.0f, 1.0f),
    };

    // NOTE: Need to transpose since this matrix is assumed to be used on the GPU
    FMatrix4 InvViewProjection = CameraViewProjectionInverse;
    InvViewProjection = InvViewProjection.GetTranspose();

    // Calculate the center of frustum
    FVector3 FrustumCenter = FVector3(0.0f);
    for (int32 Corner = 0; Corner < 8; ++Corner)
    {
        FrustumCorners[Corner] = InvViewProjection.TransformCoord(FrustumCorners[Corner]);
        FrustumCenter += FrustumCorners[Corner];
    }

    FrustumCenter /= 8.0f;

    // Calculate a Shadow-matrix
    {
        UpVector = FVector3::Up;

        FVector3 ShadowLookAt           = FrustumCenter - Direction;
        FVector3 ShadowPosition         = FrustumCenter + Direction * -0.5f;
        FMatrix4 ShadowViewMatrix       = FMatrix4::LookAt(ShadowPosition, ShadowLookAt, UpVector);
        FMatrix4 ShadowProjectionMatrix = FMatrix4::OrthographicProjection(-0.5f, 0.5f, -0.5f, 0.5f, 0.0f, 1.0f);
        ShadowMatrix = ShadowViewMatrix * ShadowProjectionMatrix;
    }
}
