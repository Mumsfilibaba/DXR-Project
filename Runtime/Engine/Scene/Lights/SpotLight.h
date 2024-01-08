#pragma once
#include "Light.h"

class ENGINE_API FSpotLight : public FLight
{
public:
    FOBJECT_DECLARE_CLASS(FPointLight, FLight, ENGINE_API);

    FSpotLight(const FObjectInitializer& ObjectInitializer);
    ~FSpotLight() = default;

    void SetConeAngle(float InConeAngle);

    FORCEINLINE float GetConeAngle() const
    {
        return ConeAngle;
    }

private:
    float ConeAngle;
};
