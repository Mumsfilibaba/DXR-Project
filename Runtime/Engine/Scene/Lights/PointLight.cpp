#include "PointLight.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FPointLight

FPointLight::FPointLight()
    : CLight()
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

void FPointLight::SetPosition(float x, float y, float z)
{
    SetPosition(FVector3(x, y, z));
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

    FVector3 Directions[6] =
    {
        { FVector3(1.0f,  0.0f,  0.0f) },
        { FVector3(-1.0f,  0.0f,  0.0f) },
        { FVector3(0.0f,  1.0f,  0.0f) },
        { FVector3(0.0f, -1.0f,  0.0f) },
        { FVector3(0.0f,  0.0f,  1.0f) },
        { FVector3(0.0f,  0.0f, -1.0f) },
    };

    FVector3 UpVectors[6] =
    {
        { FVector3(0.0f, 1.0f,  0.0f) },
        { FVector3(0.0f, 1.0f,  0.0f) },
        { FVector3(0.0f, 0.0f, -1.0f) },
        { FVector3(0.0f, 0.0f,  1.0f) },
        { FVector3(0.0f, 1.0f,  0.0f) },
        { FVector3(0.0f, 1.0f,  0.0f) },
    };

    for (uint32 i = 0; i < 6; i++)
    {
        FMatrix4 LightProjection = FMatrix4::PerspectiveProjection(NMath::kPI_f / 2.0f, 1.0f, ShadowNearPlane, ShadowFarPlane);
        FMatrix4 LightView = FMatrix4::LookTo(Position, Directions[i], UpVectors[i]);
        ViewMatrices[i] = LightView.Transpose();
        Matrices[i] = (LightView * LightProjection).Transpose();
    }
}
