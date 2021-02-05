#pragma once
#include "BaseLight.h"

class Light : public BaseLight
{
    CORE_OBJECT(Light, BaseLight);

public:
    Light();
    ~Light() = default;

    void SetColor(const XMFLOAT3& InColor);
    void SetColor(Float R, Float G, Float B);
    
    void SetIntensity(Float InIntensity);

    void SetShadowBias(Float InShadowBias)
    {
        ShadowBias = InShadowBias;
    }

    void SetMaxShadowBias(Float InShadowBias)
    {
        MaxShadowBias = InShadowBias;
    }

    Float GetIntensity() const { return Intensity; }

    const XMFLOAT3& GetColor() const { return Color; }

    Float GetShadowNearPlane() const { return ShadowNearPlane; }
    Float GetShadowFarPlane() const { return ShadowFarPlane; }

    Float GetShadowBias() const { return ShadowBias; }
    Float GetMaxShadowBias() const { return MaxShadowBias; }

protected:
    XMFLOAT3 Color;
    Float Intensity = 1.0f;
    Float ShadowNearPlane;
    Float ShadowFarPlane;
    Float ShadowBias;
    Float MaxShadowBias;
};