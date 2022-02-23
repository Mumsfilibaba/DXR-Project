#include "MathTest.h"

#include <Core/Math/Matrix3.h>

#include <cstdio>

#define _XM_NO_INTRINSICS_
#include <DirectXMath.h>
using namespace DirectX;

bool TestMatrix3()
{
    // Identity
    CMatrix3 Identity = CMatrix3::Identity();
    if (Identity != CMatrix3(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Constructors
    CMatrix3 Test = CMatrix3(5.0f);
    if (Test != CMatrix3(
        5.0f, 0.0f, 0.0f,
        0.0f, 5.0f, 0.0f,
        0.0f, 0.0f, 5.0f))
    {
        TEST_FAILED();
    }

    Test = CMatrix3(
        CVector3(1.0f, 0.0f, 0.0f),
        CVector3(0.0f, 1.0f, 0.0f),
        CVector3(0.0f, 0.0f, 1.0f));
    if (Identity != Test)
    {
        TEST_FAILED();
    }

    float Arr[9] =
    {
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f,
    };

    Test = CMatrix3(Arr);
    if (Test != CMatrix3(
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f))
    {
        TEST_FAILED();
    }

    // Transpose
    Test = Test.Transpose();
    if (Test != CMatrix3(
        1.0f, 4.0f, 7.0f,
        2.0f, 5.0f, 8.0f,
        3.0f, 6.0f, 9.0f))
    {
        TEST_FAILED();
    }

    // Determinant
    CMatrix3 Scale = CMatrix3::Scale(6.0f);
    float fDeterminant0 = Scale.Determinant();

    XMMATRIX XmScale = XMMatrixScaling(6.0f, 6.0f, 6.0f);
    float fDeterminant1 = XMVectorGetX(XMMatrixDeterminant(XmScale));

    if (fDeterminant0 != fDeterminant1)
    {
        TEST_FAILED();
    }

    XMFLOAT3X3 Float3x3Matrix;

    // Roll Pitch Yaw
    for (double Angle = -NMath::TWO_PI; Angle < NMath::TWO_PI; Angle += NMath::ONE_DEGREE)
    {
        CMatrix3 RollPitchYaw = CMatrix3::RotationRollPitchYaw((float)Angle, (float)Angle, (float)Angle);
        XMMATRIX XmRollPitchYaw = XMMatrixRotationRollPitchYaw((float)Angle, (float)Angle, (float)Angle);

        XMStoreFloat3x3(&Float3x3Matrix, XmRollPitchYaw);

        if (RollPitchYaw != CMatrix3(reinterpret_cast<float*>(&Float3x3Matrix)))
        {
            TEST_FAILED();
        }
    }

    // RotationX
    for (double Angle = -NMath::TWO_PI; Angle < NMath::TWO_PI; Angle += NMath::ONE_DEGREE)
    {
        CMatrix3 Rotation = CMatrix3::RotationX((float)Angle);
        XMMATRIX XmRotation = XMMatrixRotationX((float)Angle);

        XMStoreFloat3x3(&Float3x3Matrix, XmRotation);

        if (Rotation != CMatrix3(reinterpret_cast<float*>(&Float3x3Matrix)))
        {
            TEST_FAILED();
        }
    }

    // RotationY
    for (double Angle = -NMath::TWO_PI; Angle < NMath::TWO_PI; Angle += NMath::ONE_DEGREE)
    {
        CMatrix3 Rotation = CMatrix3::RotationY((float)Angle);
        XMMATRIX XmRotation = XMMatrixRotationY((float)Angle);

        XMStoreFloat3x3(&Float3x3Matrix, XmRotation);

        if (Rotation != CMatrix3(reinterpret_cast<float*>(&Float3x3Matrix)))
        {
            TEST_FAILED();
        }
    }

    // RotationZ
    for (double Angle = -NMath::TWO_PI; Angle < NMath::TWO_PI; Angle += NMath::ONE_DEGREE)
    {
        CMatrix3 Rotation = CMatrix3::RotationZ((float)Angle);
        XMMATRIX XmRotation = XMMatrixRotationZ((float)Angle);

        XMStoreFloat3x3(&Float3x3Matrix, XmRotation);

        if (Rotation != CMatrix3(reinterpret_cast<float*>(&Float3x3Matrix)))
        {
            TEST_FAILED();
        }
    }

    // Multiplication
    CMatrix3 Mat0 = CMatrix3::RotationX(NMath::HALF_PI_F);
    CMatrix3 Mat1 = CMatrix3::RotationY(NMath::HALF_PI_F);
    CMatrix3 Mult = Mat0 * Mat1;

    XMMATRIX XmMat0 = XMMatrixRotationX(NMath::HALF_PI_F);
    XMMATRIX XmMat1 = XMMatrixRotationY(NMath::HALF_PI_F);
    XMMATRIX XmMult = XMMatrixMultiply(XmMat0, XmMat1);

    XMStoreFloat3x3(&Float3x3Matrix, XmMult);

    if (Mult != CMatrix3(reinterpret_cast<float*>(&Float3x3Matrix)))
    {
        TEST_FAILED();
    }

    // Inverse
    CMatrix3 Inverse = Mult.Invert();
    fDeterminant0 = Mult.Determinant();

    XMVECTOR XmDeterminant;
    XMMATRIX XmInverse = XMMatrixInverse(&XmDeterminant, XmMult);
    fDeterminant1 = XMVectorGetX(XmDeterminant);

    XMStoreFloat3x3(&Float3x3Matrix, XmInverse);

    if (Inverse != CMatrix3(reinterpret_cast<float*>(&Float3x3Matrix)))
    {
        TEST_FAILED();
    }

    // Adjoint
    CMatrix3 Adjoint = Mult.Adjoint();
    CMatrix3 Inverse2 = Adjoint * (1.0f / fDeterminant0);

    if (Inverse != Inverse2)
    {
        TEST_FAILED();
    }

    CMatrix3 InvInverse = Inverse * fDeterminant0;
    CMatrix3 XmInvInverse = CMatrix3(reinterpret_cast<float*>(&Float3x3Matrix)) * fDeterminant1;

    if (InvInverse != XmInvInverse)
    {
        TEST_FAILED();
    }

    if (Adjoint != XmInvInverse)
    {
        TEST_FAILED();
    }

    // NaN
    CMatrix3 NaN(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, NAN);
    if (NaN.HasNan() != true)
    {
        TEST_FAILED();
    }

    // Infinity
    CMatrix3 Infinity(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, INFINITY);
    if (Infinity.HasInfinity() != true)
    {
        TEST_FAILED();
    }

    // Valid
    if (NaN.IsValid() || Infinity.IsValid())
    {
        TEST_FAILED();
    }

    // Get Row
    CVector3 Row = Infinity.GetRow(0);
    if (Row != CVector3(1.0f, 0.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // Column
    CVector3 Column = Infinity.GetColumn(0);
    if (Column != CVector3(1.0f, 0.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // SetIdentity
    Infinity.SetIdentity();
    if (Infinity != CMatrix3::Identity())
    {
        TEST_FAILED();
    }

    // GetData
    CMatrix3 Matrix0 = CMatrix3::Identity();
    CMatrix3 Matrix1 = CMatrix3(Matrix0.GetData());

    if (Matrix0 != Matrix1)
    {
        TEST_FAILED();
    }

    // Multiply a vector
    CMatrix3 Rot = CMatrix3::RotationX(NMath::HALF_PI_F);
    CVector3 TranslatedVector = Rot * CVector3(1.0f, 1.0f, 1.0f);

    XMVECTOR XmTranslatedVector = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
    XMMATRIX XmRot = XMMatrixRotationX(NMath::HALF_PI_F);
    XmTranslatedVector = XMVector3Transform(XmTranslatedVector, XmRot);

    XMFLOAT3 XmFloat3;
    XMStoreFloat3(&XmFloat3, XmTranslatedVector);

    if (TranslatedVector != CVector3(reinterpret_cast<float*>(&XmFloat3)))
    {
        TEST_FAILED();
    }

    return true;
}