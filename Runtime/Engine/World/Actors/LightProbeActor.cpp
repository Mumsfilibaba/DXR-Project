#include "Engine/World/Actors/LightProbeActor.h"
#include "Engine/World/Components/LightProbeComponent.h"

FOBJECT_IMPLEMENT_CLASS(FLightProbeActor);

FLightProbeActor::FLightProbeActor(const FObjectInitializer& ObjectInitializer)
    : FActor(ObjectInitializer)
    , LightProbeComponent(nullptr)
{
    SetName("Light Probe");
    
    LightProbeComponent = NewObject<FLightProbeComponent>();
    AddComponent(LightProbeComponent);
}

FLightProbeActor::~FLightProbeActor()
{
}

void FLightProbeActor::Initialize(const Vector3& InPosition, const FRHITextureRef& InCubeMap)
{
    GetTransform().SetTranslation(InPosition);
    LightProbeComponent->SetCubeMap(InCubeMap);
}
