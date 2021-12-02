#pragma once
#include "Light.h"

#include "Core/Math/Matrix4.h"

class ENGINE_API CPointLight : public CLight
{
    CORE_OBJECT( CPointLight, CLight );

public:
    CPointLight();
    ~CPointLight() = default;

    void SetPosition( const CVector3& InPosition );
    void SetPosition( float x, float y, float z );

    void SetShadowNearPlane( float InShadowNearPlane );
    void SetShadowFarPlane( float InShadowFarPlane );

    FORCEINLINE void SetShadowCaster( bool bInShadowCaster )
    {
        bShadowCaster = bInShadowCaster;
        CalculateMatrices();
    }

    FORCEINLINE bool IsShadowCaster() const
    {
        return bShadowCaster;
    }

    FORCEINLINE const CVector3& GetPosition() const
    {
        return Position;
    }

    FORCEINLINE const CMatrix4& GetMatrix( uint32 Index ) const
    {
        Assert( Index < 6 );
        return Matrices[Index];
    }

    FORCEINLINE const CMatrix4& GetViewMatrix( uint32 Index ) const
    {
        Assert( Index < 6 );
        return ViewMatrices[Index];
    }

    FORCEINLINE const CMatrix4& GetProjectionMatrix( uint32 Index ) const
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
    bool     bShadowCaster = false;
};