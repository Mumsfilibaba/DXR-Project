#pragma once
#include "Core/Math/Vector3.h"
#include "Engine/World/Components/SceneComponent.h"

class ENGINE_API FLightComponent : public FSceneComponent
{
public:
    FOBJECT_DECLARE_CLASS(FLightComponent, FSceneComponent);

    FLightComponent(const FObjectInitializer& ObjectInitializer);
    virtual ~FLightComponent();

    void SetColor(const Vector3& InColor);
    void SetIntensity(float InIntensity);
    void SetCastShadows(bool bInCastShadows);
    void SetShadowNearPlane(float InShadowNearPlane);
    void SetShadowFarPlane(float InShadowFarPlane);
    void SetShadowBias(float InShadowBias);

    FORCEINLINE const Vector3& GetColor() const
    {
        return Color;
    }

    FORCEINLINE float GetIntensity() const
    {
        return Intensity;
    }

    FORCEINLINE bool CastsShadows() const
    {
        return bCastShadows;
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
    FLightComponent(const FObjectInitializer& ObjectInitializer, float InShadowNearPlane, float InShadowFarPlane);

    Vector3 Color;
    float   Intensity;
    float   ShadowNearPlane;
    float   ShadowFarPlane;
    float   ShadowBias;
    bool    bCastShadows;
};
