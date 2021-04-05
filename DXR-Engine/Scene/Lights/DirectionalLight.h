#pragma once
#include "Light.h"

#define NUM_SHADOW_CASCADES (4)

class DirectionalLight : public Light
{
    CORE_OBJECT(DirectionalLight, Light);

public:
    DirectionalLight();
    ~DirectionalLight();

    void UpdateCascades(class Camera& Camera);

    // Rotation in Radians
    void SetRotation(const XMFLOAT3& InRotation);
    void SetRotation(float x, float y, float z);

    void SetLookAt(const XMFLOAT3& InInLookAt);
    void SetLookAt(float x, float y, float z);

    void SetShadowNearPlane(float InShadowNearPlane);
    void SetShadowFarPlane(float InShadowFarPlane);

    void SetCascadeSplitLambda(float InCascadeSplitLambda) { CascadeSplitLambda = InCascadeSplitLambda; }

    const XMFLOAT3& GetDirection() const { return Direction; }

    const XMFLOAT3& GetUp() const { return Up; }

    const XMFLOAT3& GetRotation() const { return Rotation; }

    const XMFLOAT3& GetPosition() const { return Position; }

    const XMFLOAT3& GetLookAt() const { return LookAt; }

    const XMFLOAT4X4& GetMatrix(uint32 CascadeIndex) const 
    {
        Assert(CascadeIndex < NUM_SHADOW_CASCADES);
        return Matrices[CascadeIndex];
    }

    const XMFLOAT4X4& GetViewMatrix(uint32 CascadeIndex) const
    {
        Assert(CascadeIndex < NUM_SHADOW_CASCADES);
        return ViewMatrices[CascadeIndex];
    }

    const XMFLOAT4X4& GetProjectionMatrix(uint32 CascadeIndex) const 
    {
        Assert(CascadeIndex < NUM_SHADOW_CASCADES);
        return ProjectionMatrices[CascadeIndex];
    }

    float GetCascadeSplitLambda() const { return CascadeSplitLambda; }

    float GetCascadeSplit(uint32 CascadeIndex) const { return CascadeSplits[CascadeIndex]; }

private:
    XMFLOAT3 Direction;
    XMFLOAT3 Up;
    XMFLOAT3 Rotation;
    XMFLOAT3 LookAt;
    XMFLOAT3 Position;

    XMFLOAT4X4 ViewMatrices[NUM_SHADOW_CASCADES];
    XMFLOAT4X4 ProjectionMatrices[NUM_SHADOW_CASCADES];
    XMFLOAT4X4 Matrices[NUM_SHADOW_CASCADES];

    float CascadeSplits[NUM_SHADOW_CASCADES];

    float CascadeSplitLambda = 0.95f;
};