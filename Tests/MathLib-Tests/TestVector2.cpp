#include "MathTest.h"

#include <Core/Math/Vector2.h>

#include <cstdio>

bool TestVector2()
{
    // Constructors
    FVector2 Point0;
    FVector2 Point1(1.0f, 2.0f);

    float Arr[2] = { 5.0f, -7.0f };
    FVector2 Point2(Arr);
    FVector2 Point3(-3.0f);

    // Dot
    float Dot = Point1.DotProduct(Point3);
    if (Dot != -9.0f)
    {
        TEST_FAILED();
    }

    // Project On
    FVector2 v0 = FVector2(2.0f, 5.0f);
    FVector2 v1 = FVector2(1.0f, 0.0f);
    FVector2 Projected = v0.ProjectOn(v1);

    if (Projected != FVector2(2.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // Min
    FVector2 MinPoint = Min(Point2, Point3);
    if (MinPoint != FVector2(-3.0f, -7.0f))
    {
        TEST_FAILED();
    }

    // Max
    FVector2 MaxPoint = Max(Point2, Point3);
    if (MaxPoint != FVector2(5.0f, -3.0f))
    {
        TEST_FAILED();
    }

    // Lerp
    FVector2 Lerped = Lerp(FVector2(0.0f), FVector2(1.0f), 0.5f);
    if (Lerped != FVector2(0.5f, 0.5f))
    {
        TEST_FAILED();
    }

    // Clamp
    FVector2 Clamped = Clamp(FVector2(-2.0f), FVector2(5.0f), FVector2(-3.5f, 7.5f));
    if (Clamped != FVector2(-2.0f, 5.0f))
    {
        TEST_FAILED();
    }

    // Saturate
    FVector2 Saturated = Saturate(FVector2(-5.0f, 1.5f));
    if (Saturated != FVector2(0.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Normalize
    FVector2 Norm(1.0f);
    Norm.Normalize();

    if (Norm != FVector2(0.70710678118f, 0.70710678118f))
    {
        TEST_FAILED();
    }

    if (!Norm.IsUnitVector())
    {
        TEST_FAILED();
    }

    // NaN
    FVector2 NaN(1.0f, NAN);
    if (!NaN.HasNaN())
    {
        TEST_FAILED();
    }

    // Infinity
    FVector2 Infinity(1.0f, INFINITY);
    if (!Infinity.HasInfinity())
    {
        TEST_FAILED();
    }

    // Valid
    if (Infinity.IsValid() || NaN.IsValid())
    {
        TEST_FAILED();
    }

    // Length
    FVector2 LengthVector(2.0f, 2.0f);
    float Length = LengthVector.GetLength();

    if (Length != 2.82842712475f)
    {
        TEST_FAILED();
    }

    // Length Squared
    float LengthSqrd = LengthVector.LengthSquared();
    if (LengthSqrd != 8.0f)
    {
        TEST_FAILED();
    }

    // Unary minus
    FVector2 Minus = -Point1;
    if (Minus != FVector2(-1.0f, -2.0f))
    {
        TEST_FAILED();
    }

    // Add
    FVector2 Add0 = Minus + FVector2(3.0f, 1.0f);
    if (Add0 != FVector2(2.0f, -1.0f))
    {
        TEST_FAILED();
    }

    FVector2 Add1 = Minus + 5.0f;
    if (Add1 != FVector2(4.0f, 3.0f))
    {
        TEST_FAILED();
    }

    // Subtraction
    FVector2 Sub0 = Add1 - FVector2(3.0f, 1.0f);
    if (Sub0 != FVector2(1.0f, 2.0f))
    {
        TEST_FAILED();
    }

    FVector2 Sub1 = Add1 - 5.0f;
    if (Sub1 != FVector2(-1.0f, -2.0f))
    {
        TEST_FAILED();
    }

    // Multiplication
    FVector2 Mul0 = Sub0 * FVector2(3.0f, 1.0f);
    if (Mul0 != FVector2(3.0f, 2.0f))
    {
        TEST_FAILED();
    }

    FVector2 Mul1 = Sub0 * 5.0f;
    if (Mul1 != FVector2(5.0f, 10.0f))
    {
        TEST_FAILED();
    }

    // Division
    FVector2 Div0 = Mul0 / FVector2(3.0f, 1.0f);
    if (Div0 != FVector2(1.0f, 2.0f))
    {
        TEST_FAILED();
    }

    const FVector2 Div1 = Mul1 / 5.0f;
    if (Div1 != FVector2(1.0f, 2.0f))
    {
        TEST_FAILED();
    }

    // Get Component
    float Component = Div1[1];
    if (Component != 2.0f)
    {
        TEST_FAILED();
    }

    return true;
}