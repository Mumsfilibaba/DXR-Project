#pragma once
#include "Light.h"

class ENGINE_API FSpotLight 
    : public FLight
{
    FOBJECT_BODY(FSpotLight, FLight);

public:
    FSpotLight();
    ~FSpotLight() = default;

    void SetConeAngle(float InConeAngle);

    FORCEINLINE float GetConeAngle() const
    {
        return ConeAngle;
    }

private:
    float ConeAngle;
};