#include <Core/Math/Vector4.h>

#include <cstdio>

#define _XM_NO_INTRINSICS_
#include <DirectXMath.h>
using namespace DirectX;

bool TestVector4()
{
    // Constructors
    FVector4 Point0;

    FVector4 Point1( 1.0f, 2.0f, -2.0f, 4.0f );

    float Arr[4] = { 5.0f, -7.0f, 2.0f, 4.0f };
    FVector4 Point2( Arr );

    FVector4 Point3( -3.0f );

    // Dot
    float Dot = Point1.DotProduct( Point3 );
    if ( Dot != -15.0f )
    {
        assert(false);
        return false;
    }

    // Cross
    FVector4 Cross = Point1.CrossProduct( Point2 );

    XMVECTOR Xm0 = XMVectorSet( 1.0f, 2.0f, -2.0f, 4.0f );
    XMVECTOR Xm1 = XMVectorSet( 5.0f, -7.0f, 2.0f, 4.0f );
    XMVECTOR XmCross = XMVector3Cross( Xm0, Xm1 );

    XMFLOAT4 XmFloat4;
    XMStoreFloat4( &XmFloat4, XmCross );

    if ( Cross != FVector4( reinterpret_cast<float*>(&XmFloat4) ) )
    {
        assert(false);
        return false;
    }

    // Project On
    FVector4 v0 = FVector4( 4.0f, 5.0f, 3.0f, 10.0f );
    FVector4 v1 = FVector4( 1.0f, 0.0f, 0.0f, 0.0f );
    FVector4 Projected = v0.ProjectOn( v1 );

    if ( Projected != FVector4( 4.0f, 0.0f, 0.0f, 0.0f ) )
    {
        assert(false);
        return false;
    }

    // Reflection
    FVector4 Reflect = Point1.Reflect( FVector4( 0.0f, 1.0f, 0.0f, 0.0f ) );

    XMVECTOR Xm2 = XMVectorSet( 1.0f, 2.0f, -2.0f, 4.0f );
    XMVECTOR Xm3 = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
    XMVECTOR XmReflect = XMVector4Reflect( Xm2, Xm3 );

    XMStoreFloat4( &XmFloat4, XmReflect );

    if ( Reflect != FVector4( reinterpret_cast<float*>(&XmFloat4) ) )
    {
        assert(false);
        return false;
    }

    // Min
    FVector4 MinPoint = Min( Point2, Point3 );
    if ( MinPoint != FVector4( -3.0f, -7.0f, -3.0f, -3.0f ) )
    {
        assert(false);
        return false;
    }

    // Max
    FVector4 MaxPoint = Max( Point2, Point3 );
    if ( MaxPoint != FVector4( 5.0f, -3.0f, 2.0f, 4.0f ) )
    {
        assert(false);
        return false;
    }

    // Lerp
    FVector4 Lerped = Lerp( FVector4( 0.0f ), FVector4( 1.0f ), 0.5f );
    if ( Lerped != FVector4( 0.5f ) )
    {
        assert(false);
        return false;
    }

    // Clamp
    FVector4 Clamped = Clamp( FVector4( -2.0f ), FVector4( 5.0f ), FVector4( -3.5f, 7.5f, 1.0f, 2.0f ) );
    if ( Clamped != FVector4( -2.0f, 5.0f, 1.0f, 2.0f ) )
    {
        assert(false);
        return false;
    }

    // Saturate
    FVector4 Saturated = Saturate( FVector4( -5.0f, 1.5f, 0.25f, -5.7f ) );
    if ( Saturated != FVector4( 0.0f, 1.0f, 0.25f, 0.0f ) )
    {
        assert(false);
        return false;
    }

    // Normalize
    FVector4 Norm( 1.0f );
    FVector4 Normalize = Norm.GetNormalized();

    Norm.Normalize();
    if ( Norm != FVector4( 0.5f ) )
    {
        assert(false);
        return false;
    }

    if ( Normalize != FVector4( 0.5f ) )
    {
        assert(false);
        return false;
    }

    if ( !Norm.IsUnitVector() )
    {
        assert(false);
        return false;
    }

    // NaN
    FVector4 NaN( 1.0f, 0.0f, 0.0f, NAN );
    if ( !NaN.HasNaN() )
    {
        assert(false);
        return false;
    }

    // Infinity
    FVector4 Infinity( 1.0f, 0.0f, 0.0f, INFINITY );
    if ( !Infinity.HasInfinity() )
    {
        assert(false);
        return false;
    }

    // Valid
    if ( Infinity.IsValid() || NaN.IsValid() )
    {
        assert(false);
        return false;
    }

    // Length
    FVector4 LengthVector( 2.0f, 2.0f, 2.0f, 2.0f );
    float Length = LengthVector.Length();

    if ( Length != 4.0f )
    {
        assert(false);
        return false;
    }

    // Length Squared
    float LengthSqrd = LengthVector.LengthSquared();
    if ( LengthSqrd != 16.0f )
    {
        assert(false);
        return false;
    }

    // Unary minus
    FVector4 Minus = -Point1;
    if ( Minus != FVector4( -1.0f, -2.0f, 2.0f, -4.0f ) )
    {
        assert(false);
        return false;
    }

    // Add
    FVector4 Add0 = Minus + FVector4( 3.0f, 1.0f, -1.0f, 2.0f );
    if ( Add0 != FVector4( 2.0f, -1.0f, 1.0f, -2.0f ) )
    {
        assert(false);
        return false;
    }

    FVector4 Add1 = Minus + 5.0f;
    if ( Add1 != FVector4( 4.0f, 3.0f, 7.0f, 1.0f ) )
    {
        assert(false);
        return false;
    }

    // Subtraction
    FVector4 Sub0 = Add1 - FVector4( 3.0f, 1.0f, 8.0f, 3.0f );
    if ( Sub0 != FVector4( 1.0f, 2.0f, -1.0f, -2.0f ) )
    {
        assert(false);
        return false;
    }

    FVector4 Sub1 = Add1 - 5.0f;
    if ( Sub1 != FVector4( -1.0f, -2.0f, 2.0f, -4.0f ) )
    {
        assert(false);
        return false;
    }

    // Multiplication
    FVector4 Mul0 = Sub0 * FVector4( 3.0f, 1.0f, 2.0f, -1.0f );
    if ( Mul0 != FVector4( 3.0f, 2.0f, -2.0f, 2.0f ) )
    {
        assert(false);
        return false;
    }

    FVector4 Mul1 = Sub0 * 5.0f;
    if ( Mul1 != FVector4( 5.0f, 10.0f, -5.0f, -10.0f ) )
    {
        assert(false);
        return false;
    }

    // Division
    FVector4 Div0 = Mul0 / FVector4( 3.0f, 1.0f, 2.0f, 2.0f );
    if ( Div0 != FVector4( 1.0f, 2.0f, -1.0f, 1.0f ) )
    {
        assert(false);
        return false;
    }

    const FVector4 Div1 = Mul1 / 5.0f;
    if ( Div1 != FVector4( 1.0f, 2.0f, -1.0f, -2.0f ) )
    {
        assert(false);
        return false;
    }

    // Get Component
    float Component = Div1[3];
    if ( Component != -2.0f )
    {
        assert(false);
        return false;
    }

    return true;
}