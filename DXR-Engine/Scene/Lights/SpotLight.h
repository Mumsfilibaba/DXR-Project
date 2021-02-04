#pragma once
#include "Light.h"

class SpotLight : public Light
{
    CORE_OBJECT(SpotLight, Light);

public:
    SpotLight();
    ~SpotLight();

    void SetConeAngle(Float InConeAngle);

    FORCEINLINE Float GetConeAngle() const
    {
        return ConeAngle;
    }

private:
    Float ConeAngle;
};