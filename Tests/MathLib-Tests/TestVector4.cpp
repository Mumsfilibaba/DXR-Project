#include "MathTest.h"

#include <Core/Math/Vector4.h>

#include <cstdio>

#define _XM_NO_INTRINSICS_
#include <DirectXMath.h>
using namespace DirectX;

bool TestVector4()
{
    // Constructors
    Vector4 Point0;
    Vector4 Point1(1.0f, 2.0f, -2.0f, 4.0f);

    float Arr[4] = { 5.0f, -7.0f, 2.0f, 4.0f };
    Vector4 Point2(Arr);

    Vector4 Point3(-3.0f);

    // Dot
    float Dot = Point1.DotProduct(Point3);
    if (Dot != -15.0f)
    {
        TEST_FAILED();
    }

    // Cross
    Vector4 Cross = Point1.CrossProduct(Point2);

    XMVECTOR Xm0 = XMVectorSet(1.0f, 2.0f, -2.0f, 4.0f);
    XMVECTOR Xm1 = XMVectorSet(5.0f, -7.0f, 2.0f, 4.0f);
    XMVECTOR XmCross = XMVector3Cross(Xm0, Xm1);

    XMFLOAT4 XmFloat4;
    XMStoreFloat4(&XmFloat4, XmCross);

    if (Cross != Vector4(reinterpret_cast<float*>(&XmFloat4)))
    {
        TEST_FAILED();
    }

    // Project On
    Vector4 v0 = Vector4(4.0f, 5.0f, 3.0f, 10.0f);
    Vector4 v1 = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
    Vector4 Projected = v0.ProjectOn(v1);

    if (Projected != Vector4(4.0f, 0.0f, 0.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // Reflection
    Vector4 Reflect = Point1.Reflect(Vector4(0.0f, 1.0f, 0.0f, 0.0f));

    XMVECTOR Xm2 = XMVectorSet(1.0f, 2.0f, -2.0f, 4.0f);
    XMVECTOR Xm3 = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR XmReflect = XMVector4Reflect(Xm2, Xm3);

    XMStoreFloat4(&XmFloat4, XmReflect);

    if (Reflect != Vector4(reinterpret_cast<float*>(&XmFloat4)))
    {
        TEST_FAILED();
    }

    // Min
    Vector4 MinPoint = Min(Point2, Point3);
    if (MinPoint != Vector4(-3.0f, -7.0f, -3.0f, -3.0f))
    {
        TEST_FAILED();
    }

    // Max
    Vector4 MaxPoint = Max(Point2, Point3);
    if (MaxPoint != Vector4(5.0f, -3.0f, 2.0f, 4.0f))
    {
        TEST_FAILED();
    }

    // Lerp
    Vector4 Lerped = Lerp(Vector4(0.0f), Vector4(1.0f), 0.5f);
    if (Lerped != Vector4(0.5f))
    {
        TEST_FAILED();
    }

    // Clamp
    Vector4 Clamped = Clamp(Vector4(-2.0f), Vector4(5.0f), Vector4(-3.5f, 7.5f, 1.0f, 2.0f));
    if (Clamped != Vector4(-2.0f, 5.0f, 1.0f, 2.0f))
    {
        TEST_FAILED();
    }

    // Saturate
    Vector4 Saturated = Saturate(Vector4(-5.0f, 1.5f, 0.25f, -5.7f));
    if (Saturated != Vector4(0.0f, 1.0f, 0.25f, 0.0f))
    {
        TEST_FAILED();
    }

    // Normalize
    Vector4 Norm(1.0f);
    Vector4 Normalize = Norm.GetNormalized();

    Norm.Normalize();
    if (Norm != Vector4(0.5f))
    {
        TEST_FAILED();
    }

    if (Normalize != Vector4(0.5f))
    {
        TEST_FAILED();
    }

    if (!Norm.IsUnitVector())
    {
        TEST_FAILED();
    }

    // NaN
    Vector4 NaN(1.0f, 0.0f, 0.0f, NAN);
    if (!NaN.HasNaN())
    {
        TEST_FAILED();
    }

    // Infinity
    Vector4 Infinity(1.0f, 0.0f, 0.0f, INFINITY);
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
    Vector4 LengthVector(2.0f, 2.0f, 2.0f, 2.0f);

    float Length = LengthVector.Length();
    if (Length != 4.0f)
    {
        TEST_FAILED();
    }

    // Length Squared
    float LengthSqrd = LengthVector.LengthSquared();
    if (LengthSqrd != 16.0f)
    {
        TEST_FAILED();
    }

    // Unary minus
    Vector4 Minus = -Point1;
    if (Minus != Vector4(-1.0f, -2.0f, 2.0f, -4.0f))
    {
        TEST_FAILED();
    }

    // Add
    Vector4 Add0 = Minus + Vector4(3.0f, 1.0f, -1.0f, 2.0f);
    if (Add0 != Vector4(2.0f, -1.0f, 1.0f, -2.0f))
    {
        TEST_FAILED();
    }

    Vector4 Add1 = Minus + 5.0f;
    if (Add1 != Vector4(4.0f, 3.0f, 7.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Subtraction
    Vector4 Sub0 = Add1 - Vector4(3.0f, 1.0f, 8.0f, 3.0f);
    if (Sub0 != Vector4(1.0f, 2.0f, -1.0f, -2.0f))
    {
        TEST_FAILED();
    }

    Vector4 Sub1 = Add1 - 5.0f;
    if (Sub1 != Vector4(-1.0f, -2.0f, 2.0f, -4.0f))
    {
        TEST_FAILED();
    }

    // Multiplication
    Vector4 Mul0 = Sub0 * Vector4(3.0f, 1.0f, 2.0f, -1.0f);
    if (Mul0 != Vector4(3.0f, 2.0f, -2.0f, 2.0f))
    {
        TEST_FAILED();
    }

    Vector4 Mul1 = Sub0 * 5.0f;
    if (Mul1 != Vector4(5.0f, 10.0f, -5.0f, -10.0f))
    {
        TEST_FAILED();
    }

    // Division
    Vector4 Div0 = Mul0 / Vector4(3.0f, 1.0f, 2.0f, 2.0f);
    if (Div0 != Vector4(1.0f, 2.0f, -1.0f, 1.0f))
    {
        TEST_FAILED();
    }

    const Vector4 Div1 = Mul1 / 5.0f;
    if (Div1 != Vector4(1.0f, 2.0f, -1.0f, -2.0f))
    {
        TEST_FAILED();
    }

    // Get Component
    float Component = Div1[3];
    if (Component != -2.0f)
    {
        TEST_FAILED();
    }

    return true;
}