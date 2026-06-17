#include "MathTest.h"

#include <Core/Math/Matrix2.h>

#include <cstdio>

#define _XM_NO_INTRINSICS_
#include <DirectXMath.h>
using namespace DirectX;

inline Matrix2 ToMatrix2(const XMFLOAT3X3& Matrix)
{
    return Matrix2(Matrix._11, Matrix._12, Matrix._21, Matrix._22);
}

bool TestMatrix2()
{
    // Identity
    Matrix2 Identity = Matrix2::Identity();
    if (Identity != Matrix2(1.0f, 0.0f, 0.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Constructors
    Matrix2 Test = Matrix2(5.0f);
    if (Test != Matrix2(5.0f, 0.0f, 0.0f, 5.0f))
    {
        TEST_FAILED();
    }

    Test = Matrix2(
        Vector2(1.0f, 0.0f),
        Vector2(0.0f, 1.0f));
    if (Identity != Test)
    {
        TEST_FAILED();
    }

    float Arr[9] =
    {
        1.0f, 2.0f,
        3.0f, 4.0f,
    };

    Test = Matrix2(Arr);
    if (Test != Matrix2(1.0f, 2.0f, 3.0f, 4.0f))
    {
        TEST_FAILED();
    }

    // Transpose
    Test = Test.Transpose();
    if (Test != Matrix2(1.0f, 3.0f, 2.0f, 4.0f))
    {
        TEST_FAILED();
    }

    // Determinant
    Matrix2 Scale = Matrix2::Scale(6.0f);
    float fDeterminant0 = Scale.Determinant();

    XMMATRIX XmScale = XMMatrixScaling(6.0f, 6.0f, 1.0f);
    float fDeterminant1 = XMVectorGetX(XMMatrixDeterminant(XmScale));

    if (fDeterminant0 != fDeterminant1)
    {
        TEST_FAILED();
    }

    XMFLOAT3X3 Float3x3Matrix;

    // Rotation
    for (double Angle = -Math::TwoPI; Angle < Math::TwoPI; Angle += Math::OneDegree)
    {
        Matrix2 Rotation = Matrix2::Rotation((float)Angle);
        XMMATRIX XmRotation = XMMatrixRotationZ((float)Angle);

        XMStoreFloat3x3(&Float3x3Matrix, XmRotation);

        if (Rotation != ToMatrix2(Float3x3Matrix))
        {
            TEST_FAILED();
        }
    }

    // Multiplication
    Matrix2 Mat0 = Matrix2::Rotation(Math::Constants::HalfPI);
    Matrix2 Mat1 = Matrix2::Rotation(Math::Constants::HalfPI);
    Matrix2 Mult = Mat0 * Mat1;

    XMMATRIX XmMat0 = XMMatrixRotationZ(Math::Constants::HalfPI);
    XMMATRIX XmMat1 = XMMatrixRotationZ(Math::Constants::HalfPI);
    XMMATRIX XmMult = XMMatrixMultiply(XmMat0, XmMat1);

    XMStoreFloat3x3(&Float3x3Matrix, XmMult);

    if (Mult != ToMatrix2(Float3x3Matrix))
    {
        TEST_FAILED();
    }

    // Division
    Matrix2 Div0(1.0f);
    if (Div0 / 2.0f != Matrix2(0.5f))
    {
        TEST_FAILED();
    }

    // Sub
    Matrix2 Sub(1.0f);
    if (Div0 - 0.5f != Matrix2(0.5f, -0.5f, -0.5f, 0.5f))
    {
        TEST_FAILED();
    }

    Matrix2 Sub0(1.0f);
    Matrix2 Sub1(1.0f);
    if (Sub0 - Sub1 != Matrix2(0.0f))
    {
        TEST_FAILED();
    }

    // Add
    Matrix2 Add(0.0f);
    Matrix2 TempAdd(0.5f, 0.5f, 0.5f, 0.5f);
    if (Add + 0.5f != TempAdd)
    {
        TEST_FAILED();
    }

    Matrix2 Add0(1.0f);
    Matrix2 Add1(1.0f);
    if (Add0 + Add1 != Matrix2(2.0f))
    {
        TEST_FAILED();
    }

    // Inverse
    Matrix2 Inverse = Mult.Invert();
    fDeterminant0 = Mult.Determinant();

    XMVECTOR XmDeterminant;
    XMMATRIX XmInverse = XMMatrixInverse(&XmDeterminant, XmMult);
    fDeterminant1 = XMVectorGetX(XmDeterminant);

    XMStoreFloat3x3(&Float3x3Matrix, XmInverse);

    if (Inverse != ToMatrix2(Float3x3Matrix))
    {
        TEST_FAILED();
    }

    // Adjoint
    Matrix2 Adjoint = Mult.Adjoint();
    Matrix2 Inverse2 = Adjoint * (1.0f / fDeterminant0);

    if (Inverse != Inverse2)
    {
        TEST_FAILED();
    }

    Matrix2 InvInverse = Inverse * fDeterminant0;
    Matrix2 XmInvInverse = ToMatrix2(Float3x3Matrix) * fDeterminant1;

    if (InvInverse != XmInvInverse)
    {
        TEST_FAILED();
    }

    if (Adjoint != XmInvInverse)
    {
        TEST_FAILED();
    }

    // NaN
    Matrix2 NaN(1.0f, 0.0f, 0.0f, NAN);
    if (NaN.HasNaN() != true)
    {
        TEST_FAILED();
    }

    // Infinity
    Matrix2 Infinity(1.0f, 0.0f, 0.0f, INFINITY);
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
    Vector2 Row = Infinity.GetRow(0);
    if (Row != Vector2(1.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // Column
    Vector2 Column = Infinity.GetColumn(0);
    if (Column != Vector2(1.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // SetIdentity
    Infinity.SetIdentity();
    if (Infinity != Matrix2::Identity())
    {
        TEST_FAILED();
    }

    // Data
    Matrix2 Matrix0 = Matrix2::Identity();
    Matrix2 Matrix1 = Matrix2(Matrix0.Data());

    if (Matrix0 != Matrix1)
    {
        TEST_FAILED();
    }

    // Multiply a vector
    Matrix2 Rot = Matrix2::Rotation(Math::Constants::HalfPI);
    Vector2 TranslatedVector = Rot * Vector2(1.0f, 2.0f);

    XMVECTOR XmTranslatedVector = XMVectorSet(1.0f, 2.0f, 0.0f, 0.0f);
    XMMATRIX XmRot = XMMatrixRotationZ(Math::Constants::HalfPI);
    XmTranslatedVector = XMVector3Transform(XmTranslatedVector, XmRot);

    XMFLOAT2 XmFloat2;
    XMStoreFloat2(&XmFloat2, XmTranslatedVector);

    if (TranslatedVector != Vector2(reinterpret_cast<float*>(&XmFloat2)))
    {
        TEST_FAILED();
    }

    return true;
}