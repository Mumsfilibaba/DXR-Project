#pragma once
#include "Light.h"

class PointLight : public Light
{
    CORE_OBJECT(PointLight, Light);

public:
    PointLight();
    ~PointLight() = default;

    void SetPosition(const XMFLOAT3& InPosition);
    void SetPosition(Float x, Float y, Float z);

    void SetShadowNearPlane(Float InShadowNearPlane);
    void SetShadowFarPlane(Float InShadowFarPlane);

    const XMFLOAT3& GetPosition() const { return Position; }

    const XMFLOAT4X4& GetMatrix(UInt32 Index) const
    {
        VALIDATE(Index < 6);
        return Matrices[Index];
    }

    const XMFLOAT4X4& GetViewMatrix(UInt32 Index) const
    {
        VALIDATE(Index < 6);
        return ViewMatrices[Index];
    }

    const XMFLOAT4X4& GetProjectionMatrix(UInt32 Index) const
    {
        VALIDATE(Index < 6);
        return ProjMatrices[Index];
    }

private:
    void CalculateMatrices();

    XMFLOAT4X4 Matrices[6];
    XMFLOAT4X4 ViewMatrices[6];
    XMFLOAT4X4 ProjMatrices[6];
    XMFLOAT3   Position;
};