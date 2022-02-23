#include "MathTest.h"

#include <Core/Math/Matrix2.h>

#include <cstdio>

#define _XM_NO_INTRINSICS_
#include <DirectXMath.h>
using namespace DirectX;

inline CMatrix2 ToMatrix2(const XMFLOAT3X3& Matrix)
{
    return CMatrix2(Matrix._11, Matrix._12, Matrix._21, Matrix._22);
}

bool TestMatrix2()
{
    // Identity
    CMatrix2 Identity = CMatrix2::Identity();
    if (Identity != CMatrix2(1.0f, 0.0f, 0.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Constructors
    CMatrix2 Test = CMatrix2(5.0f);
    if (Test != CMatrix2(5.0f, 0.0f, 0.0f, 5.0f))
    {
        TEST_FAILED();
    }

    Test = CMatrix2(
        CVector2(1.0f, 0.0f),
        CVector2(0.0f, 1.0f));
    if (Identity != Test)
    {
        TEST_FAILED();
    }

    float Arr[9] =
    {
        1.0f, 2.0f,
        3.0f, 4.0f,
    };

    Test = CMatrix2(Arr);
    if (Test != CMatrix2(1.0f, 2.0f, 3.0f, 4.0f))
    {
        TEST_FAILED();
    }

    // Transpose
    Test = Test.Transpose();
    if (Test != CMatrix2(1.0f, 3.0f, 2.0f, 4.0f))
    {
        TEST_FAILED();
    }

    // Determinant
    CMatrix2 Scale = CMatrix2::Scale(6.0f);
    float fDeterminant0 = Scale.Determinant();

    XMMATRIX XmScale = XMMatrixScaling(6.0f, 6.0f, 1.0f);
    float fDeterminant1 = XMVectorGetX(XMMatrixDeterminant(XmScale));

    if (fDeterminant0 != fDeterminant1)
    {
        TEST_FAILED();
    }

    XMFLOAT3X3 Float3x3Matrix;

    // Rotation
    for (double Angle = -NMath::TWO_PI; Angle < NMath::TWO_PI; Angle += NMath::ONE_DEGREE)
    {
        CMatrix2 Rotation = CMatrix2::Rotation((float)Angle);
        XMMATRIX XmRotation = XMMatrixRotationZ((float)Angle);

        XMStoreFloat3x3(&Float3x3Matrix, XmRotation);

        if (Rotation != ToMatrix2(Float3x3Matrix))
        {
            TEST_FAILED();
        }
    }

    // Multiplication
    CMatrix2 Mat0 = CMatrix2::Rotation(NMath::HALF_PI_F);
    CMatrix2 Mat1 = CMatrix2::Rotation(NMath::HALF_PI_F);
    CMatrix2 Mult = Mat0 * Mat1;

    XMMATRIX XmMat0 = XMMatrixRotationZ(NMath::HALF_PI_F);
    XMMATRIX XmMat1 = XMMatrixRotationZ(NMath::HALF_PI_F);
    XMMATRIX XmMult = XMMatrixMultiply(XmMat0, XmMat1);

    XMStoreFloat3x3(&Float3x3Matrix, XmMult);

    if (Mult != ToMatrix2(Float3x3Matrix))
    {
        TEST_FAILED();
    }

    // Division
    CMatrix2 Div0(1.0f);
    if (Div0 / 2.0f != CMatrix2(0.5f))
    {
        TEST_FAILED();
    }

    // Sub
    CMatrix2 Sub(1.0f);
    if (Div0 - 0.5f != CMatrix2(0.5f, -0.5f, -0.5f, 0.5f))
    {
        TEST_FAILED();
    }

    CMatrix2 Sub0(1.0f);
    CMatrix2 Sub1(1.0f);
    if (Sub0 - Sub1 != CMatrix2(0.0f))
    {
        TEST_FAILED();
    }

    // Add
    CMatrix2 Add(0.0f);
    CMatrix2 TempAdd(0.5f, 0.5f, 0.5f, 0.5f);
    if (Add + 0.5f != TempAdd)
    {
        TEST_FAILED();
    }

    CMatrix2 Add0(1.0f);
    CMatrix2 Add1(1.0f);
    if (Add0 + Add1 != CMatrix2(2.0f))
    {
        TEST_FAILED();
    }

    // Inverse
    CMatrix2 Inverse = Mult.Invert();
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
    CMatrix2 Adjoint = Mult.Adjoint();
    CMatrix2 Inverse2 = Adjoint * (1.0f / fDeterminant0);

    if (Inverse != Inverse2)
    {
        TEST_FAILED();
    }

    CMatrix2 InvInverse = Inverse * fDeterminant0;
    CMatrix2 XmInvInverse = ToMatrix2(Float3x3Matrix) * fDeterminant1;

    if (InvInverse != XmInvInverse)
    {
        TEST_FAILED();
    }

    if (Adjoint != XmInvInverse)
    {
        TEST_FAILED();
    }

    // NaN
    CMatrix2 NaN(1.0f, 0.0f, 0.0f, NAN);
    if (NaN.HasNan() != true)
    {
        TEST_FAILED();
    }

    // Infinity
    CMatrix2 Infinity(1.0f, 0.0f, 0.0f, INFINITY);
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
    CVector2 Row = Infinity.GetRow(0);
    if (Row != CVector2(1.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // Column
    CVector2 Column = Infinity.GetColumn(0);
    if (Column != CVector2(1.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // SetIdentity
    Infinity.SetIdentity();
    if (Infinity != CMatrix2::Identity())
    {
        TEST_FAILED();
    }

    // GetData
    CMatrix2 Matrix0 = CMatrix2::Identity();
    CMatrix2 Matrix1 = CMatrix2(Matrix0.GetData());

    if (Matrix0 != Matrix1)
    {
        TEST_FAILED();
    }

    // Multiply a vector
    CMatrix2 Rot = CMatrix2::Rotation(NMath::HALF_PI_F);
    CVector2 TranslatedVector = Rot * CVector2(1.0f, 2.0f);

    XMVECTOR XmTranslatedVector = XMVectorSet(1.0f, 2.0f, 0.0f, 0.0f);
    XMMATRIX XmRot = XMMatrixRotationZ(NMath::HALF_PI_F);
    XmTranslatedVector = XMVector3Transform(XmTranslatedVector, XmRot);

    XMFLOAT2 XmFloat2;
    XMStoreFloat2(&XmFloat2, XmTranslatedVector);

    if (TranslatedVector != CVector2(reinterpret_cast<float*>(&XmFloat2)))
    {
        TEST_FAILED();
    }

    return true;
}