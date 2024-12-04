#include "DirectionalLight.h"
#include "Core/Math/Math.h"
#include "Core/Misc/ConsoleManager.h"
#include "Engine/World/Camera.h"

static TAutoConsoleVariable<float> CVarSunSize(
    "Scene.Lightning.Sun.Size",
    "Sets the size of the sun, used to determine the penumbra for soft-shadows", 
    0.05f);

static TAutoConsoleVariable<float> CVarCascadeSplitLambda(
    "Scene.Lightning.CascadeSplitLambda",
    "Determines how the Cascades should be split for the Cascaded Shadow Maps", 
    1.0f);

FOBJECT_IMPLEMENT_CLASS(FDirectionalLight);

FDirectionalLight::FDirectionalLight(const FObjectInitializer& ObjectInitializer)
    : FLight(ObjectInitializer)
    , Direction(0.0f, -1.0f, 0.0f)
    , Rotation(0.0f, 0.0f, 0.0f)
    , LookAt(0.0f, 0.0f, 0.0f)
    , Position(0.0f, 0.0f, 0.0f)
    , CascadeSplitLambda(CVarCascadeSplitLambda.GetValue())
    , Size(CVarSunSize.GetValue())
{
    // TODO: Probably move to scene
    CVarSunSize->SetOnChangedDelegate(FConsoleVariableDelegate::CreateLambda([this](IConsoleVariable* SunLight)
    {
        if (SunLight && SunLight->IsVariableFloat())
        {
            const float NewSize = FMath::Clamp(SunLight->GetFloat(), 0.0f, 1.0f);
            this->Size = NewSize;
        }
    }));

    CVarCascadeSplitLambda->SetOnChangedDelegate(FConsoleVariableDelegate::CreateLambda([this](IConsoleVariable* CascadeSplitLambda)
    {
        if (CascadeSplitLambda && CascadeSplitLambda->IsVariableFloat())
        {
            const float NewLambda = FMath::Clamp(CascadeSplitLambda->GetFloat(), 0.0f, 1.0f);
            this->CascadeSplitLambda = NewLambda;
        }
    }));

    ShadowMatrix.SetIdentity();
    ViewMatrix.SetIdentity();
    ProjectionMatrix.SetIdentity();
}

void FDirectionalLight::Tick(FCamera& Camera)
{
    // Update direction based on rotation
    {
        FMatrix4 RotationMatrix = FMatrix4::RotationRollPitchYaw(Rotation.X, Rotation.Y, Rotation.Z);

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
    InverseViewProjection = InverseViewProjection.GetTranspose();

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
        FMatrix4 ShadowProjectionMatrix = FMatrix4::OrthographicProjection(-0.5f, 0.5f, -0.5f, 0.5f, 0.0f, 1.0f);

        ShadowMatrix = ShadowViewMatrix * ShadowProjectionMatrix;
        ShadowMatrix = ShadowMatrix.GetTranspose();
    }

    // Generate a bounds matrix
    {
        float Radius = 0.0f;
        for (int32 Index = 0; Index < 8; ++Index)
        {
            const float Distance = (FrustumCorners[Index] - FrustumCenter).GetLength();
            Radius = FMath::Max(Radius, Distance);
        }

        Radius = FMath::Ceil(Radius * 16.0f) / 16.0f;

        FVector3 MaxExtents = FVector3(Radius);
        FVector3 MinExtents = -MaxExtents;

        // Setup ShadowView
        FVector3 Extents        = MaxExtents - MinExtents;
        FVector3 LightDirection = Direction.GetNormalized();
        Position = FrustumCenter - LightDirection * MaxExtents.Z;

        ShadowNearPlane = -Extents.Z;
        ShadowFarPlane  =  Extents.Z;

        ViewMatrix = FMatrix4::LookAt(Position, LightDirection, UpVector);
        ViewMatrix = ViewMatrix.GetTranspose();

        ProjectionMatrix = FMatrix4::OrthographicProjection(MinExtents.X, MaxExtents.X, MinExtents.Y, MaxExtents.Y, ShadowNearPlane, ShadowFarPlane);
        // ProjectionMatrix = ProjectionMatrix.GetTranspose();
    }
}

void FDirectionalLight::SetRotation(const FVector3& InRotation)
{
    Rotation = InRotation;
}

void FDirectionalLight::SetCascadeSplitLambda(float InCascadeSplitLambda)
{
    CVarCascadeSplitLambda->SetAsFloat(InCascadeSplitLambda, EConsoleVariableFlags::SetByCode);
}

void FDirectionalLight::SetSize(float InSize)
{
    CVarSunSize->SetAsFloat(InSize, EConsoleVariableFlags::SetByCode);
}
