#include <Core/Math/IntPoint3.h>

#include <cstdio>

bool TestIntPoint3()
{
    // Constructors
    CIntPoint3 Point0;

    CIntPoint3 Point1( 1, 2, -4 );

    int Arr[3] = { 5, -7, 2 };
    CIntPoint3 Point2( Arr );

    CIntPoint3 Point3( -3 );

    // Min
    CIntPoint3 MinPoint = Min( Point2, Point3 );
    if ( MinPoint != CIntPoint3( -3, -7, -3 ) )
    {
        assert( false );
        return false;
    }

    // Max
    CIntPoint3 MaxPoint = Max( Point2, Point3 );
    if ( MaxPoint != CIntPoint3( 5, -3, 2 ) )
    {
        assert( false );
        return false;
    }

    // Unary minus
    CIntPoint3 Minus = -Point1;
    if ( Minus != CIntPoint3( -1, -2, 4 ) )
    {
        assert( false );
        return false;
    }

    // Add
    CIntPoint3 Add0 = Minus + CIntPoint3( 3, 1, 2 );
    if ( Add0 != CIntPoint3( 2, -1, 6 ) )
    {
        assert( false );
        return false;
    }

    CIntPoint3 Add1 = Minus + 5;
    if ( Add1 != CIntPoint3( 4, 3, 9 ) )
    {
        assert( false );
        return false;
    }

    // Subtraction
    CIntPoint3 Sub0 = Add1 - CIntPoint3( 3, 1, 6 );
    if ( Sub0 != CIntPoint3( 1, 2, 3 ) )
    {
        assert( false );
        return false;
    }

    CIntPoint3 Sub1 = Add1 - 5;
    if ( Sub1 != CIntPoint3( -1, -2, 4 ) )
    {
        assert( false );
        return false;
    }

    // Multiplication
    CIntPoint3 Mul0 = Sub0 * CIntPoint3( 3, 1, 2 );
    if ( Mul0 != CIntPoint3( 3, 2, 6 ) )
    {
        assert( false );
        return false;
    }

    CIntPoint3 Mul1 = Sub0 * 5;
    if ( Mul1 != CIntPoint3( 5, 10, 15 ) )
    {
        assert( false );
        return false;
    }

    // Division
    CIntPoint3 Div0 = Mul0 / CIntPoint3( 3, 1, 3 );
    if ( Div0 != CIntPoint3( 1, 2, 2 ) )
    {
        assert( false );
        return false;
    }

    const CIntPoint3 Div1 = Mul1 / 5;
    if ( Div1 != CIntPoint3( 1, 2, 3 ) )
    {
        assert( false );
        return false;
    }

    // Get Component
    int Component = Div1[2];
    if ( Component != 3 )
    {
        assert( false );
        return false;
    }

    return true;
}