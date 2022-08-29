#include "PointLight.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FPointLight

FPointLight::FPointLight()
    : FLight()
    , Matrices()
    , Position(0.0f, 0.0f, 0.0f)
{
    CORE_OBJECT_INIT();

    CalculateMatrices();
}

void FPointLight::SetPosition(const FVector3& InPosition)
{
    Position = InPosition;
    CalculateMatrices();
}

void FPointLight::SetShadowNearPlane(float InShadowNearPlane)
{
    if (InShadowNearPlane > 0.0f)
    {
        if (NMath::Abs(ShadowFarPlane - InShadowNearPlane) >= 0.1f)
        {
            ShadowNearPlane = InShadowNearPlane;
            CalculateMatrices();
        }
    }
}

void FPointLight::SetShadowFarPlane(float InShadowFarPlane)
{
    if (InShadowFarPlane > 0.0f)
    {
        if (NMath::Abs(InShadowFarPlane - ShadowNearPlane) >= 0.1f)
        {
            ShadowFarPlane = InShadowFarPlane;
            CalculateMatrices();
        }
    }
}

void FPointLight::CalculateMatrices()
{
    if (!bShadowCaster)
    {
        return;
    }

    const FVector3 Directions[6] =
    {
        { FVector3( 1.0f,  0.0f,  0.0f) },
        { FVector3(-1.0f,  0.0f,  0.0f) },
        { FVector3( 0.0f,  1.0f,  0.0f) },
        { FVector3( 0.0f, -1.0f,  0.0f) },
        { FVector3( 0.0f,  0.0f,  1.0f) },
        { FVector3( 0.0f,  0.0f, -1.0f) },
    };

    const FVector3 UpVectors[6] =
    {
        { FVector3(0.0f, 1.0f,  0.0f) },
        { FVector3(0.0f, 1.0f,  0.0f) },
        { FVector3(0.0f, 0.0f, -1.0f) },
        { FVector3(0.0f, 0.0f,  1.0f) },
        { FVector3(0.0f, 1.0f,  0.0f) },
        { FVector3(0.0f, 1.0f,  0.0f) },
    };

    for (uint32 Face = 0; Face < 6; ++Face)
    {
        const FMatrix4 LightProjection = FMatrix4::PerspectiveProjection(NMath::kPI_f / 2.0f, 1.0f, ShadowNearPlane, ShadowFarPlane);
        const FMatrix4 LightView       = FMatrix4::LookTo(Position, Directions[Face], UpVectors[Face]);
        ViewMatrices[Face] = LightView.Transpose();
        ProjMatrices[Face] = LightProjection;
        Matrices[Face]     = (LightView * LightProjection).Transpose();
    }
}
