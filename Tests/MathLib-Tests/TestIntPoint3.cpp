#include "MathTest.h"

#include <Core/Math/IntVector3.h>

#include <cstdio>

bool TestIntPoint3()
{
    // Constructors
    CIntVector3 Point0;

    CIntVector3 Point1(1, 2, -4);

    int Arr[3] = { 5, -7, 2 };
    CIntVector3 Point2(Arr);

    CIntVector3 Point3(-3);

    // Min
    CIntVector3 MinPoint = Min(Point2, Point3);
    if (MinPoint != CIntVector3(-3, -7, -3))
    {
        TEST_FAILED();
    }

    // Max
    CIntVector3 MaxPoint = Max(Point2, Point3);
    if (MaxPoint != CIntVector3(5, -3, 2))
    {
        TEST_FAILED();
    }

    // Unary minus
    CIntVector3 Minus = -Point1;
    if (Minus != CIntVector3(-1, -2, 4))
    {
        TEST_FAILED();
    }

    // Add
    CIntVector3 Add0 = Minus + CIntVector3(3, 1, 2);
    if (Add0 != CIntVector3(2, -1, 6))
    {
        TEST_FAILED();
    }

    CIntVector3 Add1 = Minus + 5;
    if (Add1 != CIntVector3(4, 3, 9))
    {
        TEST_FAILED();
    }

    // Subtraction
    CIntVector3 Sub0 = Add1 - CIntVector3(3, 1, 6);
    if (Sub0 != CIntVector3(1, 2, 3))
    {
        TEST_FAILED();
    }

    CIntVector3 Sub1 = Add1 - 5;
    if (Sub1 != CIntVector3(-1, -2, 4))
    {
        TEST_FAILED();
    }

    // Multiplication
    CIntVector3 Mul0 = Sub0 * CIntVector3(3, 1, 2);
    if (Mul0 != CIntVector3(3, 2, 6))
    {
        TEST_FAILED();
    }

    CIntVector3 Mul1 = Sub0 * 5;
    if (Mul1 != CIntVector3(5, 10, 15))
    {
        TEST_FAILED();
    }

    // Division
    CIntVector3 Div0 = Mul0 / CIntVector3(3, 1, 3);
    if (Div0 != CIntVector3(1, 2, 2))
    {
        TEST_FAILED();
    }

    const CIntVector3 Div1 = Mul1 / 5;
    if (Div1 != CIntVector3(1, 2, 3))
    {
        TEST_FAILED();
    }

    // Get Component
    int Component = Div1[2];
    if (Component != 3)
    {
        TEST_FAILED();
    }

    return true;
}