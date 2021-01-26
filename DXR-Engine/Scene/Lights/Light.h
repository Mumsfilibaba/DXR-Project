#pragma once
#include "Core/CoreObject.h"

class Light : public CoreObject
{
    CORE_OBJECT(Light, CoreObject);

public:
    Light();
    virtual ~Light() = default;

    void SetColor(const XMFLOAT3& InColor);
    void SetColor(Float R, Float G, Float B);
    
    void SetIntensity(Float InIntensity);

    FORCEINLINE void SetShadowBias(Float InShadowBias)
    {
        ShadowBias = InShadowBias;
    }

    FORCEINLINE void SetMaxShadowBias(Float InShadowBias)
    {
        MaxShadowBias = InShadowBias;
    }

    FORCEINLINE Float GetIntensity() const
    {
        return Intensity;
    }

    FORCEINLINE const XMFLOAT3& GetColor() const
    {
        return Color;
    }

    FORCEINLINE Float GetShadowNearPlane() const
    {
        return ShadowNearPlane;
    }

    FORCEINLINE Float GetShadowFarPlane() const
    {
        return ShadowFarPlane;
    }

    FORCEINLINE Float GetShadowBias() const
    {
        return ShadowBias;
    }

    FORCEINLINE Float GetMaxShadowBias() const
    {
        return MaxShadowBias;
    }

protected:
    XMFLOAT3 Color;
    Float Intensity = 1.0f;
    Float ShadowNearPlane;
    Float ShadowFarPlane;
    Float ShadowBias;
    Float MaxShadowBias;
};