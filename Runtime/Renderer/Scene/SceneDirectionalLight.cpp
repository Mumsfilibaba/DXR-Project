#include "Renderer/Scene/Scene.h"
#include "Renderer/Scene/SceneDirectionalLight.h"
#include "Engine/World/Camera.h"
#include "Engine/World/Lights/DirectionalLight.h"

FSceneDirectionalLight::FSceneDirectionalLight(FScene* InScene, FDirectionalLight* InDirectionalLight)
    : FSceneObject(InScene)
    , DirectionalLight(InDirectionalLight)
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

FSceneDirectionalLight::~FSceneDirectionalLight()
{
    DirectionalLight = nullptr;
}

void FSceneDirectionalLight::Tick()
{
    // Retrieve color 
    Vector3 LocalColor = DirectionalLight->GetColor();

    // TODO: Just specify the light color directly Vector4(100.0f, 1.0f, 58.0f, 6.0f)
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
    Matrix4 InvViewProjection = GetScene()->Camera->GetViewProjectionInverseMatrix();
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
        // Update up-vector
        UpVector = Vector3::Up;

        Vector3 ShadowLookAt           = FrustumCenter - Direction;
        Vector3 ShadowPosition         = FrustumCenter + Direction * -0.5f;
        Matrix4 ShadowViewMatrix       = Matrix4::LookAt(ShadowPosition, ShadowLookAt, UpVector);
        Matrix4 ShadowProjectionMatrix = Matrix4::OrthographicProjection(-0.5f, 0.5f, -0.5f, 0.5f, 0.0f, 1.0f);
        ShadowMatrix = ShadowViewMatrix * ShadowProjectionMatrix;
    }
}
