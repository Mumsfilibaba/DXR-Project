#include "Engine/World/Actors/LightProbeActor.h"
#include "Engine/World/Components/LightProbeComponent.h"

FOBJECT_IMPLEMENT_CLASS(FLightProbeActor);

FLightProbeActor::FLightProbeActor(const FObjectInitializer& ObjectInitializer)
    : FActor(ObjectInitializer)
    , LightProbeComponent(nullptr)
{
    SetName(TEXT("Light Probe"));
    
    LightProbeComponent = NewObject<FLightProbeComponent>();
    AddComponent(LightProbeComponent);
}

FLightProbeActor::~FLightProbeActor()
{
}

void FLightProbeActor::Initialize(const FVector3& InPosition, const TSharedPtr<FEnvironment>& InEnvironment)
{
    GetTransform().SetTranslation(InPosition);
    LightProbeComponent->SetEnvironment(InEnvironment);
}
