#include "MathTest.h"

#include <Core/Math/IntVector3.h>

#include <cstdio>

bool TestIntPoint3()
{
    // Constructors
    FIntVector3 Point0;

    FIntVector3 Point1(1, 2, -4);

    int Arr[3] = { 5, -7, 2 };
    FIntVector3 Point2(Arr);

    FIntVector3 Point3(-3);

    // Min
    FIntVector3 MinPoint = Min(Point2, Point3);
    if (MinPoint != FIntVector3(-3, -7, -3))
    {
        TEST_FAILED();
    }

    // Max
    FIntVector3 MaxPoint = Max(Point2, Point3);
    if (MaxPoint != FIntVector3(5, -3, 2))
    {
        TEST_FAILED();
    }

    // Unary minus
    FIntVector3 Minus = -Point1;
    if (Minus != FIntVector3(-1, -2, 4))
    {
        TEST_FAILED();
    }

    // Add
    FIntVector3 Add0 = Minus + FIntVector3(3, 1, 2);
    if (Add0 != FIntVector3(2, -1, 6))
    {
        TEST_FAILED();
    }

    FIntVector3 Add1 = Minus + 5;
    if (Add1 != FIntVector3(4, 3, 9))
    {
        TEST_FAILED();
    }

    // Subtraction
    FIntVector3 Sub0 = Add1 - FIntVector3(3, 1, 6);
    if (Sub0 != FIntVector3(1, 2, 3))
    {
        TEST_FAILED();
    }

    FIntVector3 Sub1 = Add1 - 5;
    if (Sub1 != FIntVector3(-1, -2, 4))
    {
        TEST_FAILED();
    }

    // Multiplication
    FIntVector3 Mul0 = Sub0 * FIntVector3(3, 1, 2);
    if (Mul0 != FIntVector3(3, 2, 6))
    {
        TEST_FAILED();
    }

    FIntVector3 Mul1 = Sub0 * 5;
    if (Mul1 != FIntVector3(5, 10, 15))
    {
        TEST_FAILED();
    }

    // Division
    FIntVector3 Div0 = Mul0 / FIntVector3(3, 1, 3);
    if (Div0 != FIntVector3(1, 2, 2))
    {
        TEST_FAILED();
    }

    const FIntVector3 Div1 = Mul1 / 5;
    if (Div1 != FIntVector3(1, 2, 3))
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