#pragma once
#include "Light.h"

#include "Core/Math/Matrix4.h"

#define NUM_SHADOW_CASCADES (4)

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FDirectionalLight

class ENGINE_API FDirectionalLight 
    : public FLight
{
    CORE_OBJECT(FDirectionalLight, FLight);

public:
    FDirectionalLight();
    ~FDirectionalLight() = default;

    void Tick(class FCamera& Camera);

    // Rotation in Radians
    void SetRotation(const FVector3& InRotation);
    void SetCascadeSplitLambda(float InCascadeSplitLambda);
    void SetSize(float InSize);

    FORCEINLINE const FVector3& GetDirection() const
    {
        return Direction;
    }

    FORCEINLINE const FVector3& GetUp() const
    {
        return UpVector;
    }

    FORCEINLINE const FVector3& GetRotation() const
    {
        return Rotation;
    }

    FORCEINLINE const FVector3& GetPosition() const
    {
        return Position;
    }

    FORCEINLINE const FVector3& GetLookAt() const
    {
        return LookAt;
    }

    FORCEINLINE const FMatrix4& GetShadowMatrix() const
    {
        return ShadowMatrix;
    }

    FORCEINLINE const FMatrix4& GetViewMatrix() const
    {
        return ViewMatrix;
    }

    FORCEINLINE const FMatrix4& GetProjectionMatrix() const
    {
        return ProjectionMatrix;
    }

    FORCEINLINE float GetCascadeSplitLambda() const
    {
        return CascadeSplitLambda;
    }

    FORCEINLINE float GetSize() const
    {
        return Size;
    }

private:
    FVector3 Direction;
    FVector3 Rotation;
    FVector3 UpVector;
    FVector3 LookAt;
    FVector3 Position;

    FMatrix4 ShadowMatrix;
    FMatrix4 ViewMatrix;
    FMatrix4 ProjectionMatrix;

    float CascadeSplitLambda;
    float Size;
};