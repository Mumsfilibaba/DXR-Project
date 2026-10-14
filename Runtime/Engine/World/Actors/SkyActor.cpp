#include "Engine/World/Actors/SkyActor.h"
#include "Engine/World/Components/SkyboxComponent.h"
#include "Engine/World/Components/SkyLightComponent.h"

FOBJECT_IMPLEMENT_CLASS(FSkyActor);

FSkyActor::FSkyActor(const FObjectInitializer& ObjectInitializer)
    : FActor(ObjectInitializer)
    , SkyboxComponent(nullptr)
    , SkyLightComponent(nullptr)
{
    SetName(TEXT("Sky"));

    SkyboxComponent = NewObject<FSkyboxComponent>();
    AddComponent(SkyboxComponent);

    SkyLightComponent = NewObject<FSkyLightComponent>();
    SkyLightComponent->SetSource(ESkyLightSource::FollowSkybox);
    AddComponent(SkyLightComponent);
}

FSkyActor::~FSkyActor()
{
}

void FSkyActor::Initialize(const TSharedPtr<FEnvironment>& InEnvironment)
{
    SkyboxComponent->SetEnvironment(InEnvironment);
}
