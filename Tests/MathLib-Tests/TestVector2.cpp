#include "MathTest.h"

#include <Core/Math/Vector2.h>

#include <cstdio>

bool TestVector2()
{
    // Constructors
    Vector2 Point0;
    Vector2 Point1(1.0f, 2.0f);

    float Arr[2] = { 5.0f, -7.0f };
    Vector2 Point2(Arr);
    Vector2 Point3(-3.0f);

    // Dot
    float Dot = Point1.DotProduct(Point3);
    if (Dot != -9.0f)
    {
        TEST_FAILED();
    }

    // Project On
    Vector2 v0 = Vector2(2.0f, 5.0f);
    Vector2 v1 = Vector2(1.0f, 0.0f);
    Vector2 Projected = v0.ProjectOn(v1);

    if (Projected != Vector2(2.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // Min
    Vector2 MinPoint = Min(Point2, Point3);
    if (MinPoint != Vector2(-3.0f, -7.0f))
    {
        TEST_FAILED();
    }

    // Max
    Vector2 MaxPoint = Max(Point2, Point3);
    if (MaxPoint != Vector2(5.0f, -3.0f))
    {
        TEST_FAILED();
    }

    // Lerp
    Vector2 Lerped = Lerp(Vector2(0.0f), Vector2(1.0f), 0.5f);
    if (Lerped != Vector2(0.5f, 0.5f))
    {
        TEST_FAILED();
    }

    // Clamp
    Vector2 Clamped = Clamp(Vector2(-2.0f), Vector2(5.0f), Vector2(-3.5f, 7.5f));
    if (Clamped != Vector2(-2.0f, 5.0f))
    {
        TEST_FAILED();
    }

    // Saturate
    Vector2 Saturated = Saturate(Vector2(-5.0f, 1.5f));
    if (Saturated != Vector2(0.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Normalize
    Vector2 Norm(1.0f);
    Norm.Normalize();

    if (Norm != Vector2(0.70710678118f, 0.70710678118f))
    {
        TEST_FAILED();
    }

    if (!Norm.IsUnitVector())
    {
        TEST_FAILED();
    }

    // NaN
    Vector2 NaN(1.0f, NAN);
    if (!NaN.HasNaN())
    {
        TEST_FAILED();
    }

    // Infinity
    Vector2 Infinity(1.0f, INFINITY);
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
    Vector2 LengthVector(2.0f, 2.0f);
    float Length = LengthVector.Length();

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
    Vector2 Minus = -Point1;
    if (Minus != Vector2(-1.0f, -2.0f))
    {
        TEST_FAILED();
    }

    // Add
    Vector2 Add0 = Minus + Vector2(3.0f, 1.0f);
    if (Add0 != Vector2(2.0f, -1.0f))
    {
        TEST_FAILED();
    }

    Vector2 Add1 = Minus + 5.0f;
    if (Add1 != Vector2(4.0f, 3.0f))
    {
        TEST_FAILED();
    }

    // Subtraction
    Vector2 Sub0 = Add1 - Vector2(3.0f, 1.0f);
    if (Sub0 != Vector2(1.0f, 2.0f))
    {
        TEST_FAILED();
    }

    Vector2 Sub1 = Add1 - 5.0f;
    if (Sub1 != Vector2(-1.0f, -2.0f))
    {
        TEST_FAILED();
    }

    // Multiplication
    Vector2 Mul0 = Sub0 * Vector2(3.0f, 1.0f);
    if (Mul0 != Vector2(3.0f, 2.0f))
    {
        TEST_FAILED();
    }

    Vector2 Mul1 = Sub0 * 5.0f;
    if (Mul1 != Vector2(5.0f, 10.0f))
    {
        TEST_FAILED();
    }

    // Division
    Vector2 Div0 = Mul0 / Vector2(3.0f, 1.0f);
    if (Div0 != Vector2(1.0f, 2.0f))
    {
        TEST_FAILED();
    }

    const Vector2 Div1 = Mul1 / 5.0f;
    if (Div1 != Vector2(1.0f, 2.0f))
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