#include "Core/Math/Math.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/PointLightComponent.h"

FOBJECT_IMPLEMENT_CLASS(FPointLightComponent);

FPointLightComponent::FPointLightComponent(const FObjectInitializer& ObjectInitializer)
    : FLightComponent(ObjectInitializer, 1.0f, 30.0f)
    , ViewProjMatrices()
    , ViewMatrices()
    , ProjMatrices()
{
    SetTickable(true);
}

FPointLightComponent::~FPointLightComponent()
{
}

void FPointLightComponent::Tick(float /* DeltaTime */)
{
    UpdateShadowMatrices();
}

void FPointLightComponent::UpdateShadowMatrices()
{
    CalculateMatrices();
}

void FPointLightComponent::SetShadowNearPlane(float InShadowNearPlane)
{
    if (InShadowNearPlane > 0.0f && Math::Abs(ShadowFarPlane - InShadowNearPlane) >= 0.1f)
    {
        ShadowNearPlane = InShadowNearPlane;
        CalculateMatrices();
    }
}

void FPointLightComponent::SetShadowFarPlane(float InShadowFarPlane)
{
    if (InShadowFarPlane > 0.0f && Math::Abs(InShadowFarPlane - ShadowNearPlane) >= 0.1f)
    {
        ShadowFarPlane = InShadowFarPlane;
        CalculateMatrices();
    }
}

const FVector3& FPointLightComponent::GetPosition() const
{
    CHECK(GetActorOwner() != nullptr);
    return GetActorOwner()->GetWorldTransform().GetTranslation();
}

void FPointLightComponent::CalculateMatrices()
{
    if (!bCastShadows || !GetActorOwner())
    {
        return;
    }

    const FVector3 Directions[6] =
    {
        { FVector3( 1.0f, 0.0f,  0.0f) },
        { FVector3(-1.0f, 0.0f,  0.0f) },
        { FVector3( 0.0f, 1.0f,  0.0f) },
        { FVector3( 0.0f, -1.0f, 0.0f) },
        { FVector3( 0.0f, 0.0f,  1.0f) },
        { FVector3( 0.0f, 0.0f, -1.0f) },
    };

    const FVector3 UpVectors[6] =
    {
        { FVector3(0.0f, 1.0f, 0.0f) },
        { FVector3(0.0f, 1.0f, 0.0f) },
        { FVector3(0.0f, 0.0f, -1.0f) },
        { FVector3(0.0f, 0.0f, 1.0f) },
        { FVector3(0.0f, 1.0f, 0.0f) },
        { FVector3(0.0f, 1.0f, 0.0f) },
    };

    const FVector3& Position = GetPosition();
    for (uint32 Face = 0; Face < 6; ++Face)
    {
        const FMatrix4 LightProjection = FMatrix4::PerspectiveProjection(Math::Constants::HalfPI, 1.0f, ShadowNearPlane, ShadowFarPlane);
        const FMatrix4 LightView       = FMatrix4::LookTo(Position, Directions[Face], UpVectors[Face]);

        ViewMatrices[Face]     = LightView;
        ProjMatrices[Face]     = LightProjection;
        ViewProjMatrices[Face] = LightView * LightProjection;
    }
}
