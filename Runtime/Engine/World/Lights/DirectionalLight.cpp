#include "Core/Math/Math.h"
#include "Core/Misc/ConsoleManager.h"
#include "Engine/World/Camera.h"
#include "Engine/World/Lights/DirectionalLight.h"

FOBJECT_IMPLEMENT_CLASS(FDirectionalLight);

FDirectionalLight::FDirectionalLight(const FObjectInitializer& ObjectInitializer)
    : FLight(ObjectInitializer, 120.0f, 250.0f)
    , Direction(-FVector3::Up)
    , Rotation(0.0f, 0.0f, 0.0f)
    , ShadowPositionOffset(200.0f)
    , CascadeSplitLambda(0.60f)
    , CascadeSplitMode(ECascadeSplitMode::AutoLambda)
    , ManualCascadeSplitDistances{ 50.0f, 150.0f, 400.0f }
    , LightArea(0.5f)
{
    ShadowBias = 0.0005f;
}

FDirectionalLight::~FDirectionalLight()
{
}

void FDirectionalLight::SetRotation(const FVector3& InRotation)
{
    Rotation = InRotation;

    // Update direction based on rotation
    FMatrix4 RotationMatrix = FMatrix4::RotationRollPitchYaw(Rotation.X, Rotation.Y, Rotation.Z);

    // Create the proper direction
    FVector3 StartDirection = -FVector3::Up;
    Direction = RotationMatrix.TransformNormal(StartDirection).GetNormalized();
}

void FDirectionalLight::SetCascadeSplitLambda(float InCascadeSplitLambda)
{
    CascadeSplitLambda = InCascadeSplitLambda;
}

void FDirectionalLight::SetCascadeSplitMode(ECascadeSplitMode InMode)
{
    CascadeSplitMode = InMode;
}

void FDirectionalLight::SetManualCascadeSplitDistance(int32 Index, float Distance)
{
    if (Index < 0 || Index >= (NUM_SHADOW_CASCADES - 1))
    {
        return;
    }

    ManualCascadeSplitDistances[Index] = Math::Max(Distance, 0.0f);
}

void FDirectionalLight::SetShadowPositionOffset(float InShadowPositionOffset)
{
    ShadowPositionOffset = InShadowPositionOffset;
}

void FDirectionalLight::SetLightArea(float InLightArea)
{
    LightArea = InLightArea;
}
