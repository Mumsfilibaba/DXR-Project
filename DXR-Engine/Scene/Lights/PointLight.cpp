#include "PointLight.h"

#include "Rendering/Renderer.h"

PointLight::PointLight()
    : Light()
    , Matrices()
    , Position( 0.0f, 0.0f, 0.0f )
{
    CORE_OBJECT_INIT();

    CalculateMatrices();
}

void PointLight::SetPosition( const CVector3& InPosition )
{
    Position = InPosition;
    CalculateMatrices();
}

void PointLight::SetPosition( float x, float y, float z )
{
    SetPosition( CVector3( x, y, z ) );
}

void PointLight::SetShadowNearPlane( float InShadowNearPlane )
{
    if ( InShadowNearPlane > 0.0f )
    {
        if ( abs( ShadowFarPlane - InShadowNearPlane ) >= 0.1f )
        {
            ShadowNearPlane = InShadowNearPlane;
            CalculateMatrices();
        }
    }
}

void PointLight::SetShadowFarPlane( float InShadowFarPlane )
{
    if ( InShadowFarPlane > 0.0f )
    {
        if ( abs( InShadowFarPlane - ShadowNearPlane ) >= 0.1f )
        {
            ShadowFarPlane = InShadowFarPlane;
            CalculateMatrices();
        }
    }
}

void PointLight::CalculateMatrices()
{
    if ( !ShadowCaster )
    {
        return;
    }

    CVector3 Directions[6] =
    {
        { CVector3( 1.0f,  0.0f,  0.0f ) },
        { CVector3( -1.0f,  0.0f,  0.0f ) },
        { CVector3( 0.0f,  1.0f,  0.0f ) },
        { CVector3( 0.0f, -1.0f,  0.0f ) },
        { CVector3( 0.0f,  0.0f,  1.0f ) },
        { CVector3( 0.0f,  0.0f, -1.0f ) },
    };

    CVector3 UpVectors[6] =
    {
        { CVector3( 0.0f, 1.0f,  0.0f ) },
        { CVector3( 0.0f, 1.0f,  0.0f ) },
        { CVector3( 0.0f, 0.0f, -1.0f ) },
        { CVector3( 0.0f, 0.0f,  1.0f ) },
        { CVector3( 0.0f, 1.0f,  0.0f ) },
        { CVector3( 0.0f, 1.0f,  0.0f ) },
    };

    for ( uint32 i = 0; i < 6; i++ )
    {
        CMatrix4 LightProjection = CMatrix4::PerspectiveProjection( NMath::PI_F / 2.0f, 1.0f, ShadowNearPlane, ShadowFarPlane );
        CMatrix4 LightView = CMatrix4::LookTo( Position, Directions[i], UpVectors[i] );
        ViewMatrices[i] = LightView.Transpose();
        Matrices[i] = (LightView * LightProjection).Transpose();
    }
}
