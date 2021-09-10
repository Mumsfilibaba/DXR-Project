#pragma once
#include "Light.h"

#include "Core/Math/Matrix4.h"

class PointLight : public Light
{
    CORE_OBJECT( PointLight, Light );

public:
    PointLight();
    ~PointLight() = default;

    void SetPosition( const CVector3& InPosition );
    void SetPosition( float x, float y, float z );

    void SetShadowNearPlane( float InShadowNearPlane );
    void SetShadowFarPlane( float InShadowFarPlane );

    void SetShadowCaster( bool InShadowCaster )
    {
        ShadowCaster = InShadowCaster;
        CalculateMatrices();
    }

    bool IsShadowCaster() const
    {
        return ShadowCaster;
    }

    const CVector3& GetPosition() const
    {
        return Position;
    }

    const CMatrix4& GetMatrix( uint32 Index ) const
    {
        Assert( Index < 6 );
        return Matrices[Index];
    }

    const CMatrix4& GetViewMatrix( uint32 Index ) const
    {
        Assert( Index < 6 );
        return ViewMatrices[Index];
    }

    const CMatrix4& GetProjectionMatrix( uint32 Index ) const
    {
        Assert( Index < 6 );
        return ProjMatrices[Index];
    }

private:
    void CalculateMatrices();

    CMatrix4 Matrices[6];
    CMatrix4 ViewMatrices[6];
    CMatrix4 ProjMatrices[6];
    CVector3 Position;
    bool     ShadowCaster = false;
};