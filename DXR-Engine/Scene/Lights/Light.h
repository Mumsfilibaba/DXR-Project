#pragma once
#include "Core/Math/Vector3.h"
#include "Core/CoreObject/CoreObject.h"

class CORE_API CLight : public CCoreObject
{
    CORE_OBJECT( CLight, CCoreObject );

public:
    CLight();
    virtual ~CLight() = default;

    void SetColor( const CVector3& InColor );
    void SetColor( float r, float g, float b );

    void SetIntensity( float InIntensity );

    FORCEINLINE void SetShadowBias( float InShadowBias )
    {
        ShadowBias = InShadowBias;
    }

    FORCEINLINE void SetMaxShadowBias( float InShadowBias )
    {
        MaxShadowBias = InShadowBias;
    }

    FORCEINLINE float GetIntensity() const
    {
        return Intensity;
    }

    FORCEINLINE const CVector3& GetColor() const
    {
        return Color;
    }

    FORCEINLINE float GetShadowNearPlane() const
    {
        return ShadowNearPlane;
    }

    FORCEINLINE float GetShadowFarPlane() const
    {
        return ShadowFarPlane;
    }

    FORCEINLINE float GetShadowBias() const
    {
        return ShadowBias;
    }

    FORCEINLINE float GetMaxShadowBias() const
    {
        return MaxShadowBias;
    }

protected:
    CVector3 Color;
    float Intensity = 1.0f;
    float ShadowNearPlane;
    float ShadowFarPlane;
    float ShadowBias;
    float MaxShadowBias;
};