#include "Core/Math/Math.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/PointLightComponent.h"
#include "Engine/World/World.h"

FOBJECT_IMPLEMENT_CLASS(FPointLightComponent);

FPointLightComponent::FPointLightComponent(const FObjectInitializer& ObjectInitializer)
    : FLightComponent(ObjectInitializer, 1.0f, 30.0f)
    , ViewProjMatrices()
    , ViewMatrices()
    , ProjMatrices()
    , bShadowCaster(false)
{
    SetTickable(true);
    CalculateMatrices();
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

void FPointLightComponent::SetShadowCaster(bool bInShadowCaster)
{
    if (bShadowCaster == bInShadowCaster)
    {
        return;
    }

    bShadowCaster = bInShadowCaster;

    if (FActor* OwnerActor = GetActorOwner())
    {
        if (FWorld* World = OwnerActor->GetWorld())
        {
            if (bShadowCaster)
            {
                World->AddSceneComponent(this);
            }
            else
            {
                World->RemoveSceneComponent(this);
            }
        }
    }

    CalculateMatrices();
}

const Vector3& FPointLightComponent::GetPosition() const
{
    CHECK(GetActorOwner() != nullptr);
    return GetActorOwner()->GetTransform().GetTranslation();
}

void FPointLightComponent::CalculateMatrices()
{
    if (!bShadowCaster)
    {
        return;
    }

    CHECK(GetActorOwner() != nullptr);

    const Vector3 Directions[6] =
    {
        { Vector3( 1.0f,  0.0f,  0.0f) },
        { Vector3(-1.0f,  0.0f,  0.0f) },
        { Vector3( 0.0f,  1.0f,  0.0f) },
        { Vector3( 0.0f, -1.0f,  0.0f) },
        { Vector3( 0.0f,  0.0f,  1.0f) },
        { Vector3( 0.0f,  0.0f, -1.0f) },
    };

    const Vector3 UpVectors[6] =
    {
        { Vector3(0.0f, 1.0f,  0.0f) },
        { Vector3(0.0f, 1.0f,  0.0f) },
        { Vector3(0.0f, 0.0f, -1.0f) },
        { Vector3(0.0f, 0.0f,  1.0f) },
        { Vector3(0.0f, 1.0f,  0.0f) },
        { Vector3(0.0f, 1.0f,  0.0f) },
    };

    const Vector3& Position = GetPosition();
    for (uint32 Face = 0; Face < 6; ++Face)
    {
        const Matrix4 LightProjection = Matrix4::PerspectiveProjection(Math::Constants::HalfPI, 1.0f, ShadowNearPlane, ShadowFarPlane);
        const Matrix4 LightView       = Matrix4::LookTo(Position, Directions[Face], UpVectors[Face]);

        ViewMatrices[Face]     = LightView;
        ProjMatrices[Face]     = LightProjection;
        ViewProjMatrices[Face] = LightView * LightProjection;
    }
}
