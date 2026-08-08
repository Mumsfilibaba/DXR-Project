#pragma once
#include "Engine/World/Actors/Actor.h"

class FSpotLightComponent;

class ENGINE_API FSpotLightActor : public FActor
{
public:
    FOBJECT_DECLARE_CLASS(FSpotLightActor, FActor);

    FSpotLightActor(const FObjectInitializer& ObjectInitializer);
    ~FSpotLightActor();

    virtual const CHAR* GetTypeLabel() const override
    {
        return "SpotLight";
    }

    FSpotLightComponent* GetLightComponent() const
    {
        return LightComponent;
    }

private:
    FSpotLightComponent* LightComponent;
};
