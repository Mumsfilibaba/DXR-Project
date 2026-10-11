#include "Engine/World/Actors/DirectionalLightActor.h"
#include "Engine/World/Components/DirectionalLightComponent.h"

FOBJECT_IMPLEMENT_CLASS(FDirectionalLightActor);

FDirectionalLightActor::FDirectionalLightActor(const FObjectInitializer& ObjectInitializer)
    : FActor(ObjectInitializer)
    , LightComponent(nullptr)
{
    SetName(TEXT("Directional Light"));
    
    LightComponent = NewObject<FDirectionalLightComponent>();
    AddComponent(LightComponent);
}

FDirectionalLightActor::~FDirectionalLightActor()
{
}

void FDirectionalLightActor::Initialize(const FVector3& InRotation)
{
    GetTransform().SetRotation(InRotation);
}
