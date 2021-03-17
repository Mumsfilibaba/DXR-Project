#pragma once
#include "Light.h"

class DirectionalLight : public Light
{
    CORE_OBJECT(DirectionalLight, Light);

public:
    DirectionalLight();
    ~DirectionalLight();

    // Rotation in Radians
    void SetRotation(const XMFLOAT3& InRotation);
    void SetRotation(float x, float y, float z);

    void SetLookAt(const XMFLOAT3& InInLookAt);
    void SetLookAt(float x, float y, float z);

    void SetShadowNearPlane(float InShadowNearPlane);
    void SetShadowFarPlane(float InShadowFarPlane);

    const XMFLOAT3& GetDirection() const { return Direction; }

    const XMFLOAT3& GetRotation() const { return Rotation; }

    const XMFLOAT3& GetShadowMapPosition() const { return ShadowMapPosition; }

    const XMFLOAT3& GetLookAt() const { return LookAt; }

    const XMFLOAT4X4& GetMatrix() const { return Matrix; }

private:
    void CalculateMatrix();

    XMFLOAT3   Direction;
    XMFLOAT3   Rotation;
    XMFLOAT3   LookAt;
    XMFLOAT3   ShadowMapPosition;
    XMFLOAT4X4 Matrix;
};