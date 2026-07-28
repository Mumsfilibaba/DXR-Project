#pragma once
#include "Core/Math/Matrix4.h"
#include "Engine/World/Components/LightComponent.h"

#define NUM_SHADOW_CASCADES (4)

class ENGINE_API FDirectionalLightComponent : public FLightComponent
{
public:
    FOBJECT_DECLARE_CLASS(FDirectionalLightComponent, FLightComponent);

    FDirectionalLightComponent(const FObjectInitializer& ObjectInitializer);
    ~FDirectionalLightComponent();

    void SetCascadeSplitLambda(float InCascadeSplitLambda);
    void SetShadowPositionOffset(float InShadowPositionOffset);
    void SetLightArea(float InLightArea);

    Vector3 GetDirectionVector() const;

    FORCEINLINE float GetShadowPositionOffset() const
    {
        return ShadowPositionOffset;
    }

    FORCEINLINE float GetCascadeSplitLambda() const
    {
        return CascadeSplitLambda;
    }

    FORCEINLINE float GetLightArea() const
    {
        return LightArea;
    }

private:
    float ShadowPositionOffset;
    float CascadeSplitLambda;
    float LightArea;
};
