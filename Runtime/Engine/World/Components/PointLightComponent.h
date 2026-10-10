#pragma once
#include "Core/Math/Matrix4.h"
#include "Engine/World/Components/LightComponent.h"

class ENGINE_API FPointLightComponent : public FLightComponent
{
public:
    FOBJECT_DECLARE_CLASS(FPointLightComponent, FLightComponent);

    FPointLightComponent(const FObjectInitializer& ObjectInitializer);
    ~FPointLightComponent();

    // FActorComponent Interface
    virtual void Tick(float DeltaTime) override;

    void UpdateShadowMatrices();
    void SetShadowNearPlane(float InShadowNearPlane);
    void SetShadowFarPlane(float InShadowFarPlane);

    const FVector3& GetPosition() const;

    FORCEINLINE const FMatrix4& GetViewProjectionMatrix(uint32 Index) const
    {
        CHECK(Index < 6);
        return ViewProjMatrices[Index];
    }

    FORCEINLINE const FMatrix4& GetViewMatrix(uint32 Index) const
    {
        CHECK(Index < 6);
        return ViewMatrices[Index];
    }

    FORCEINLINE const FMatrix4& GetProjectionMatrix(uint32 Index) const
    {
        CHECK(Index < 6);
        return ProjMatrices[Index];
    }

private:
    void CalculateMatrices();

    FMatrix4 ViewProjMatrices[6];
    FMatrix4 ViewMatrices[6];
    FMatrix4 ProjMatrices[6];
};
