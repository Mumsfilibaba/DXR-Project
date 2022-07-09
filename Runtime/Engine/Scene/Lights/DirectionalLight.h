#pragma once
#include "Light.h"

#include "Core/Math/Matrix4.h"

#define NUM_SHADOW_CASCADES (4)

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FDirectionalLight

class ENGINE_API FDirectionalLight : public CLight
{
    CORE_OBJECT(FDirectionalLight, CLight);

public:
    FDirectionalLight();
    ~FDirectionalLight();

    void UpdateCascades(class CCamera& Camera);

    // Rotation in Radians
    void SetRotation(const FVector3& InRotation);
    void SetRotation(float x, float y, float z);

    void SetLookAt(const FVector3& InInLookAt);
    void SetLookAt(float x, float y, float z);

    FORCEINLINE void SetCascadeSplitLambda(float InCascadeSplitLambda)
    {
        CascadeSplitLambda = InCascadeSplitLambda;
    }

    FORCEINLINE void SetSize(float InSize)
    {
        Size = InSize;
    }

    FORCEINLINE const FVector3& GetDirection() const
    {
        return Direction;
    }

    FORCEINLINE const FVector3& GetUp() const
    {
        return Up;
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

    FORCEINLINE const FMatrix4& GetMatrix(uint32 CascadeIndex) const
    {
        Check(CascadeIndex < NUM_SHADOW_CASCADES);
        return Matrices[CascadeIndex];
    }

    FORCEINLINE const FMatrix4& GetViewMatrix(uint32 CascadeIndex) const
    {
        Check(CascadeIndex < NUM_SHADOW_CASCADES);
        return ViewMatrices[CascadeIndex];
    }

    FORCEINLINE const FMatrix4& GetProjectionMatrix(uint32 CascadeIndex) const
    {
        Check(CascadeIndex < NUM_SHADOW_CASCADES);
        return ProjectionMatrices[CascadeIndex];
    }

    FORCEINLINE float GetCascadeSplitLambda() const
    {
        return CascadeSplitLambda;
    }

    FORCEINLINE float GetCascadeSplit(uint32 CascadeIndex) const
    {
        return CascadeSplits[CascadeIndex];
    }

    FORCEINLINE float GetCascadeRadius(uint32 CascadeIndex) const
    {
        return CascadeRadius[CascadeIndex];
    }

    FORCEINLINE float GetSize() const
    {
        return Size;
    }

private:
    FVector3 Direction;
    FVector3 Up;
    FVector3 Rotation;
    FVector3 LookAt;
    FVector3 Position;

    FMatrix4 ViewMatrices[NUM_SHADOW_CASCADES];
    FMatrix4 ProjectionMatrices[NUM_SHADOW_CASCADES];
    FMatrix4 Matrices[NUM_SHADOW_CASCADES];

    float CascadeSplits[NUM_SHADOW_CASCADES];
    float CascadeRadius[NUM_SHADOW_CASCADES];

    float CascadeSplitLambda = 0.8f;

    float Size = 0.05f;
};