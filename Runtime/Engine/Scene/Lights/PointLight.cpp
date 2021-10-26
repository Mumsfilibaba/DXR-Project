#include "PointLight.h"

CPointLight::CPointLight()
    : CLight()
    , Matrices()
    , Position( 0.0f, 0.0f, 0.0f )
{
    CORE_OBJECT_INIT();

    CalculateMatrices();
}

void CPointLight::SetPosition( const CVector3& InPosition )
{
    Position = InPosition;
    CalculateMatrices();
}

void CPointLight::SetPosition( float x, float y, float z )
{
    SetPosition( CVector3( x, y, z ) );
}

void CPointLight::SetShadowNearPlane( float InShadowNearPlane )
{
    if ( InShadowNearPlane > 0.0f )
    {
        if ( NMath::Abs( ShadowFarPlane - InShadowNearPlane ) >= 0.1f )
        {
            ShadowNearPlane = InShadowNearPlane;
            CalculateMatrices();
        }
    }
}

void CPointLight::SetShadowFarPlane( float InShadowFarPlane )
{
    if ( InShadowFarPlane > 0.0f )
    {
        if ( NMath::Abs( InShadowFarPlane - ShadowNearPlane ) >= 0.1f )
        {
            ShadowFarPlane = InShadowFarPlane;
            CalculateMatrices();
        }
    }
}

void CPointLight::CalculateMatrices()
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
