#include "MathTest.h"

#include <Core/Math/IntVector3.h>

#include <cstdio>

bool TestIntPoint3()
{
    // Constructors
    IntVector3 Point0;

    IntVector3 Point1(1, 2, -4);

    int Arr[3] = { 5, -7, 2 };
    IntVector3 Point2(Arr);

    IntVector3 Point3(-3);

    // Min
    IntVector3 MinPoint = Min(Point2, Point3);
    if (MinPoint != IntVector3(-3, -7, -3))
    {
        TEST_FAILED();
    }

    // Max
    IntVector3 MaxPoint = Max(Point2, Point3);
    if (MaxPoint != IntVector3(5, -3, 2))
    {
        TEST_FAILED();
    }

    // Unary minus
    IntVector3 Minus = -Point1;
    if (Minus != IntVector3(-1, -2, 4))
    {
        TEST_FAILED();
    }

    // Add
    IntVector3 Add0 = Minus + IntVector3(3, 1, 2);
    if (Add0 != IntVector3(2, -1, 6))
    {
        TEST_FAILED();
    }

    IntVector3 Add1 = Minus + 5;
    if (Add1 != IntVector3(4, 3, 9))
    {
        TEST_FAILED();
    }

    // Subtraction
    IntVector3 Sub0 = Add1 - IntVector3(3, 1, 6);
    if (Sub0 != IntVector3(1, 2, 3))
    {
        TEST_FAILED();
    }

    IntVector3 Sub1 = Add1 - 5;
    if (Sub1 != IntVector3(-1, -2, 4))
    {
        TEST_FAILED();
    }

    // Multiplication
    IntVector3 Mul0 = Sub0 * IntVector3(3, 1, 2);
    if (Mul0 != IntVector3(3, 2, 6))
    {
        TEST_FAILED();
    }

    IntVector3 Mul1 = Sub0 * 5;
    if (Mul1 != IntVector3(5, 10, 15))
    {
        TEST_FAILED();
    }

    // Division
    IntVector3 Div0 = Mul0 / IntVector3(3, 1, 3);
    if (Div0 != IntVector3(1, 2, 2))
    {
        TEST_FAILED();
    }

    const IntVector3 Div1 = Mul1 / 5;
    if (Div1 != IntVector3(1, 2, 3))
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