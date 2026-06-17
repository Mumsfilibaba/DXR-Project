#include "Engine/World/Lights/PointLight.h"

FOBJECT_IMPLEMENT_CLASS(FPointLight);

FPointLight::FPointLight(const FObjectInitializer& ObjectInitializer)
    : FLight(ObjectInitializer, 1.0f, 30.0f)
    , ViewProjMatrices()
    , ViewMatrices()
    , ProjMatrices()
    , Position(0.0f, 0.0f, 0.0f)
    , bShadowCaster(false)
{
    CalculateMatrices();
}

FPointLight::~FPointLight()
{
}

void FPointLight::SetPosition(const Vector3& InPosition)
{
    Position = InPosition;
    CalculateMatrices();
}

void FPointLight::SetShadowNearPlane(float InShadowNearPlane)
{
    if (InShadowNearPlane > 0.0f)
    {
        if (Math::Abs(ShadowFarPlane - InShadowNearPlane) >= 0.1f)
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
        if (Math::Abs(InShadowFarPlane - ShadowNearPlane) >= 0.1f)
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

    const Vector3 Directions[6] =
    {
        { Vector3( 1.0f,  0.0f,  0.0f) },
        { Vector3(-1.0f,  0.0f,  0.0f) },
        { Vector3( 0.0f,  1.0f,  0.0f) },
        { Vector3( 0.0f, -1.0f,  0.0f) },
        { Vector3( 0.0f,  0.0f,  1.0f) },
        { Vector3( 0.0f,  0.0f, -1.0f) },
    };

    const Vector3 UpVectors[6] =
    {
        { Vector3(0.0f, 1.0f,  0.0f) },
        { Vector3(0.0f, 1.0f,  0.0f) },
        { Vector3(0.0f, 0.0f, -1.0f) },
        { Vector3(0.0f, 0.0f,  1.0f) },
        { Vector3(0.0f, 1.0f,  0.0f) },
        { Vector3(0.0f, 1.0f,  0.0f) },
    };

    for (uint32 Face = 0; Face < 6; ++Face)
    {
        const Matrix4 LightProjection = Matrix4::PerspectiveProjection(Math::Constants::HalfPI, 1.0f, ShadowNearPlane, ShadowFarPlane);
        const Matrix4 LightView       = Matrix4::LookTo(Position, Directions[Face], UpVectors[Face]);

        ViewMatrices[Face]     = LightView;
        ProjMatrices[Face]     = LightProjection;
        ViewProjMatrices[Face] = LightView * LightProjection;
    }
}
