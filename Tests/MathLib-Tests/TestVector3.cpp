#include "MathTest.h"

#include <Core/Math/Vector3.h>

#include <cstdio>

#define _XM_NO_INTRINSICS_
#include <DirectXMath.h>
using namespace DirectX;

bool TestVector3()
{
    // Constructors
    FVector3 Point0;
    FVector3 Point1(1.0f, 2.0f, -2.0f);

    float Arr[3] = { 5.0f, -7.0f, 2.0f };
    FVector3 Point2(Arr);

    FVector3 Point3(-3.0f);

    // Dot
    float Dot = Point1.DotProduct(Point3);
    if (Dot != -3.0f)
    {
        TEST_FAILED();
    }

    // Cross
    FVector3 Cross = Point1.CrossProduct(Point2);

    XMVECTOR Xm0 = XMVectorSet(1.0f, 2.0f, -2.0f, 0.0f);
    XMVECTOR Xm1 = XMVectorSet(5.0f, -7.0f, 2.0f, 0.0f);
    XMVECTOR XmCross = XMVector3Cross(Xm0, Xm1);

    XMFLOAT3 XmFloat3;
    XMStoreFloat3(&XmFloat3, XmCross);

    if (Cross != FVector3(reinterpret_cast<float*>(&XmFloat3)))
    {
        TEST_FAILED();
    }

    // Project On
    FVector3 v0 = FVector3(4.0f, 5.0f, 3.0f);
    FVector3 v1 = FVector3(1.0f, 0.0f, 0.0f);
    FVector3 Projected = v0.ProjectOn(v1);

    if (Projected != FVector3(4.0f, 0.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // Reflection
    FVector3 Reflect = Point1.Reflect(FVector3(0.0f, 1.0f, 0.0f));

    XMVECTOR Xm2 = XMVectorSet(1.0f, 2.0f, -2.0f, 0.0f);
    XMVECTOR Xm3 = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR XmReflect = XMVector3Reflect(Xm2, Xm3);

    XMStoreFloat3(&XmFloat3, XmReflect);
    if (Reflect != FVector3(reinterpret_cast<float*>(&XmFloat3)))
    {
        TEST_FAILED();
    }

    // Min
    FVector3 MinPoint = Min(Point2, Point3);
    if (MinPoint != FVector3(-3.0f, -7.0f, -3.0f))
    {
        TEST_FAILED();
    }

    // Max
    FVector3 MaxPoint = Max(Point2, Point3);
    if (MaxPoint != FVector3(5.0f, -3.0f, 2.0f))
    {
        TEST_FAILED();
    }

    // Lerp
    FVector3 Lerped = Lerp(FVector3(0.0f), FVector3(1.0f), 0.5f);
    if (Lerped != FVector3(0.5f))
    {
        TEST_FAILED();
    }

    // Clamp
    FVector3 Clamped = Clamp(FVector3(-2.0f), FVector3(5.0f), FVector3(-3.5f, 7.5f, 1.0f));
    if (Clamped != FVector3(-2.0f, 5.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Saturate
    FVector3 Saturated = Saturate(FVector3(-5.0f, 1.5f, 0.25f));
    if (Saturated != FVector3(0.0f, 1.0f, 0.25f))
    {
        TEST_FAILED();
    }

    // Normalize
    FVector3 Norm(1.0f);
    Norm.Normalize();

    if (Norm != FVector3(0.57735026919f))
    {
        TEST_FAILED();
    }

    if (!Norm.IsUnitVector())
    {
        TEST_FAILED();
    }

    // NaN
    FVector3 NaN(1.0f, 0.0f, NAN);
    if (!NaN.HasNan())
    {
        TEST_FAILED();
    }

    // Infinity
    FVector3 Infinity(1.0f, 0.0f, INFINITY);
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
    FVector3 LengthVector(2.0f, 2.0f, 2.0f);
    float Length = LengthVector.Length();

    if (Length != 3.46410161514f)
    {
        TEST_FAILED();
    }

    // Length Squared
    float LengthSqrd = LengthVector.LengthSquared();
    if (LengthSqrd != 12.0f)
    {
        TEST_FAILED();
    }

    // Unary minus
    FVector3 Minus = -Point1;
    if (Minus != FVector3(-1.0f, -2.0f, 2.0f))
    {
        TEST_FAILED();
    }

    // Add
    FVector3 Add0 = Minus + FVector3(3.0f, 1.0f, -1.0f);
    if (Add0 != FVector3(2.0f, -1.0f, 1.0f))
    {
        TEST_FAILED();
    }

    FVector3 Add1 = Minus + 5.0f;
    if (Add1 != FVector3(4.0f, 3.0f, 7.0f))
    {
        TEST_FAILED();
    }

    // Subtraction
    FVector3 Sub0 = Add1 - FVector3(3.0f, 1.0f, 8.0f);
    if (Sub0 != FVector3(1.0f, 2.0f, -1.0f))
    {
        TEST_FAILED();
    }

    FVector3 Sub1 = Add1 - 5.0f;
    if (Sub1 != FVector3(-1.0f, -2.0f, 2.0f))
    {
        TEST_FAILED();
    }

    // Multiplication
    FVector3 Mul0 = Sub0 * FVector3(3.0f, 1.0f, 2.0f);
    if (Mul0 != FVector3(3.0f, 2.0f, -2.0f))
    {
        TEST_FAILED();
    }

    FVector3 Mul1 = Sub0 * 5.0f;
    if (Mul1 != FVector3(5.0f, 10.0f, -5.0f))
    {
        TEST_FAILED();
    }

    // Division
    FVector3 Div0 = Mul0 / FVector3(3.0f, 1.0f, 2.0f);
    if (Div0 != FVector3(1.0f, 2.0f, -1.0f))
    {
        TEST_FAILED();
    }

    const FVector3 Div1 = Mul1 / 5.0f;
    if (Div1 != FVector3(1.0f, 2.0f, -1.0f))
    {
        TEST_FAILED();
    }

    // Get Component
    float Component = Div1[2];
    if (Component != -1.0f)
    {
        TEST_FAILED();
    }

    return true;
}