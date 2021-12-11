#include <Core/Math/IntVector2.h>

#include <cstdio>

bool TestIntPoint2()
{
    // Constructors
    CIntVector2 Point0;

    CIntVector2 Point1( 1, 2 );

    int Arr[2] = { 5, -7 };
    CIntVector2 Point2( Arr );

    CIntVector2 Point3( -3 );

    // Min
    CIntVector2 MinPoint = Min( Point2, Point3 );
    if ( MinPoint != CIntVector2( -3, -7 ) )
    {
        assert( false );
        return false;
    }

    // Max
    CIntVector2 MaxPoint = Max( Point2, Point3 );
    if ( MaxPoint != CIntVector2( 5, -3 ) )
    {
        assert( false );
        return false;
    }

    // Unary minus
    CIntVector2 Minus = -Point1;
    if ( Minus != CIntVector2( -1, -2 ) )
    {
        assert( false );
        return false;
    }

    // Add
    CIntVector2 Add0 = Minus + CIntVector2( 3, 1 );
    if ( Add0 != CIntVector2( 2, -1 ) )
    {
        assert( false );
        return false;
    }

    CIntVector2 Add1 = Minus + 5;
    if ( Add1 != CIntVector2( 4, 3 ) )
    {
        assert( false );
        return false;
    }

    // Subtraction
    CIntVector2 Sub0 = Add1 - CIntVector2( 3, 1 );
    if ( Sub0 != CIntVector2( 1, 2 ) )
    {
        assert( false );
        return false;
    }

    CIntVector2 Sub1 = Add1 - 5;
    if ( Sub1 != CIntVector2( -1, -2 ) )
    {
        assert( false );
        return false;
    }

    // Multiplication
    CIntVector2 Mul0 = Sub0 * CIntVector2( 3, 1 );
    if ( Mul0 != CIntVector2( 3, 2 ) )
    {
        assert( false );
        return false;
    }

    CIntVector2 Mul1 = Sub0 * 5;
    if ( Mul1 != CIntVector2( 5, 10 ) )
    {
        assert( false );
        return false;
    }

    // Division
    CIntVector2 Div0 = Mul0 / CIntVector2( 3, 1 );
    if ( Div0 != CIntVector2( 1, 2 ) )
    {
        assert( false );
        return false;
    }

    const CIntVector2 Div1 = Mul1 / 5;
    if ( Div1 != CIntVector2( 1, 2 ) )
    {
        assert( false );
        return false;
    }

    // Get Component
    int Component = Div1[1];
    if ( Component != 2 )
    {
        assert( false );
        return false;
    }

    return true;
}