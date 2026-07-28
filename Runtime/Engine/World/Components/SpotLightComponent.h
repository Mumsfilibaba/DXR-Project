#pragma once
#include "Engine/World/Components/LightComponent.h"

class ENGINE_API FSpotLightComponent : public FLightComponent
{
public:
    FOBJECT_DECLARE_CLASS(FSpotLightComponent, FLightComponent);

    FSpotLightComponent(const FObjectInitializer& ObjectInitializer);
    ~FSpotLightComponent();

    void SetConeAngle(float InConeAngle);

    float GetConeAngle() const
    {
        return ConeAngle;
    }

private:
    float ConeAngle;
};
