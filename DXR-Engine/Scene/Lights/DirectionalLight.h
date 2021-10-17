#pragma once
#include "Light.h"

#include "Core/Math/Matrix4.h"

#define NUM_SHADOW_CASCADES (4)

class CORE_API CDirectionalLight : public CLight
{
    CORE_OBJECT( CDirectionalLight, CLight );

public:
    CDirectionalLight();
    ~CDirectionalLight();

    void UpdateCascades( class CCamera& Camera );

    // Rotation in Radians
    void SetRotation( const CVector3& InRotation );
    void SetRotation( float x, float y, float z );

    void SetLookAt( const CVector3& InInLookAt );
    void SetLookAt( float x, float y, float z );

    FORCEINLINE void SetCascadeSplitLambda( float InCascadeSplitLambda )
    {
        CascadeSplitLambda = InCascadeSplitLambda;
    }

    FORCEINLINE void SetSize( float InSize )
    {
        Size = InSize;
    }

    FORCEINLINE const CVector3& GetDirection() const
    {
        return Direction;
    }

    FORCEINLINE const CVector3& GetUp() const
    {
        return Up;
    }

    FORCEINLINE const CVector3& GetRotation() const
    {
        return Rotation;
    }

    FORCEINLINE const CVector3& GetPosition() const
    {
        return Position;
    }

    FORCEINLINE const CVector3& GetLookAt() const
    {
        return LookAt;
    }

    FORCEINLINE const CMatrix4& GetMatrix( uint32 CascadeIndex ) const
    {
        Assert( CascadeIndex < NUM_SHADOW_CASCADES );
        return Matrices[CascadeIndex];
    }

    FORCEINLINE const CMatrix4& GetViewMatrix( uint32 CascadeIndex ) const
    {
        Assert( CascadeIndex < NUM_SHADOW_CASCADES );
        return ViewMatrices[CascadeIndex];
    }

    FORCEINLINE const CMatrix4& GetProjectionMatrix( uint32 CascadeIndex ) const
    {
        Assert( CascadeIndex < NUM_SHADOW_CASCADES );
        return ProjectionMatrices[CascadeIndex];
    }

    FORCEINLINE float GetCascadeSplitLambda() const
    {
        return CascadeSplitLambda;
    }

    FORCEINLINE float GetCascadeSplit( uint32 CascadeIndex ) const
    {
        return CascadeSplits[CascadeIndex];
    }

    FORCEINLINE float GetCascadeRadius( uint32 CascadeIndex ) const
    {
        return CascadeRadius[CascadeIndex];
    }

    FORCEINLINE float GetSize() const
    {
        return Size;
    }

private:
    CVector3 Direction;
    CVector3 Up;
    CVector3 Rotation;
    CVector3 LookAt;
    CVector3 Position;

    CMatrix4 ViewMatrices[NUM_SHADOW_CASCADES];
    CMatrix4 ProjectionMatrices[NUM_SHADOW_CASCADES];
    CMatrix4 Matrices[NUM_SHADOW_CASCADES];

    float CascadeSplits[NUM_SHADOW_CASCADES];
    float CascadeRadius[NUM_SHADOW_CASCADES];

    float CascadeSplitLambda = 0.8f;

    float Size = 0.05f;
};