#include "MathTest.h"

#include <Core/Math/Matrix3.h>

#include <cstdio>

#define _XM_NO_INTRINSICS_
#include <DirectXMath.h>
using namespace DirectX;

bool TestMatrix3()
{
    // Identity
    Matrix3 Identity = Matrix3::Identity();
    if (Identity != Matrix3(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Constructors
    Matrix3 Test = Matrix3(5.0f);
    if (Test != Matrix3(
        5.0f, 0.0f, 0.0f,
        0.0f, 5.0f, 0.0f,
        0.0f, 0.0f, 5.0f))
    {
        TEST_FAILED();
    }

    Test = Matrix3(
        Vector3(1.0f, 0.0f, 0.0f),
        Vector3(0.0f, 1.0f, 0.0f),
        Vector3(0.0f, 0.0f, 1.0f));
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

    Test = Matrix3(Arr);
    if (Test != Matrix3(
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f))
    {
        TEST_FAILED();
    }

    // Transpose
    Test = Test.Transpose();
    if (Test != Matrix3(
        1.0f, 4.0f, 7.0f,
        2.0f, 5.0f, 8.0f,
        3.0f, 6.0f, 9.0f))
    {
        TEST_FAILED();
    }

    // Determinant
    Matrix3 Scale = Matrix3::Scale(6.0f);
    float fDeterminant0 = Scale.Determinant();

    XMMATRIX XmScale = XMMatrixScaling(6.0f, 6.0f, 6.0f);
    float fDeterminant1 = XMVectorGetX(XMMatrixDeterminant(XmScale));

    if (fDeterminant0 != fDeterminant1)
    {
        TEST_FAILED();
    }

    XMFLOAT3X3 Float3x3Matrix;

    // Roll Pitch Yaw
    for (double Angle = -Math::TwoPI; Angle < Math::TwoPI; Angle += Math::OneDegree)
    {
        Matrix3 RollPitchYaw = Matrix3::RotationRollPitchYaw((float)Angle, (float)Angle, (float)Angle);
        XMMATRIX XmRollPitchYaw = XMMatrixRotationRollPitchYaw((float)Angle, (float)Angle, (float)Angle);

        XMStoreFloat3x3(&Float3x3Matrix, XmRollPitchYaw);

        if (RollPitchYaw != Matrix3(reinterpret_cast<float*>(&Float3x3Matrix)))
        {
            TEST_FAILED();
        }
    }

    // RotationX
    for (double Angle = -Math::TwoPI; Angle < Math::TwoPI; Angle += Math::OneDegree)
    {
        Matrix3 Rotation = Matrix3::RotationX((float)Angle);
        XMMATRIX XmRotation = XMMatrixRotationX((float)Angle);

        XMStoreFloat3x3(&Float3x3Matrix, XmRotation);

        if (Rotation != Matrix3(reinterpret_cast<float*>(&Float3x3Matrix)))
        {
            TEST_FAILED();
        }
    }

    // RotationY
    for (double Angle = -Math::TwoPI; Angle < Math::TwoPI; Angle += Math::OneDegree)
    {
        Matrix3 Rotation = Matrix3::RotationY((float)Angle);
        XMMATRIX XmRotation = XMMatrixRotationY((float)Angle);
        XMStoreFloat3x3(&Float3x3Matrix, XmRotation);

        if (Rotation != Matrix3(reinterpret_cast<float*>(&Float3x3Matrix)))
        {
            TEST_FAILED();
        }
    }

    // RotationZ
    for (double Angle = -Math::TwoPI; Angle < Math::TwoPI; Angle += Math::OneDegree)
    {
        Matrix3 Rotation = Matrix3::RotationZ((float)Angle);
        XMMATRIX XmRotation = XMMatrixRotationZ((float)Angle);
        XMStoreFloat3x3(&Float3x3Matrix, XmRotation);

        if (Rotation != Matrix3(reinterpret_cast<float*>(&Float3x3Matrix)))
        {
            TEST_FAILED();
        }
    }

    // Multiplication
    Matrix3 Mat0 = Matrix3::RotationX(Math::Constants::HalfPI);
    Matrix3 Mat1 = Matrix3::RotationY(Math::Constants::HalfPI);
    Matrix3 Mult = Mat0 * Mat1;

    XMMATRIX XmMat0 = XMMatrixRotationX(Math::Constants::HalfPI);
    XMMATRIX XmMat1 = XMMatrixRotationY(Math::Constants::HalfPI);
    XMMATRIX XmMult = XMMatrixMultiply(XmMat0, XmMat1);
    XMStoreFloat3x3(&Float3x3Matrix, XmMult);

    if (Mult != Matrix3(reinterpret_cast<float*>(&Float3x3Matrix)))
    {
        TEST_FAILED();
    }

    // Inverse
    Matrix3 Inverse = Mult.Invert();
    fDeterminant0 = Mult.Determinant();

    XMVECTOR XmDeterminant;
    XMMATRIX XmInverse = XMMatrixInverse(&XmDeterminant, XmMult);
    fDeterminant1 = XMVectorGetX(XmDeterminant);

    XMStoreFloat3x3(&Float3x3Matrix, XmInverse);

    if (Inverse != Matrix3(reinterpret_cast<float*>(&Float3x3Matrix)))
    {
        TEST_FAILED();
    }

    // Adjoint
    Matrix3 Adjoint = Mult.Adjoint();
    Matrix3 Inverse2 = Adjoint * (1.0f / fDeterminant0);

    if (Inverse != Inverse2)
    {
        TEST_FAILED();
    }

    Matrix3 InvInverse = Inverse * fDeterminant0;
    Matrix3 XmInvInverse = Matrix3(reinterpret_cast<float*>(&Float3x3Matrix)) * fDeterminant1;
    if (InvInverse != XmInvInverse)
    {
        TEST_FAILED();
    }

    if (Adjoint != XmInvInverse)
    {
        TEST_FAILED();
    }

    // NaN
    Matrix3 NaN(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, NAN);
    if (NaN.HasNaN() != true)
    {
        TEST_FAILED();
    }

    // Infinity
    Matrix3 Infinity(
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
    Vector3 Row = Infinity.GetRow(0);
    if (Row != Vector3(1.0f, 0.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // Column
    Vector3 Column = Infinity.GetColumn(0);
    if (Column != Vector3(1.0f, 0.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // SetIdentity
    Infinity.SetIdentity();
    if (Infinity != Matrix3::Identity())
    {
        TEST_FAILED();
    }

    // Data
    Matrix3 Matrix0 = Matrix3::Identity();
    Matrix3 Matrix1 = Matrix3(Matrix0.Data());

    if (Matrix0 != Matrix1)
    {
        TEST_FAILED();
    }

    // Multiply a vector
    Matrix3 Rot = Matrix3::RotationX(Math::Constants::HalfPI);
    Vector3 TranslatedVector = Rot * Vector3(1.0f, 1.0f, 1.0f);

    XMVECTOR XmTranslatedVector = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
    XMMATRIX XmRot = XMMatrixRotationX(Math::Constants::HalfPI);
    XmTranslatedVector = XMVector3Transform(XmTranslatedVector, XmRot);

    XMFLOAT3 XmFloat3;
    XMStoreFloat3(&XmFloat3, XmTranslatedVector);

    if (TranslatedVector != Vector3(reinterpret_cast<float*>(&XmFloat3)))
    {
        TEST_FAILED();
    }

    return true;
}