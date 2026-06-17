#pragma once
#include "Core/Math/Matrix4.h"
#include "Engine/World/Lights/Light.h"

class ENGINE_API FPointLight : public FLight
{
public:
    FOBJECT_DECLARE_CLASS(FPointLight, FLight);

    FPointLight(const FObjectInitializer& ObjectInitializer);
    ~FPointLight();

    void SetPosition(const Vector3& InPosition);
    void SetShadowNearPlane(float InShadowNearPlane);
    void SetShadowFarPlane(float InShadowFarPlane);

    FORCEINLINE void SetShadowCaster(bool bInShadowCaster)
    {
        bShadowCaster = bInShadowCaster;
        CalculateMatrices();
    }

    FORCEINLINE bool IsShadowCaster() const
    {
        return bShadowCaster;
    }

    FORCEINLINE const Vector3& GetPosition() const
    {
        return Position;
    }

    FORCEINLINE const Matrix4& GetViewProjectionMatrix(uint32 Index) const
    {
        CHECK(Index < 6);
        return ViewProjMatrices[Index];
    }

    FORCEINLINE const Matrix4& GetViewMatrix(uint32 Index) const
    {
        CHECK(Index < 6);
        return ViewMatrices[Index];
    }

    FORCEINLINE const Matrix4& GetProjectionMatrix(uint32 Index) const
    {
        CHECK(Index < 6);
        return ProjMatrices[Index];
    }

private:
    void CalculateMatrices();

    Matrix4 ViewProjMatrices[6];
    Matrix4 ViewMatrices[6];
    Matrix4 ProjMatrices[6];
    Vector3 Position;
    bool    bShadowCaster;
};
