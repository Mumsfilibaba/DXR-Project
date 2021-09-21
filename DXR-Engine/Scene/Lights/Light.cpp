#include "Light.h"

Light::Light()
    : Color()
    , ShadowNearPlane( 1.0f )
    , ShadowFarPlane( 30.0f )
    , ShadowBias( 0.005f )
    , MaxShadowBias( 0.05f )
{
    CORE_OBJECT_INIT();
}

void Light::SetColor( const CVector3& InColor )
{
    Color = InColor;
}

void Light::SetColor( float R, float G, float B )
{
    Color = CVector3( R, G, B );
}

void Light::SetIntensity( float InIntensity )
{
    Intensity = InIntensity;
}
