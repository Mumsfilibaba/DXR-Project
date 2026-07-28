#pragma once
#include "Engine/World/Actors/Actor.h"
#include "RHI/RHITexture.h"

class FLightProbeComponent;

class ENGINE_API FLightProbeActor : public FActor
{
public:
    FOBJECT_DECLARE_CLASS(FLightProbeActor, FActor);

    FLightProbeActor(const FObjectInitializer& ObjectInitializer);
    ~FLightProbeActor();

    void Initialize(const Vector3& InPosition, const FRHITextureRef& InCubeMap);

    FLightProbeComponent* GetLightProbeComponent() const
    {
        return LightProbeComponent;
    }

private:
    FLightProbeComponent* LightProbeComponent;
};
