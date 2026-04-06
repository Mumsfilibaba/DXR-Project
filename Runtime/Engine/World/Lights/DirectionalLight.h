#pragma once
#include "Core/Math/Matrix4.h"
#include "Engine/World/Lights/Light.h"

#define NUM_SHADOW_CASCADES (4)

enum class ECascadeSplitMode : uint8
{
    AutoLambda = 0,
    Manual     = 1,
};

class ENGINE_API FDirectionalLight : public FLight
{
public:
    FOBJECT_DECLARE_CLASS(FDirectionalLight, FLight);

    FDirectionalLight(const FObjectInitializer& ObjectInitializer);
    ~FDirectionalLight();

    // Rotation in Radians
    void SetRotation(const FVector3& InRotation);

    // Lambda for determine the splits of the shadow-cascades
    void SetCascadeSplitLambda(float InCascadeSplitLambda);

    void SetCascadeSplitMode(ECascadeSplitMode InMode);
    void SetManualCascadeSplitDistance(int32 Index, float Distance);

    // Offset from the calculated camera position, that then becomes the point-of-view of the shadow-cascade
    void SetShadowPositionOffset(float InShadowPositionOffset);

    // Area of the light-source
    void SetLightArea(float InLightArea);

    FORCEINLINE const FVector3& GetDirectionVector() const
    {
        return Direction;
    }

    FORCEINLINE const FVector3& GetRotation() const
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

    FORCEINLINE ECascadeSplitMode GetCascadeSplitMode() const
    {
        return CascadeSplitMode;
    }

    FORCEINLINE float GetManualCascadeSplitDistance(int32 Index) const
    {
        return (Index >= 0 && Index < (NUM_SHADOW_CASCADES - 1)) ? ManualCascadeSplitDistances[Index] : 0.0f;
    }

    FORCEINLINE float GetLightArea() const
    {
        return LightArea;
    }

private:
    FVector3 Direction;
    FVector3 Rotation;
    float    ShadowPositionOffset;
    float    CascadeSplitLambda;
    ECascadeSplitMode CascadeSplitMode;
    float    ManualCascadeSplitDistances[NUM_SHADOW_CASCADES - 1];
    float    LightArea;
};
