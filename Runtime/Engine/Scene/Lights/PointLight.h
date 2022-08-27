#pragma once
#include "Light.h"

#include "Core/Math/Matrix4.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FPointLight

class ENGINE_API FPointLight 
    : public FLight
{
    CORE_OBJECT(FPointLight, FLight);

public:
    FPointLight();
    ~FPointLight() = default;

    void SetPosition(const FVector3& InPosition);
    void SetPosition(float x, float y, float z);

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

    FORCEINLINE const FVector3& GetPosition() const
    {
        return Position;
    }

    FORCEINLINE const FMatrix4& GetMatrix(uint32 Index) const
    {
        Check(Index < 6);
        return Matrices[Index];
    }

    FORCEINLINE const FMatrix4& GetViewMatrix(uint32 Index) const
    {
        Check(Index < 6);
        return ViewMatrices[Index];
    }

    FORCEINLINE const FMatrix4& GetProjectionMatrix(uint32 Index) const
    {
        Check(Index < 6);
        return ProjMatrices[Index];
    }

private:
    void CalculateMatrices();

    FMatrix4 Matrices[6];
    FMatrix4 ViewMatrices[6];
    FMatrix4 ProjMatrices[6];
    
    FVector3 Position;

    bool     bShadowCaster = false;
};