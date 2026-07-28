#include "Engine/World/Actors/PointLightActor.h"
#include "Engine/World/Components/PointLightComponent.h"

FOBJECT_IMPLEMENT_CLASS(FPointLightActor);

FPointLightActor::FPointLightActor(const FObjectInitializer& ObjectInitializer)
    : FActor(ObjectInitializer)
    , LightComponent(nullptr)
{
    SetName("Point Light");
    
    LightComponent = NewObject<FPointLightComponent>();
    AddComponent(LightComponent);
}

FPointLightActor::~FPointLightActor()
{
}

void FPointLightActor::Initialize(const Vector3& InPosition, bool bInShadowCaster)
{
    GetTransform().SetTranslation(InPosition);
    LightComponent->SetShadowCaster(bInShadowCaster);
}
