#include "Engine/World/Actors/SpotLightActor.h"
#include "Engine/World/Components/SpotLightComponent.h"

FOBJECT_IMPLEMENT_CLASS(FSpotLightActor);

FSpotLightActor::FSpotLightActor(const FObjectInitializer& ObjectInitializer)
    : FActor(ObjectInitializer)
    , LightComponent(nullptr)
{
    SetName("Spot Light");
    
    LightComponent = NewObject<FSpotLightComponent>();
    AddComponent(LightComponent);
}

FSpotLightActor::~FSpotLightActor()
{
}
