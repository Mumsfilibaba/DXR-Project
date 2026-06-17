#pragma once
#include "Core/Math/Vector3.h"
#include "Engine/Core/Object.h"

class ENGINE_API FLight : public FObject
{
public:
    FOBJECT_DECLARE_CLASS(FLight, FObject);

    FLight(const FObjectInitializer& ObjectInitializer);
    FLight(const FObjectInitializer& ObjectInitializer, float InShadowNearPlane, float InShadowFarPlane);
    virtual ~FLight();

    // Set color of a light
    void SetColor(const Vector3& InColor);

    // Set intensity of the light
    void SetIntensity(float InIntensity);

    // Set near-plane for shadows
    void SetShadowNearPlane(float InShadowNearPlane);

    // Set near-plane for shadows
    void SetShadowFarPlane(float InShadowFarPlane);

    // Set shadow-bias
    void SetShadowBias(float InShadowBias);

    FORCEINLINE const Vector3& GetColor() const
    {
        return Color;
    }

    FORCEINLINE float GetIntensity() const
    {
        return Intensity;
    }

    FORCEINLINE float GetShadowNearPlane() const
    {
        return ShadowNearPlane;
    }

    FORCEINLINE float GetShadowFarPlane() const
    {
        return ShadowFarPlane;
    }

    FORCEINLINE float GetShadowBias() const
    {
        return ShadowBias;
    }

protected:
    Vector3 Color;
    float   Intensity;
    float   ShadowNearPlane;
    float   ShadowFarPlane;
    float   ShadowBias;
};
