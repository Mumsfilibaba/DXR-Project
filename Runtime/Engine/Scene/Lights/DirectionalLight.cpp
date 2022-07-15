#include "DirectionalLight.h"

#include "Core/Math/Math.h"
#include "Core/Debug/Console/ConsoleManager.h"
#include "Core/Debug/Console/ConsoleVariable.h"

#include "Engine/Scene/Camera.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Console-variable

TAutoConsoleVariable<float> GSunSize("Scene.SunSize", 0.5f);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// DirectionalLight

FDirectionalLight::FDirectionalLight()
    : FLight()
    , Direction(0.0f, -1.0f, 0.0f)
    , Rotation(0.0f, 0.0f, 0.0f)
    , LookAt(0.0f, 0.0f, 0.0f)
    , Position(0.0f, 0.0f, 0.0f)
    , Matrices()
{
    CORE_OBJECT_INIT();

    // TODO: Probably move to scene
    GSunSize.GetChangedDelegate().AddLambda([this](IConsoleVariable* SunLight)
    {
        if (SunLight && SunLight->IsFloat())
        {
            const float NewSize = NMath::Clamp(0.0f, 1.0f, SunLight->GetFloat());
            this->SetSize(NewSize);
        }
    });

    for (uint32 i = 0; i < NUM_SHADOW_CASCADES; i++)
    {
        Matrices[i].SetIdentity();
        ViewMatrices[i].SetIdentity();
        ProjectionMatrices[i].SetIdentity();
    }
}

FDirectionalLight::~FDirectionalLight()
{
    // Empty for now
}

void FDirectionalLight::UpdateCascades(FCamera& Camera)
{
    //XMVECTOR XmDirection = XMVectorSet( 0.0, -1.0f, 0.0f, 0.0f );
    //XMMATRIX XmRotation = XMMatrixRotationRollPitchYaw( Rotation.x, Rotation.y, Rotation.z );
    //XMVECTOR XmOffset = XMVector3Transform( XmDirection, XmRotation );
    //XmDirection = XMVector3Normalize( XmOffset );
    //XMStoreFloat3( &Direction, XmDirection );

    FMatrix4 RotationMatrix = FMatrix4::RotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);

    FVector3 StartDirection(0.0f, -1.0f, 0.0f);
    StartDirection = RotationMatrix.TransformDirection(StartDirection);
    StartDirection.Normalize();
    Direction = StartDirection;

    FVector3 StartUp(0.0, 0.0f, 1.0f);
    StartUp = RotationMatrix.TransformDirection(StartUp);
    StartUp.Normalize();
    Up = StartUp;

    FMatrix4 InvCamera = Camera.GetViewProjectionInverseMatrix();
    InvCamera = InvCamera.Transpose();

    float NearPlane = Camera.GetNearPlane();
    float FarPlane = NMath::Min<float>(Camera.GetFarPlane(), 100.0f); // TODO: Should be a setting
    float ClipRange = FarPlane - NearPlane;

    ShadowNearPlane = NearPlane;
    ShadowFarPlane = FarPlane;

    float MinZ = NearPlane;
    float MaxZ = FarPlane;

    float Range = ClipRange;
    float Ratio = MaxZ / MinZ;

    float LocalCascadeSplits[NUM_SHADOW_CASCADES];

    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for (uint32 i = 0; i < 4; i++)
    {
        float p = (i + 1) / static_cast<float>(NUM_SHADOW_CASCADES);
        float Log = MinZ * std::pow(Ratio, p);
        float Uniform = MinZ + Range * p;
        float d = CascadeSplitLambda * (Log - Uniform) + Uniform;
        LocalCascadeSplits[i] = (d - NearPlane) / ClipRange;
    }

    // TODO: This has to be moved so we do not duplicate it
    const float CascadeSizes[NUM_SHADOW_CASCADES] =
    {
        2048.0f, 2048.0f, 2048.0f, 4096.0f
    };

    UNREFERENCED_VARIABLE(CascadeSizes);

    float LastSplitDist = 0.0f;
    for (uint32 i = 0; i < 4; i++)
    {
        float SplitDist = LocalCascadeSplits[i];

        FVector4 FrustumCorners[8] =
        {
            FVector4(-1.0f,  1.0f, 0.0f, 1.0f),
            FVector4(1.0f,  1.0f, 0.0f, 1.0f),
            FVector4(1.0f, -1.0f, 0.0f, 1.0f),
            FVector4(-1.0f, -1.0f, 0.0f, 1.0f),
            FVector4(-1.0f,  1.0f, 1.0f, 1.0f),
            FVector4(1.0f,  1.0f, 1.0f, 1.0f),
            FVector4(1.0f, -1.0f, 1.0f, 1.0f),
            FVector4(-1.0f, -1.0f, 1.0f, 1.0f),
        };

        // Calculate position of light frustum
        for (uint32 j = 0; j < 8; j++)
        {
            FVector4 Corner = InvCamera * FrustumCorners[j];
            FrustumCorners[j] = Corner / Corner.w;
        }

        for (uint32 j = 0; j < 4; j++)
        {
            const FVector4 Distance = FrustumCorners[j + 4] - FrustumCorners[j];
            FrustumCorners[j + 4] = FrustumCorners[j] + (Distance * SplitDist);
            FrustumCorners[j] = FrustumCorners[j] + (Distance * LastSplitDist);
        }

        // Calc frustum center
        FVector4 Center = FVector4(0.0f);
        for (uint32 j = 0; j < 8; j++)
        {
            Center = Center + FrustumCorners[j];
        }
        Center = Center * (1.0f / 8.0f);

        float Radius = 0.0f;
        for (uint32 j = 0; j < 8; j++)
        {
            float Distance = ceil((FrustumCorners[j] - Center).Length());
            Radius = NMath::Min(NMath::Max(Radius, Distance), 80.0f); // This should be dynamic
        }

        // Make sure we only move cascades with whole pixels
        //float TexelsPerUnit = CascadeSizes[i] / (Radius * 2.0f);
        //XMMATRIX Scale = XMMatrixScaling(TexelsPerUnit, TexelsPerUnit, TexelsPerUnit);

        //XMVECTOR EyePosition  = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        //XMVECTOR LookPosition = XMVectorSet(-Direction.x, -Direction.y, -Direction.z, 0.0f);
        //
        //XMMATRIX LookAtMat = XMMatrixLookAtLH(EyePosition, LookPosition, XmUp);
        //LookAtMat = XMMatrixMultiply(Scale, LookAtMat);

        //XMMATRIX LookAtMatInverse = XMMatrixInverse(nullptr, LookAtMat);

        //XMVECTOR XmCenter = XMLoadFloat4(&Center);
        //XMVector3Transform(XmCenter, LookAtMat);
        //XMStoreFloat4(&Center, XmCenter);

        //Center.x = floor(Center.x);
        //Center.y = floor(Center.y);
        //Center.z = floor(Center.z);

        //XmCenter = XMLoadFloat4(&Center);
        //XMVector3Transform(XmCenter, LookAtMatInverse);
        //XMStoreFloat4(&Center, XmCenter);

        FVector3 CascadePosition = FVector3(Center.x, Center.y, Center.z) - (Direction * Radius * 6.0f);
        FVector3 EyePosition = CascadePosition;
        FVector3 LookPosition = FVector3(Center.x, Center.y, Center.z);

        FMatrix4 View = FMatrix4::LookAt(EyePosition, LookPosition, Up);
        FMatrix4 Projection = FMatrix4::OrtographicProjection(-Radius, Radius, -Radius, Radius, 0.01f, Radius * 12.0f);
        ViewMatrices[i] = View.Transpose();
        ProjectionMatrices[i] = Projection.Transpose();
        Matrices[i] = (View * Projection).Transpose();

        LastSplitDist = SplitDist;

        CascadeSplits[i] = (NearPlane + SplitDist * ClipRange);
        CascadeRadius[i] = Radius;

        if (i == 0)
        {
            LookAt = FVector3(Center.x, Center.y, Center.z);
            Position = CascadePosition;
        }
    }

    return;
}

void FDirectionalLight::SetRotation(const FVector3& InRotation)
{
    Rotation = InRotation;
}

void FDirectionalLight::SetRotation(float x, float y, float z)
{
    SetRotation(FVector3(x, y, z));
}

void FDirectionalLight::SetLookAt(const FVector3& InLookAt)
{
    LookAt = InLookAt;
}

void FDirectionalLight::SetLookAt(float x, float y, float z)
{
    SetLookAt(FVector3(x, y, z));
}
