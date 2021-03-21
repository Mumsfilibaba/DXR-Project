#pragma once
#include "Light.h"

class SpotLight : public Light
{
    CORE_OBJECT(SpotLight, Light);

public:
    SpotLight();
    ~SpotLight();

    void SetConeAngle(float InConeAngle);

    FORCEINLINE float GetConeAngle() const
    {
        return ConeAngle;
    }

private:
    float ConeAngle;
};