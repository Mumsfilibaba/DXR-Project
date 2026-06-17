#include "Core/Math/Math.h"
#include "Core/Misc/ConsoleManager.h"
#include "Engine/World/Camera.h"
#include "Engine/World/Lights/DirectionalLight.h"

FOBJECT_IMPLEMENT_CLASS(FDirectionalLight);

FDirectionalLight::FDirectionalLight(const FObjectInitializer& ObjectInitializer)
    : FLight(ObjectInitializer, 120.0f, 250.0f)
    , Direction(-Vector3::Up)
    , Rotation(0.0f, 0.0f, 0.0f)
    , ShadowPositionOffset(200.0f)
    , CascadeSplitLambda(0.95f)
    , LightArea(0.05f)
{
}

FDirectionalLight::~FDirectionalLight()
{
}

void FDirectionalLight::SetRotation(const Vector3& InRotation)
{
    Rotation = InRotation;

    // Update direction based on rotation
    Matrix4 RotationMatrix = Matrix4::RotationRollPitchYaw(Rotation.X, Rotation.Y, Rotation.Z);

    // Create the proper direction
    Vector3 StartDirection = -Vector3::Up;
    Direction = RotationMatrix.TransformNormal(StartDirection).GetNormalized();
}

void FDirectionalLight::SetCascadeSplitLambda(float InCascadeSplitLambda)
{
    CascadeSplitLambda = InCascadeSplitLambda;
}

void FDirectionalLight::SetShadowPositionOffset(float InShadowPositionOffset)
{
    ShadowPositionOffset = InShadowPositionOffset;
}

void FDirectionalLight::SetLightArea(float InLightArea)
{
    LightArea = InLightArea;
}
