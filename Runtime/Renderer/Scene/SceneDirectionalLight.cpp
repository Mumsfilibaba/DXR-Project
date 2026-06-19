#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneDirectionalLight.h"
#include "Renderer/Scene/SceneProxyData.h"

FSceneDirectionalLight::FSceneDirectionalLight(FScene* InScene)
    : FSceneObject(InScene)
    , ShadowView()
    , Color(1.0f, 1.0f, 1.0f)
    , Direction(-Vector3::Up)
    , UpVector(Vector3::Up)
    , ShadowMatrix()
    , ShadowNearPlane(0.0f)
    , ShadowFarPlane(0.0f)
    , ShadowBias(0.0f)
    , LightArea(0.05f)
{
    ShadowMatrix.SetIdentity();
}

FSceneDirectionalLight::~FSceneDirectionalLight() = default;

void FSceneDirectionalLight::RenderThread_ApplyUpdate(const FDirectionalLightProxyUpdate& Update)
{
    Color                = Update.Color;
    Direction            = Update.Direction;
    ShadowNearPlane      = Update.ShadowNearPlane;
    ShadowFarPlane       = Update.ShadowFarPlane;
    ShadowBias           = Update.ShadowBias;
    ShadowPositionOffset = Update.ShadowPositionOffset;
    CascadeSplitLambda   = Update.CascadeSplitLambda;
    LightArea            = Update.LightArea;

    // Update ShadowMatrix
    Vector3 FrustumCorners[8] =
    {
        Vector3(-1.0f,  1.0f, 0.0f),
        Vector3( 1.0f,  1.0f, 0.0f),
        Vector3( 1.0f, -1.0f, 0.0f),
        Vector3(-1.0f, -1.0f, 0.0f),
        Vector3(-1.0f,  1.0f, 1.0f),
        Vector3( 1.0f,  1.0f, 1.0f),
        Vector3( 1.0f, -1.0f, 1.0f),
        Vector3(-1.0f, -1.0f, 1.0f),
    };

    // NOTE: Need to transpose since this matrix is assumed to be used on the GPU
    Matrix4 InvViewProjection = Update.CameraViewProjectionInverse;
    InvViewProjection = InvViewProjection.GetTranspose();

    // Calculate the center of frustum
    Vector3 FrustumCenter = Vector3(0.0f);
    for (int32 Corner = 0; Corner < 8; ++Corner)
    {
        FrustumCorners[Corner] = InvViewProjection.TransformCoord(FrustumCorners[Corner]);
        FrustumCenter += FrustumCorners[Corner];
    }

    FrustumCenter /= 8.0f;

    // Calculate a Shadow-matrix
    {
        UpVector = Vector3::Up;

        Vector3 ShadowLookAt           = FrustumCenter - Direction;
        Vector3 ShadowPosition         = FrustumCenter + Direction * -0.5f;
        Matrix4 ShadowViewMatrix       = Matrix4::LookAt(ShadowPosition, ShadowLookAt, UpVector);
        Matrix4 ShadowProjectionMatrix = Matrix4::OrthographicProjection(-0.5f, 0.5f, -0.5f, 0.5f, 0.0f, 1.0f);
        ShadowMatrix = ShadowViewMatrix * ShadowProjectionMatrix;
    }
}
