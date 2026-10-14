#include "Engine/World/Actors/SkyLightActor.h"
#include "Engine/World/Components/SkyLightComponent.h"

FOBJECT_IMPLEMENT_CLASS(FSkyLightActor);

FSkyLightActor::FSkyLightActor(const FObjectInitializer& ObjectInitializer)
    : FActor(ObjectInitializer)
    , LightComponent(nullptr)
{
    SetName(TEXT("Sky Light"));
    
    LightComponent = NewObject<FSkyLightComponent>();
    AddComponent(LightComponent);
}

FSkyLightActor::~FSkyLightActor()
{
}

void FSkyLightActor::Initialize(const TSharedPtr<FEnvironment>& InEnvironment)
{
    LightComponent->SetSource(ESkyLightSource::SpecifiedEnvironment);
    LightComponent->SetEnvironment(InEnvironment);
}
