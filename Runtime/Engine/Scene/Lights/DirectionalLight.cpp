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
    , CascadeSplitLambda(GCascadeSplitLambda.GetFloat())
    , Size(GSunSize.GetFloat())
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
        StartDirection = RotationMatrix.TransformNormal(StartDirection);
        Direction      = StartDirection.GetNormalized();
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
    for (int32 Corner = 0; Corner < 8; ++Corner)
    {
        FrustumCorners[Corner] = InverseViewProjection.TransformCoord(FrustumCorners[Corner]);
        FrustumCenter += FrustumCorners[Corner];
    }
    FrustumCenter /= 8.0f;

    // Calculate a Shadow-matrix
    UpVector = FVector3(0.0f, 1.0f, 0.0f);

    {
        FVector3 ShadowLookAt   = FrustumCenter - Direction;
        FVector3 ShadowPosition = FrustumCenter + (Direction * -0.5f);

        FMatrix4 ShadowViewMatrix       = FMatrix4::LookAt(ShadowPosition, ShadowLookAt, UpVector);
        FMatrix4 ShadowProjectionMatrix = FMatrix4::OrtographicProjection(-0.5f, 0.5f, -0.5f, 0.5f, 0.0f, 1.0f);

        ShadowMatrix = ShadowViewMatrix * ShadowProjectionMatrix;
        ShadowMatrix = ShadowMatrix.Transpose();
    }

    // Generate a bounds matrix
    {
        float Radius = 0.0f;
        for (int32 Index = 0; Index < 8; ++Index)
        {
            const float Distance = (FrustumCorners[Index] - FrustumCenter).GetLength();
            Radius = NMath::Max(Radius, Distance);
        }

        Radius = NMath::Ceil(Radius * 16.0f) / 16.0f;

        FVector3 MaxExtents = FVector3(Radius);
        FVector3 MinExtents = -MaxExtents;

        // Setup ShadowView
        FVector3 Extents        = MaxExtents - MinExtents;
        FVector3 LightDirection = Direction.GetNormalized();
        Position = FrustumCenter - LightDirection * MaxExtents.z;

        ShadowNearPlane = -Extents.z;
        ShadowFarPlane  =  Extents.z;

        ViewMatrix = FMatrix4::LookAt(Position, LightDirection, UpVector);
        ViewMatrix = ViewMatrix.Transpose();

        ProjectionMatrix = FMatrix4::OrtographicProjection(MinExtents.x, MaxExtents.x, MinExtents.y, MaxExtents.y, ShadowNearPlane, ShadowFarPlane);
        // ProjectionMatrix = ProjectionMatrix.Transpose();
    }
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
