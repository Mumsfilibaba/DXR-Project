#pragma once
#include "Light.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FSpotLight

class ENGINE_API FSpotLight 
    : public FLight
{
    CORE_OBJECT(FSpotLight, FLight);

public:
    FSpotLight();
    ~FSpotLight();

    void SetConeAngle(float InConeAngle);

    FORCEINLINE float GetConeAngle() const
    {
        return ConeAngle;
    }

private:
    float ConeAngle;
};