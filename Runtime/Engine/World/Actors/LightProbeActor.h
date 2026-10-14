#pragma once
#include "Engine/World/Actors/Actor.h"
#include "Engine/Resources/Environment.h"

class FLightProbeComponent;

class ENGINE_API FLightProbeActor : public FActor
{
public:
    FOBJECT_DECLARE_CLASS(FLightProbeActor, FActor);

    FLightProbeActor(const FObjectInitializer& ObjectInitializer);
    ~FLightProbeActor();

    using FActor::Initialize;
    void Initialize(const FVector3& InPosition, const TSharedPtr<FEnvironment>& InEnvironment);

    virtual const CHAR_T* GetTypeLabel() const override
    {
        return TEXT("LightProbe");
    }

    FLightProbeComponent* GetLightProbeComponent() const
    {
        return LightProbeComponent;
    }

private:
    FLightProbeComponent* LightProbeComponent;
};
