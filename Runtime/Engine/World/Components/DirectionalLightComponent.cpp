#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/DirectionalLightComponent.h"

FOBJECT_IMPLEMENT_CLASS(FDirectionalLightComponent);

FDirectionalLightComponent::FDirectionalLightComponent(const FObjectInitializer& ObjectInitializer)
    : FLightComponent(ObjectInitializer, 120.0f, 250.0f)
    , ShadowPositionOffset(200.0f)
    , CascadeSplitLambda(0.95f)
    , LightArea(0.05f)
{
}

FDirectionalLightComponent::~FDirectionalLightComponent()
{
}

void FDirectionalLightComponent::SetCascadeSplitLambda(float InCascadeSplitLambda)
{
    CascadeSplitLambda = InCascadeSplitLambda;
}

void FDirectionalLightComponent::SetShadowPositionOffset(float InShadowPositionOffset)
{
    ShadowPositionOffset = InShadowPositionOffset;
}

void FDirectionalLightComponent::SetLightArea(float InLightArea)
{
    LightArea = InLightArea;
}

FVector3 FDirectionalLightComponent::GetDirectionVector() const
{
    CHECK(GetActorOwner() != nullptr);

    const FVector3& Rotation = GetActorOwner()->GetWorldTransform().GetRotation();
    const FMatrix4 RotationMatrix = FMatrix4::RotationRollPitchYaw(Rotation);
    return RotationMatrix.TransformNormal(-FVector3::Up).GetNormalized();
}
