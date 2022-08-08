#include "MathTest.h"

#include <Core/Math/IntVector2.h>

#include <cstdio>

bool TestIntPoint2()
{
    // Constructors
    FIntVector2 Point0;

    FIntVector2 Point1(1, 2);

    int Arr[2] = { 5, -7 };
    FIntVector2 Point2(Arr);

    FIntVector2 Point3(-3);

    // Min
    FIntVector2 MinPoint = Min(Point2, Point3);
    if (MinPoint != FIntVector2(-3, -7))
    {
        TEST_FAILED();
    }

    // Max
    FIntVector2 MaxPoint = Max(Point2, Point3);
    if (MaxPoint != FIntVector2(5, -3))
    {
        TEST_FAILED();
    }

    // Unary minus
    FIntVector2 Minus = -Point1;
    if (Minus != FIntVector2(-1, -2))
    {
        TEST_FAILED();
    }

    // Add
    FIntVector2 Add0 = Minus + FIntVector2(3, 1);
    if (Add0 != FIntVector2(2, -1))
    {
        TEST_FAILED();
    }

    FIntVector2 Add1 = Minus + 5;
    if (Add1 != FIntVector2(4, 3))
    {
        TEST_FAILED();
    }

    // Subtraction
    FIntVector2 Sub0 = Add1 - FIntVector2(3, 1);
    if (Sub0 != FIntVector2(1, 2))
    {
        TEST_FAILED();
    }

    FIntVector2 Sub1 = Add1 - 5;
    if (Sub1 != FIntVector2(-1, -2))
    {
        TEST_FAILED();

    }

    // Multiplication
    FIntVector2 Mul0 = Sub0 * FIntVector2(3, 1);
    if (Mul0 != FIntVector2(3, 2))
    {
        TEST_FAILED();
    }

    FIntVector2 Mul1 = Sub0 * 5;
    if (Mul1 != FIntVector2(5, 10))
    {
        TEST_FAILED();
    }

    // Division
    FIntVector2 Div0 = Mul0 / FIntVector2(3, 1);
    if (Div0 != FIntVector2(1, 2))
    {
        TEST_FAILED();
    }

    const FIntVector2 Div1 = Mul1 / 5;
    if (Div1 != FIntVector2(1, 2))
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