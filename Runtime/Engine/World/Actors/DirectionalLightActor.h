#pragma once
#include "Engine/World/Actors/Actor.h"

class FDirectionalLightComponent;

class ENGINE_API FDirectionalLightActor : public FActor
{
public:
    FOBJECT_DECLARE_CLASS(FDirectionalLightActor, FActor);

    FDirectionalLightActor(const FObjectInitializer& ObjectInitializer);
    ~FDirectionalLightActor();

    using FActor::Initialize;
    void Initialize(const Vector3& InRotation);

    virtual const CHAR* GetTypeLabel() const override
    {
        return "DirectionalLight";
    }

    FDirectionalLightComponent* GetLightComponent() const
    {
        return LightComponent;
    }

private:
    FDirectionalLightComponent* LightComponent;
};
