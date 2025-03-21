#pragma once
#include "Core/Math/Vector3.h"
#include "Engine/Core/Object.h"

class ENGINE_API FLight : public FObject
{
public:
    FOBJECT_DECLARE_CLASS(FLight, FObject);

    FLight(const FObjectInitializer& ObjectInitializer);
    virtual ~FLight() = default;

    void SetColor(const FVector3& InColor);
    void SetIntensity(float InIntensity);

    FORCEINLINE void SetShadowBias(float InShadowBias)
    {
        ShadowBias = InShadowBias;
    }

    FORCEINLINE float GetIntensity() const
    {
        return Intensity;
    }

    FORCEINLINE const FVector3& GetColor() const
    {
        return Color;
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
    FVector3 Color;
    float    Intensity;
    float    ShadowNearPlane;
    float    ShadowFarPlane;
    float    ShadowBias;
};
