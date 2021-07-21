#include "../Vector2.h"

#include <cstdio>

bool TestVector2()
{
    // Constructors
    CVector2 Point0;

    CVector2 Point1( 1.0f, 2.0f );

    float Arr[2] = { 5.0f, -7.0f };
    CVector2 Point2( Arr );

    CVector2 Point3( -3.0f );

    // Dot
    float Dot = Point1.DotProduct( Point3 );
    if ( Dot != -9.0f )
    {
        assert( false );
        return false;
    }

    // Project On
    CVector2 v0 = CVector2( 2.0f, 5.0f );
    CVector2 v1 = CVector2( 1.0f, 0.0f );
    CVector2 Projected = v0.ProjectOn( v1 );

    if ( Projected != CVector2( 2.0f, 0.0f ) )
    {
        assert( false );
        return false;
    }

    // Min
    CVector2 MinPoint = Min( Point2, Point3 );
    if ( MinPoint != CVector2( -3.0f, -7.0f ) )
    {
        assert( false );
        return false;
    }

    // Max
    CVector2 MaxPoint = Max( Point2, Point3 );
    if ( MaxPoint != CVector2( 5.0f, -3.0f ) )
    {
        assert( false );
        return false;
    }

    // Lerp
    CVector2 Lerped = Lerp( CVector2( 0.0f ), CVector2( 1.0f ), 0.5f );
    if ( Lerped != CVector2( 0.5f, 0.5f ) )
    {
        assert( false );
        return false;
    }

    // Clamp
    CVector2 Clamped = Clamp( CVector2( -2.0f ), CVector2( 5.0f ), CVector2( -3.5f, 7.5f ) );
    if ( Clamped != CVector2( -2.0f, 5.0f ) )
    {
        assert( false );
        return false;
    }

    // Saturate
    CVector2 Saturated = Saturate( CVector2( -5.0f, 1.5f ) );
    if ( Saturated != CVector2( 0.0f, 1.0f ) )
    {
        assert( false );
        return false;
    }

    // Normalize
    CVector2 Norm( 1.0f );
    Norm.Normalize();

    if ( Norm != CVector2( 0.70710678118f, 0.70710678118f ) )
    {
        assert( false );
        return false;
    }

    if ( !Norm.IsUnitVector() )
    {
        assert( false );
        return false;
    }

    // NaN
    CVector2 NaN( 1.0f, NAN );
    if ( !NaN.HasNan() )
    {
        assert( false );
        return false;
    }

    // Infinity
    CVector2 Infinity( 1.0f, INFINITY );
    if ( !Infinity.HasInfinity() )
    {
        assert( false );
        return false;
    }

    // Valid
    if ( Infinity.IsValid() || NaN.IsValid() )
    {
        assert( false );
        return false;
    }

    // Length
    CVector2 LengthVector( 2.0f, 2.0f );
    float Length = LengthVector.Length();

    if ( Length != 2.82842712475f )
    {
        assert( false );
        return false;
    }

    // Length Squared
    float LengthSqrd = LengthVector.LengthSquared();
    if ( LengthSqrd != 8.0f )
    {
        assert( false );
        return false;
    }

    // Unary minus
    CVector2 Minus = -Point1;
    if ( Minus != CVector2( -1.0f, -2.0f ) )
    {
        assert( false );
        return false;
    }

    // Add
    CVector2 Add0 = Minus + CVector2( 3.0f, 1.0f );
    if ( Add0 != CVector2( 2.0f, -1.0f ) )
    {
        assert( false );
        return false;
    }

    CVector2 Add1 = Minus + 5.0f;
    if ( Add1 != CVector2( 4.0f, 3.0f ) )
    {
        assert( false );
        return false;
    }

    // Subtraction
    CVector2 Sub0 = Add1 - CVector2( 3.0f, 1.0f );
    if ( Sub0 != CVector2( 1.0f, 2.0f ) )
    {
        assert( false );
        return false;
    }

    CVector2 Sub1 = Add1 - 5.0f;
    if ( Sub1 != CVector2( -1.0f, -2.0f ) )
    {
        assert( false );
        return false;
    }

    // Multiplication
    CVector2 Mul0 = Sub0 * CVector2( 3.0f, 1.0f );
    if ( Mul0 != CVector2( 3.0f, 2.0f ) )
    {
        assert( false );
        return false;
    }

    CVector2 Mul1 = Sub0 * 5.0f;
    if ( Mul1 != CVector2( 5.0f, 10.0f ) )
    {
        assert( false );
        return false;
    }

    // Division
    CVector2 Div0 = Mul0 / CVector2( 3.0f, 1.0f );
    if ( Div0 != CVector2( 1.0f, 2.0f ) )
    {
        assert( false );
        return false;
    }

    const CVector2 Div1 = Mul1 / 5.0f;
    if ( Div1 != CVector2( 1.0f, 2.0f ) )
    {
        assert( false );
        return false;
    }

    // Get Component
    float Component = Div1[1];
    if ( Component != 2.0f )
    {
        assert( false );
        return false;
    }

    return true;
}