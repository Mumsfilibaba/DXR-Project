#pragma once
#include "Core/CoreObject.h"

class Light : public CoreObject
{
    CORE_OBJECT(Light, CoreObject);

public:
    Light();
    virtual ~Light() = default;

    void SetColor(const XMFLOAT3& InColor);
    void SetColor(float R, float G, float B);
    
    void SetIntensity(float InIntensity);

    void SetShadowBias(float InShadowBias)
    {
        ShadowBias = InShadowBias;
    }

    void SetMaxShadowBias(float InShadowBias)
    {
        MaxShadowBias = InShadowBias;
    }

    float GetIntensity() const { return Intensity; }

    const XMFLOAT3& GetColor() const { return Color; }

    float GetShadowNearPlane() const { return ShadowNearPlane; }
    float GetShadowFarPlane() const { return ShadowFarPlane; }

    float GetShadowBias() const { return ShadowBias; }
    float GetMaxShadowBias() const { return MaxShadowBias; }

protected:
    XMFLOAT3 Color;
    float Intensity = 1.0f;
    float ShadowNearPlane;
    float ShadowFarPlane;
    float ShadowBias;
    float MaxShadowBias;
};