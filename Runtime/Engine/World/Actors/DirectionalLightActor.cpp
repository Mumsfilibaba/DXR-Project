#include "Engine/World/Actors/DirectionalLightActor.h"
#include "Engine/World/Components/DirectionalLightComponent.h"

FOBJECT_IMPLEMENT_CLASS(FDirectionalLightActor);

FDirectionalLightActor::FDirectionalLightActor(const FObjectInitializer& ObjectInitializer)
    : FActor(ObjectInitializer)
    , LightComponent(nullptr)
{
    SetName("Directional Light");
    
    LightComponent = NewObject<FDirectionalLightComponent>();
    AddComponent(LightComponent);
}

FDirectionalLightActor::~FDirectionalLightActor()
{
}

void FDirectionalLightActor::Initialize(const Vector3& InRotation)
{
    GetTransform().SetRotation(InRotation);
}
