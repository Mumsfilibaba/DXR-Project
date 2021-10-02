#pragma once
#include "Light.h"

class CSpotLight : public CLight
{
    CORE_OBJECT( CSpotLight, CLight );

public:
    CSpotLight();
    ~CSpotLight();

    void SetConeAngle( float InConeAngle );

    FORCEINLINE float GetConeAngle() const
    {
        return ConeAngle;
    }

private:
    float ConeAngle;
};