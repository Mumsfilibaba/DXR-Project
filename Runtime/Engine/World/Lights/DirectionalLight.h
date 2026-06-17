#pragma once
#include "Core/Math/Matrix4.h"
#include "Engine/World/Lights/Light.h"

#define NUM_SHADOW_CASCADES (4)

class ENGINE_API FDirectionalLight : public FLight
{
public:
    FOBJECT_DECLARE_CLASS(FDirectionalLight, FLight);

    FDirectionalLight(const FObjectInitializer& ObjectInitializer);
    ~FDirectionalLight();

    // Rotation in Radians
    void SetRotation(const Vector3& InRotation);

    // Lambda for determine the splits of the shadow-cascades
    void SetCascadeSplitLambda(float InCascadeSplitLambda);

    // Offset from the calculated camera position, that then becomes the point-of-view of the shadow-cascade
    void SetShadowPositionOffset(float InShadowPositionOffset);

    // Area of the light-source
    void SetLightArea(float InLightArea);

    FORCEINLINE const Vector3& GetDirectionVector() const
    {
        return Direction;
    }

    FORCEINLINE const Vector3& GetRotation() const
    {
        return Rotation;
    }

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
    Vector3 Direction;
    Vector3 Rotation;
    float   ShadowPositionOffset;
    float   CascadeSplitLambda;
    float   LightArea;
};
