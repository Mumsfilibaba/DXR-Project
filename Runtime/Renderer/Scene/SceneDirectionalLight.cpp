#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneDirectionalLight.h"
#include "Engine/World/Camera.h"
#include "Engine/World/Lights/DirectionalLight.h"

FSceneDirectionalLight::FSceneDirectionalLight(FScene* InScene, FDirectionalLight* InDirectionalLight)
    : FSceneObject(InScene)
    , DirectionalLight(InDirectionalLight)
    , ShadowView()
    , Color(1.0f, 1.0f, 1.0f)
    , Direction(-FVector3::Up)
    , UpVector(FVector3::Up)
    , ShadowMatrix()
    , ShadowNearPlane(0.0f)
    , ShadowFarPlane(0.0f)
    , ShadowBias(0.0f)
    , LightArea(0.05f)
{
    ShadowMatrix.SetIdentity();
}

FSceneDirectionalLight::~FSceneDirectionalLight()
{
    DirectionalLight = nullptr;
}

void FSceneDirectionalLight::Tick()
{
    // Retrieve color 
    FVector3 LocalColor = DirectionalLight->GetColor();

    // TODO: Just specify the light color directly FVector4(100.0f, 1.0f, 58.0f, 6.0f)
    Color = LocalColor * DirectionalLight->GetIntensity();

    // Update any values that might have been updated
    Direction            = DirectionalLight->GetDirectionVector();
    ShadowNearPlane      = DirectionalLight->GetShadowNearPlane();
    ShadowFarPlane       = DirectionalLight->GetShadowFarPlane();
    ShadowBias           = DirectionalLight->GetShadowBias();
    ShadowPositionOffset = DirectionalLight->GetShadowPositionOffset();
    CascadeSplitLambda   = DirectionalLight->GetCascadeSplitLambda();
    LightArea            = DirectionalLight->GetLightArea();

    // Update ShadowMatrix
    FVector3 FrustumCorners[8] =
    {
        FVector3(-1.0f,  1.0f, 0.0f),
        FVector3( 1.0f,  1.0f, 0.0f),
        FVector3( 1.0f, -1.0f, 0.0f),
        FVector3(-1.0f, -1.0f, 0.0f),
        FVector3(-1.0f,  1.0f, 1.0f),
        FVector3( 1.0f,  1.0f, 1.0f),
        FVector3( 1.0f, -1.0f, 1.0f),
        FVector3(-1.0f, -1.0f, 1.0f),
    };

    // NOTE: Need to transpose since this matrix is assumed to be used on the GPU
    FMatrix4 InvViewProjection = GetScene()->Camera->GetViewProjectionInverseMatrix();
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
        // Update up-vector
        UpVector = FVector3::Up;

        FVector3 ShadowLookAt           = FrustumCenter - Direction;
        FVector3 ShadowPosition         = FrustumCenter + Direction * -0.5f;
        FMatrix4 ShadowViewMatrix       = FMatrix4::LookAt(ShadowPosition, ShadowLookAt, UpVector);
        FMatrix4 ShadowProjectionMatrix = FMatrix4::OrthographicProjection(-0.5f, 0.5f, -0.5f, 0.5f, 0.0f, 1.0f);
        ShadowMatrix = ShadowViewMatrix * ShadowProjectionMatrix;
    }
}
