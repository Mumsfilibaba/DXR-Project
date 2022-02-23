#include "MathTest.h"

#include <Core/Math/Vector3.h>

#include <cstdio>

#define _XM_NO_INTRINSICS_
#include <DirectXMath.h>
using namespace DirectX;

bool TestVector3()
{
    // Constructors
    CVector3 Point0;

    CVector3 Point1(1.0f, 2.0f, -2.0f);

    float Arr[3] = { 5.0f, -7.0f, 2.0f };
    CVector3 Point2(Arr);

    CVector3 Point3(-3.0f);

    // Dot
    float Dot = Point1.DotProduct(Point3);
    if (Dot != -3.0f)
    {
        TEST_FAILED();
    }

    // Cross
    CVector3 Cross = Point1.CrossProduct(Point2);

    XMVECTOR Xm0 = XMVectorSet(1.0f, 2.0f, -2.0f, 0.0f);
    XMVECTOR Xm1 = XMVectorSet(5.0f, -7.0f, 2.0f, 0.0f);
    XMVECTOR XmCross = XMVector3Cross(Xm0, Xm1);

    XMFLOAT3 XmFloat3;
    XMStoreFloat3(&XmFloat3, XmCross);

    if (Cross != CVector3(reinterpret_cast<float*>(&XmFloat3)))
    {
        TEST_FAILED();
    }

    // Project On
    CVector3 v0 = CVector3(4.0f, 5.0f, 3.0f);
    CVector3 v1 = CVector3(1.0f, 0.0f, 0.0f);
    CVector3 Projected = v0.ProjectOn(v1);

    if (Projected != CVector3(4.0f, 0.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // Reflection
    CVector3 Reflect = Point1.Reflect(CVector3(0.0f, 1.0f, 0.0f));

    XMVECTOR Xm2 = XMVectorSet(1.0f, 2.0f, -2.0f, 0.0f);
    XMVECTOR Xm3 = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR XmReflect = XMVector3Reflect(Xm2, Xm3);

    XMStoreFloat3(&XmFloat3, XmReflect);

    if (Reflect != CVector3(reinterpret_cast<float*>(&XmFloat3)))
    {
        TEST_FAILED();
    }

    // Min
    CVector3 MinPoint = Min(Point2, Point3);
    if (MinPoint != CVector3(-3.0f, -7.0f, -3.0f))
    {
        TEST_FAILED();
    }

    // Max
    CVector3 MaxPoint = Max(Point2, Point3);
    if (MaxPoint != CVector3(5.0f, -3.0f, 2.0f))
    {
        TEST_FAILED();
    }

    // Lerp
    CVector3 Lerped = Lerp(CVector3(0.0f), CVector3(1.0f), 0.5f);
    if (Lerped != CVector3(0.5f))
    {
        TEST_FAILED();
    }

    // Clamp
    CVector3 Clamped = Clamp(CVector3(-2.0f), CVector3(5.0f), CVector3(-3.5f, 7.5f, 1.0f));
    if (Clamped != CVector3(-2.0f, 5.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Saturate
    CVector3 Saturated = Saturate(CVector3(-5.0f, 1.5f, 0.25f));
    if (Saturated != CVector3(0.0f, 1.0f, 0.25f))
    {
        TEST_FAILED();
    }

    // Normalize
    CVector3 Norm(1.0f);
    Norm.Normalize();

    if (Norm != CVector3(0.57735026919f))
    {
        TEST_FAILED();
    }

    if (!Norm.IsUnitVector())
    {
        TEST_FAILED();
    }

    // NaN
    CVector3 NaN(1.0f, 0.0f, NAN);
    if (!NaN.HasNan())
    {
        TEST_FAILED();
    }

    // Infinity
    CVector3 Infinity(1.0f, 0.0f, INFINITY);
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
    CVector3 LengthVector(2.0f, 2.0f, 2.0f);
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
    CVector3 Minus = -Point1;
    if (Minus != CVector3(-1.0f, -2.0f, 2.0f))
    {
        TEST_FAILED();
    }

    // Add
    CVector3 Add0 = Minus + CVector3(3.0f, 1.0f, -1.0f);
    if (Add0 != CVector3(2.0f, -1.0f, 1.0f))
    {
        TEST_FAILED();
    }

    CVector3 Add1 = Minus + 5.0f;
    if (Add1 != CVector3(4.0f, 3.0f, 7.0f))
    {
        TEST_FAILED();
    }

    // Subtraction
    CVector3 Sub0 = Add1 - CVector3(3.0f, 1.0f, 8.0f);
    if (Sub0 != CVector3(1.0f, 2.0f, -1.0f))
    {
        TEST_FAILED();
    }

    CVector3 Sub1 = Add1 - 5.0f;
    if (Sub1 != CVector3(-1.0f, -2.0f, 2.0f))
    {
        TEST_FAILED();
    }

    // Multiplication
    CVector3 Mul0 = Sub0 * CVector3(3.0f, 1.0f, 2.0f);
    if (Mul0 != CVector3(3.0f, 2.0f, -2.0f))
    {
        TEST_FAILED();
    }

    CVector3 Mul1 = Sub0 * 5.0f;
    if (Mul1 != CVector3(5.0f, 10.0f, -5.0f))
    {
        TEST_FAILED();
    }

    // Division
    CVector3 Div0 = Mul0 / CVector3(3.0f, 1.0f, 2.0f);
    if (Div0 != CVector3(1.0f, 2.0f, -1.0f))
    {
        TEST_FAILED();
    }

    const CVector3 Div1 = Mul1 / 5.0f;
    if (Div1 != CVector3(1.0f, 2.0f, -1.0f))
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