#include "MathTest.h"

#include <Core/Math/Vector4.h>

#include <cstdio>

#define _XM_NO_INTRINSICS_
#include <DirectXMath.h>
using namespace DirectX;

bool TestVector4()
{
    // Constructors
    CVector4 Point0;

    CVector4 Point1(1.0f, 2.0f, -2.0f, 4.0f);

    float Arr[4] = { 5.0f, -7.0f, 2.0f, 4.0f };
    CVector4 Point2(Arr);

    CVector4 Point3(-3.0f);

    // Dot
    float Dot = Point1.DotProduct(Point3);
    if (Dot != -15.0f)
    {
        TEST_FAILED();
    }

    // Cross
    CVector4 Cross = Point1.CrossProduct(Point2);

    XMVECTOR Xm0 = XMVectorSet(1.0f, 2.0f, -2.0f, 4.0f);
    XMVECTOR Xm1 = XMVectorSet(5.0f, -7.0f, 2.0f, 4.0f);
    XMVECTOR XmCross = XMVector3Cross(Xm0, Xm1);

    XMFLOAT4 XmFloat4;
    XMStoreFloat4(&XmFloat4, XmCross);

    if (Cross != CVector4(reinterpret_cast<float*>(&XmFloat4)))
    {
        TEST_FAILED();
    }

    // Project On
    CVector4 v0 = CVector4(4.0f, 5.0f, 3.0f, 10.0f);
    CVector4 v1 = CVector4(1.0f, 0.0f, 0.0f, 0.0f);
    CVector4 Projected = v0.ProjectOn(v1);

    if (Projected != CVector4(4.0f, 0.0f, 0.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // Reflection
    CVector4 Reflect = Point1.Reflect(CVector4(0.0f, 1.0f, 0.0f, 0.0f));

    XMVECTOR Xm2 = XMVectorSet(1.0f, 2.0f, -2.0f, 4.0f);
    XMVECTOR Xm3 = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR XmReflect = XMVector4Reflect(Xm2, Xm3);

    XMStoreFloat4(&XmFloat4, XmReflect);

    if (Reflect != CVector4(reinterpret_cast<float*>(&XmFloat4)))
    {
        TEST_FAILED();
    }

    // Min
    CVector4 MinPoint = Min(Point2, Point3);
    if (MinPoint != CVector4(-3.0f, -7.0f, -3.0f, -3.0f))
    {
        TEST_FAILED();
    }

    // Max
    CVector4 MaxPoint = Max(Point2, Point3);
    if (MaxPoint != CVector4(5.0f, -3.0f, 2.0f, 4.0f))
    {
        TEST_FAILED();
    }

    // Lerp
    CVector4 Lerped = Lerp(CVector4(0.0f), CVector4(1.0f), 0.5f);
    if (Lerped != CVector4(0.5f))
    {
        TEST_FAILED();
    }

    // Clamp
    CVector4 Clamped = Clamp(CVector4(-2.0f), CVector4(5.0f), CVector4(-3.5f, 7.5f, 1.0f, 2.0f));
    if (Clamped != CVector4(-2.0f, 5.0f, 1.0f, 2.0f))
    {
        TEST_FAILED();
    }

    // Saturate
    CVector4 Saturated = Saturate(CVector4(-5.0f, 1.5f, 0.25f, -5.7f));
    if (Saturated != CVector4(0.0f, 1.0f, 0.25f, 0.0f))
    {
        TEST_FAILED();
    }

    // Normalize
    CVector4 Norm(1.0f);
    CVector4 Normalize = Norm.GetNormalized();

    Norm.Normalize();
    if (Norm != CVector4(0.5f))
    {
        TEST_FAILED();
    }

    if (Normalize != CVector4(0.5f))
    {
        TEST_FAILED();
    }

    if (!Norm.IsUnitVector())
    {
        TEST_FAILED();
    }

    // NaN
    CVector4 NaN(1.0f, 0.0f, 0.0f, NAN);
    if (!NaN.HasNan())
    {
        TEST_FAILED();
    }

    // Infinity
    CVector4 Infinity(1.0f, 0.0f, 0.0f, INFINITY);
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
    CVector4 LengthVector(2.0f, 2.0f, 2.0f, 2.0f);
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
    CVector4 Minus = -Point1;
    if (Minus != CVector4(-1.0f, -2.0f, 2.0f, -4.0f))
    {
        TEST_FAILED();
    }

    // Add
    CVector4 Add0 = Minus + CVector4(3.0f, 1.0f, -1.0f, 2.0f);
    if (Add0 != CVector4(2.0f, -1.0f, 1.0f, -2.0f))
    {
        TEST_FAILED();
    }

    CVector4 Add1 = Minus + 5.0f;
    if (Add1 != CVector4(4.0f, 3.0f, 7.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Subtraction
    CVector4 Sub0 = Add1 - CVector4(3.0f, 1.0f, 8.0f, 3.0f);
    if (Sub0 != CVector4(1.0f, 2.0f, -1.0f, -2.0f))
    {
        TEST_FAILED();
    }

    CVector4 Sub1 = Add1 - 5.0f;
    if (Sub1 != CVector4(-1.0f, -2.0f, 2.0f, -4.0f))
    {
        TEST_FAILED();
    }

    // Multiplication
    CVector4 Mul0 = Sub0 * CVector4(3.0f, 1.0f, 2.0f, -1.0f);
    if (Mul0 != CVector4(3.0f, 2.0f, -2.0f, 2.0f))
    {
        TEST_FAILED();
    }

    CVector4 Mul1 = Sub0 * 5.0f;
    if (Mul1 != CVector4(5.0f, 10.0f, -5.0f, -10.0f))
    {
        TEST_FAILED();
    }

    // Division
    CVector4 Div0 = Mul0 / CVector4(3.0f, 1.0f, 2.0f, 2.0f);
    if (Div0 != CVector4(1.0f, 2.0f, -1.0f, 1.0f))
    {
        TEST_FAILED();
    }

    const CVector4 Div1 = Mul1 / 5.0f;
    if (Div1 != CVector4(1.0f, 2.0f, -1.0f, -2.0f))
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