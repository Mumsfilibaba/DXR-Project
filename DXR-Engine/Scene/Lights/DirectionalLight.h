#pragma once
#include "Light.h"

#include "Math/Vector3.h"
#include "Math/Matrix4.h"

#define NUM_SHADOW_CASCADES (4)

class DirectionalLight : public Light
{
    CORE_OBJECT( DirectionalLight, Light );

public:
    DirectionalLight();
    ~DirectionalLight();

    void UpdateCascades( class Camera& Camera );

    // Rotation in Radians
    void SetRotation( const CVector3& InRotation );
    void SetRotation( float x, float y, float z );

    void SetLookAt( const CVector3& InInLookAt );
    void SetLookAt( float x, float y, float z );

    void SetCascadeSplitLambda( float InCascadeSplitLambda )
    {
        CascadeSplitLambda = InCascadeSplitLambda;
    }

    void SetSize( float InSize )
    {
        Size = InSize;
    }

    const CVector3& GetDirection() const
    {
        return Direction;
    }

    const CVector3& GetUp() const
    {
        return Up;
    }

    const CVector3& GetRotation() const
    {
        return Rotation;
    }

    const CVector3& GetPosition() const
    {
        return Position;
    }

    const CVector3& GetLookAt() const
    {
        return LookAt;
    }

    const CMatrix4& GetMatrix( uint32 CascadeIndex ) const
    {
        Assert( CascadeIndex < NUM_SHADOW_CASCADES );
        return Matrices[CascadeIndex];
    }

    const CMatrix4& GetViewMatrix( uint32 CascadeIndex ) const
    {
        Assert( CascadeIndex < NUM_SHADOW_CASCADES );
        return ViewMatrices[CascadeIndex];
    }

    const CMatrix4& GetProjectionMatrix( uint32 CascadeIndex ) const
    {
        Assert( CascadeIndex < NUM_SHADOW_CASCADES );
        return ProjectionMatrices[CascadeIndex];
    }

    float GetCascadeSplitLambda() const
    {
        return CascadeSplitLambda;
    }

    float GetCascadeSplit( uint32 CascadeIndex ) const
    {
        return CascadeSplits[CascadeIndex];
    }
    float GetCascadeRadius( uint32 CascadeIndex ) const
    {
        return CascadeRadius[CascadeIndex];
    }

    float GetSize() const
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