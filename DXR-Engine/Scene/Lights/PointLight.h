#pragma once
#include "Light.h"

class PointLight : public Light
{
    CORE_OBJECT( PointLight, Light );

public:
    PointLight();
    ~PointLight() = default;

    void SetPosition( const XMFLOAT3& InPosition );
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

    const XMFLOAT3& GetPosition() const
    {
        return Position;
    }

    const XMFLOAT4X4& GetMatrix( uint32 Index ) const
    {
        Assert( Index < 6 );
        return Matrices[Index];
    }

    const XMFLOAT4X4& GetViewMatrix( uint32 Index ) const
    {
        Assert( Index < 6 );
        return ViewMatrices[Index];
    }

    const XMFLOAT4X4& GetProjectionMatrix( uint32 Index ) const
    {
        Assert( Index < 6 );
        return ProjMatrices[Index];
    }

private:
    void CalculateMatrices();

    XMFLOAT4X4 Matrices[6];
    XMFLOAT4X4 ViewMatrices[6];
    XMFLOAT4X4 ProjMatrices[6];
    XMFLOAT3   Position;
    bool       ShadowCaster = false;
};