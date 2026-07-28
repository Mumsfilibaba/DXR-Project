#pragma once
#include "Core/Math/Matrix4.h"
#include "Engine/World/Components/LightComponent.h"

class ENGINE_API FPointLightComponent : public FLightComponent
{
public:
    FOBJECT_DECLARE_CLASS(FPointLightComponent, FLightComponent);

    FPointLightComponent(const FObjectInitializer& ObjectInitializer);
    ~FPointLightComponent();

    virtual void Tick(float DeltaTime) override;

    void UpdateShadowMatrices();
    void SetShadowNearPlane(float InShadowNearPlane);
    void SetShadowFarPlane(float InShadowFarPlane);
    void SetShadowCaster(bool bInShadowCaster);

    bool IsShadowCaster() const
    {
        return bShadowCaster;
    }

    const Vector3& GetPosition() const;

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
    bool    bShadowCaster;
};
