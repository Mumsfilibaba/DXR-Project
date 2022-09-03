#include "DirectionalLight.h"

#include "Core/Math/Math.h"
#include "Core/Debug/Console/ConsoleManager.h"
#include "Core/Debug/Console/ConsoleVariable.h"

#include "Engine/Scene/Camera.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Console-variables

TAutoConsoleVariable<float> GSunSize("Scene.Lightning.Sun.Size", 0.05f);
TAutoConsoleVariable<float> GCascadeSplitLambda("Scene.Lightning.CascadeSplitLambda", 1.0f);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FDirectionalLight

FDirectionalLight::FDirectionalLight()
    : FLight()
    , Direction(0.0f, -1.0f, 0.0f)
    , Rotation(0.0f, 0.0f, 0.0f)
    , LookAt(0.0f, 0.0f, 0.0f)
    , Position(0.0f, 0.0f, 0.0f)
    , Size(GSunSize.GetFloat())
    , CascadeSplitLambda(GCascadeSplitLambda.GetFloat())
{
    CORE_OBJECT_INIT();

    // TODO: Probably move to scene
    GSunSize.GetChangedDelegate().AddLambda([this](IConsoleVariable* SunLight)
    {
        if (SunLight && SunLight->IsFloat())
        {
            const float NewSize = NMath::Clamp(0.0f, 1.0f, SunLight->GetFloat());
            this->Size = NewSize;
        }
    });

    GCascadeSplitLambda.GetChangedDelegate().AddLambda([this](IConsoleVariable* CascadeSplitLambda)
    {
        if (CascadeSplitLambda && CascadeSplitLambda->IsFloat())
        {
            const float NewLambda = NMath::Clamp(0.0f, 1.0f, CascadeSplitLambda->GetFloat());
            this->CascadeSplitLambda = NewLambda;
        }
    });

    ShadowMatrix.SetIdentity();
    ViewMatrix.SetIdentity();
    ProjectionMatrix.SetIdentity();
}

void FDirectionalLight::Tick(FCamera& Camera)
{
    // Update direction based on rotation
    {
        FMatrix4 RotationMatrix = FMatrix4::RotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);

        FVector3 StartDirection(0.0f, -1.0f, 0.0f);
        StartDirection = RotationMatrix.TransformDirection(StartDirection);
        StartDirection.Normalize();
        Direction = StartDirection;
    }

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
    FMatrix4 InverseViewProjection = Camera.GetViewProjectionInverseMatrix();
    InverseViewProjection = InverseViewProjection.Transpose();

    // Calculate the center of frustum
    FVector3 FrustumCenter = FVector3(0.0f);
    for (uint32 Corner = 0; Corner < 8; ++Corner)
    {
        FrustumCorners[Corner] = InverseViewProjection.TransformPosition(FrustumCorners[Corner]);
        FrustumCenter += FrustumCorners[Corner];
    }
    FrustumCenter /= 8.0f;

    // Calculate a matrix
    UpVector = FVector3(0.0f, 1.0f, 0.0f);

    LookAt   = FrustumCenter - GetDirection();
    Position = FrustumCenter + GetDirection() * -0.5f;

    ViewMatrix       = FMatrix4::LookAt(Position, LookAt, UpVector);
    ProjectionMatrix = FMatrix4::OrtographicProjection(-0.5f, 0.5f, -0.5f, 0.5f, 0.0f, 1.0f);
    ShadowMatrix = ViewMatrix * ProjectionMatrix;
    ShadowMatrix = ShadowMatrix.Transpose();
}

void FDirectionalLight::SetRotation(const FVector3& InRotation)
{
    Rotation = InRotation;
}

void FDirectionalLight::SetCascadeSplitLambda(float InCascadeSplitLambda)
{
    GCascadeSplitLambda.SetFloat(InCascadeSplitLambda);
}

void FDirectionalLight::SetSize(float InSize)
{
    GSunSize.SetFloat(InSize);
}