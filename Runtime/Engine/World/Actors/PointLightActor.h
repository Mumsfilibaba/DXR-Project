#pragma once
#include "Engine/World/Actors/Actor.h"

class FPointLightComponent;

class ENGINE_API FPointLightActor : public FActor
{
public:
    FOBJECT_DECLARE_CLASS(FPointLightActor, FActor);

    FPointLightActor(const FObjectInitializer& ObjectInitializer);
    ~FPointLightActor();

    void Initialize(const Vector3& InPosition, bool bInShadowCaster);

    FPointLightComponent* GetLightComponent() const
    {
        return LightComponent;
    }

private:
    FPointLightComponent* LightComponent;
};
