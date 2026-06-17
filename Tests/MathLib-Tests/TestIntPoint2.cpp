#include "MathTest.h"

#include <Core/Math/IntVector2.h>

#include <cstdio>

bool TestIntPoint2()
{
    // Constructors
    IntVector2 Point0;

    IntVector2 Point1(1, 2);

    int Arr[2] = { 5, -7 };
    IntVector2 Point2(Arr);

    IntVector2 Point3(-3);

    // Min
    IntVector2 MinPoint = Min(Point2, Point3);
    if (MinPoint != IntVector2(-3, -7))
    {
        TEST_FAILED();
    }

    // Max
    IntVector2 MaxPoint = Max(Point2, Point3);
    if (MaxPoint != IntVector2(5, -3))
    {
        TEST_FAILED();
    }

    // Unary minus
    IntVector2 Minus = -Point1;
    if (Minus != IntVector2(-1, -2))
    {
        TEST_FAILED();
    }

    // Add
    IntVector2 Add0 = Minus + IntVector2(3, 1);
    if (Add0 != IntVector2(2, -1))
    {
        TEST_FAILED();
    }

    IntVector2 Add1 = Minus + 5;
    if (Add1 != IntVector2(4, 3))
    {
        TEST_FAILED();
    }

    // Subtraction
    IntVector2 Sub0 = Add1 - IntVector2(3, 1);
    if (Sub0 != IntVector2(1, 2))
    {
        TEST_FAILED();
    }

    IntVector2 Sub1 = Add1 - 5;
    if (Sub1 != IntVector2(-1, -2))
    {
        TEST_FAILED();

    }

    // Multiplication
    IntVector2 Mul0 = Sub0 * IntVector2(3, 1);
    if (Mul0 != IntVector2(3, 2))
    {
        TEST_FAILED();
    }

    IntVector2 Mul1 = Sub0 * 5;
    if (Mul1 != IntVector2(5, 10))
    {
        TEST_FAILED();
    }

    // Division
    IntVector2 Div0 = Mul0 / IntVector2(3, 1);
    if (Div0 != IntVector2(1, 2))
    {
        TEST_FAILED();
    }

    const IntVector2 Div1 = Mul1 / 5;
    if (Div1 != IntVector2(1, 2))
    {
        TEST_FAILED();
    }

    // Get Component
    int Component = Div1[1];
    if (Component != 2)
    {
        TEST_FAILED();
    }

    return true;
}