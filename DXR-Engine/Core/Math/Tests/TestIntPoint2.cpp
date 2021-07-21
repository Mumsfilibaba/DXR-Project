#include "../IntPoint2.h"

#include <cstdio>

bool TestIntPoint2()
{
    // Constructors
    CIntPoint2 Point0;

    CIntPoint2 Point1( 1, 2 );

    int Arr[2] = { 5, -7 };
    CIntPoint2 Point2( Arr );

    CIntPoint2 Point3( -3 );

    // Min
    CIntPoint2 MinPoint = Min( Point2, Point3 );
    if ( MinPoint != CIntPoint2( -3, -7 ) )
    {
        assert( false );
        return false;
    }

    // Max
    CIntPoint2 MaxPoint = Max( Point2, Point3 );
    if ( MaxPoint != CIntPoint2( 5, -3 ) )
    {
        assert( false );
        return false;
    }

    // Unary minus
    CIntPoint2 Minus = -Point1;
    if ( Minus != CIntPoint2( -1, -2 ) )
    {
        assert( false );
        return false;
    }

    // Add
    CIntPoint2 Add0 = Minus + CIntPoint2( 3, 1 );
    if ( Add0 != CIntPoint2( 2, -1 ) )
    {
        assert( false );
        return false;
    }

    CIntPoint2 Add1 = Minus + 5;
    if ( Add1 != CIntPoint2( 4, 3 ) )
    {
        assert( false );
        return false;
    }

    // Subtraction
    CIntPoint2 Sub0 = Add1 - CIntPoint2( 3, 1 );
    if ( Sub0 != CIntPoint2( 1, 2 ) )
    {
        assert( false );
        return false;
    }

    CIntPoint2 Sub1 = Add1 - 5;
    if ( Sub1 != CIntPoint2( -1, -2 ) )
    {
        assert( false );
        return false;
    }

    // Multiplication
    CIntPoint2 Mul0 = Sub0 * CIntPoint2( 3, 1 );
    if ( Mul0 != CIntPoint2( 3, 2 ) )
    {
        assert( false );
        return false;
    }

    CIntPoint2 Mul1 = Sub0 * 5;
    if ( Mul1 != CIntPoint2( 5, 10 ) )
    {
        assert( false );
        return false;
    }

    // Division
    CIntPoint2 Div0 = Mul0 / CIntPoint2( 3, 1 );
    if ( Div0 != CIntPoint2( 1, 2 ) )
    {
        assert( false );
        return false;
    }

    const CIntPoint2 Div1 = Mul1 / 5;
    if ( Div1 != CIntPoint2( 1, 2 ) )
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