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

    using FActor::Initialize;
    void Initialize(const Vector3& InPosition, const FRHITextureRef& InCubeMap);

    virtual const CHAR* GetTypeLabel() const override
    {
        return "LightProbe";
    }

    FLightProbeComponent* GetLightProbeComponent() const
    {
        return LightProbeComponent;
    }

private:
    FLightProbeComponent* LightProbeComponent;
};
