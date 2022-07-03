#include <Core/Math/Vector2.h>

#include <cstdio>

bool TestVector2()
{
    // Constructors
    FVector2 Point0;

    FVector2 Point1( 1.0f, 2.0f );

    float Arr[2] = { 5.0f, -7.0f };
    FVector2 Point2( Arr );

    FVector2 Point3( -3.0f );

    // Dot
    float Dot = Point1.DotProduct( Point3 );
    if ( Dot != -9.0f )
    {
        assert( false );
        return false;
    }

    // Project On
    FVector2 v0 = FVector2( 2.0f, 5.0f );
    FVector2 v1 = FVector2( 1.0f, 0.0f );
    FVector2 Projected = v0.ProjectOn( v1 );

    if ( Projected != FVector2( 2.0f, 0.0f ) )
    {
        assert( false );
        return false;
    }

    // Min
    FVector2 MinPoint = Min( Point2, Point3 );
    if ( MinPoint != FVector2( -3.0f, -7.0f ) )
    {
        assert( false );
        return false;
    }

    // Max
    FVector2 MaxPoint = Max( Point2, Point3 );
    if ( MaxPoint != FVector2( 5.0f, -3.0f ) )
    {
        assert( false );
        return false;
    }

    // Lerp
    FVector2 Lerped = Lerp( FVector2( 0.0f ), FVector2( 1.0f ), 0.5f );
    if ( Lerped != FVector2( 0.5f, 0.5f ) )
    {
        assert( false );
        return false;
    }

    // Clamp
    FVector2 Clamped = Clamp( FVector2( -2.0f ), FVector2( 5.0f ), FVector2( -3.5f, 7.5f ) );
    if ( Clamped != FVector2( -2.0f, 5.0f ) )
    {
        assert( false );
        return false;
    }

    // Saturate
    FVector2 Saturated = Saturate( FVector2( -5.0f, 1.5f ) );
    if ( Saturated != FVector2( 0.0f, 1.0f ) )
    {
        assert( false );
        return false;
    }

    // Normalize
    FVector2 Norm( 1.0f );
    Norm.Normalize();

    if ( Norm != FVector2( 0.70710678118f, 0.70710678118f ) )
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
    FVector2 NaN( 1.0f, NAN );
    if ( !NaN.HasNan() )
    {
        assert( false );
        return false;
    }

    // Infinity
    FVector2 Infinity( 1.0f, INFINITY );
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
    FVector2 LengthVector( 2.0f, 2.0f );
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
    FVector2 Minus = -Point1;
    if ( Minus != FVector2( -1.0f, -2.0f ) )
    {
        assert( false );
        return false;
    }

    // Add
    FVector2 Add0 = Minus + FVector2( 3.0f, 1.0f );
    if ( Add0 != FVector2( 2.0f, -1.0f ) )
    {
        assert( false );
        return false;
    }

    FVector2 Add1 = Minus + 5.0f;
    if ( Add1 != FVector2( 4.0f, 3.0f ) )
    {
        assert( false );
        return false;
    }

    // Subtraction
    FVector2 Sub0 = Add1 - FVector2( 3.0f, 1.0f );
    if ( Sub0 != FVector2( 1.0f, 2.0f ) )
    {
        assert( false );
        return false;
    }

    FVector2 Sub1 = Add1 - 5.0f;
    if ( Sub1 != FVector2( -1.0f, -2.0f ) )
    {
        assert( false );
        return false;
    }

    // Multiplication
    FVector2 Mul0 = Sub0 * FVector2( 3.0f, 1.0f );
    if ( Mul0 != FVector2( 3.0f, 2.0f ) )
    {
        assert( false );
        return false;
    }

    FVector2 Mul1 = Sub0 * 5.0f;
    if ( Mul1 != FVector2( 5.0f, 10.0f ) )
    {
        assert( false );
        return false;
    }

    // Division
    FVector2 Div0 = Mul0 / FVector2( 3.0f, 1.0f );
    if ( Div0 != FVector2( 1.0f, 2.0f ) )
    {
        assert( false );
        return false;
    }

    const FVector2 Div1 = Mul1 / 5.0f;
    if ( Div1 != FVector2( 1.0f, 2.0f ) )
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