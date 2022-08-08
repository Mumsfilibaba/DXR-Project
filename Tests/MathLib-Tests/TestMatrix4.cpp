#include "MathTest.h"

#include <Core/Math/Matrix4.h>

#include <cstdio>

#define _XM_NO_INTRINSICS_
#include <DirectXMath.h>
using namespace DirectX;

bool TestMatrix4()
{
    // Identity
    FMatrix4 Identity = FMatrix4::Identity();
    if (Identity != FMatrix4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Constructors
    FMatrix4 Test = FMatrix4(5.0f);
    if (Test != FMatrix4(
        5.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 5.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 5.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 5.0f))
    {
        TEST_FAILED();
    }

    Test = FMatrix4(
        CVector4(1.0f, 0.0f, 0.0f, 0.0f),
        CVector4(0.0f, 1.0f, 0.0f, 0.0f),
        CVector4(0.0f, 0.0f, 1.0f, 0.0f),
        CVector4(0.0f, 0.0f, 0.0f, 1.0f));
    if (Identity != Test)
    {
        TEST_FAILED();
    }

    float Arr[16] =
    {
        1.0f,  2.0f,  3.0f,  4.0f,
        5.0f,  6.0f,  7.0f,  8.0f,
        9.0f,  10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f
    };

    Test = FMatrix4(Arr);
    if (Test != FMatrix4(
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f))
    {
        TEST_FAILED();
    }

    // Translation
    FMatrix4 Translation = FMatrix4::Translation(5.0f, 1.0f, -2.0f);
    if (Translation != FMatrix4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        5.0f, 1.0f, -2.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Transformation
    CVector3 Vec0 = CVector3(1.0f, 1.0f, 1.0f);
    CVector3 Vec1 = Translation.TransformPosition(Vec0);
    if (Vec1 != CVector3(6.0f, 2.0f, -1.0f))
    {
        TEST_FAILED();
    }

    Vec1 = Translation.TransformDirection(Vec0);
    if (Vec1 != CVector3(1.0f, 1.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Transpose
    Test = Test.Transpose();
    if (Test != FMatrix4(
        1.0f, 5.0f, 9.0f, 13.0f,
        2.0f, 6.0f, 10.0f, 14.0f,
        3.0f, 7.0f, 11.0f, 15.0f,
        4.0f, 8.0f, 12.0f, 16.0f))
    {
        TEST_FAILED();
    }

    // Determinant
    FMatrix4 Scale = FMatrix4::Scale(6.0f);
    float fDeterminant0 = Scale.Determinant();

    XMMATRIX XmScale = XMMatrixScaling(6.0f, 6.0f, 6.0f);
    float fDeterminant1 = XMVectorGetX(XMMatrixDeterminant(XmScale));

    if (fDeterminant0 != fDeterminant1)
    {
        TEST_FAILED();
    }

    // LookAt / Look To
    FMatrix4 LookAt = FMatrix4::LookAt(CVector3(0.0f, 0.0f, 1.0f), CVector3(0.0f), CVector3(0.0f, 1.0f, 0.0f));
    XMMATRIX XmLookAt = XMMatrixLookAtLH(
        XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
        XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

    XMFLOAT4X4 Float4x4Matrix;
    XMStoreFloat4x4(&Float4x4Matrix, XmLookAt);
    if (LookAt != FMatrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
    {
        TEST_FAILED();
    }

    // Perspective Projection
    float Width = 1920.0f;
    float Height = 1080.0f;
    float FOV = NMath::kPI_f / 2.0f;
    float Near = 0.01f;
    float Far = 100.0f;

    FMatrix4 Projection = FMatrix4::PerspectiveProjection(FOV, Width, Height, Near, Far);
    XMMATRIX XmProjection = XMMatrixPerspectiveFovLH(FOV, Width / Height, Near, Far);

    Float4x4Matrix;
    XMStoreFloat4x4(&Float4x4Matrix, XmProjection);
    if (Projection != FMatrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
    {
        TEST_FAILED();
    }

    // Multiplication
    FMatrix4 Mult = LookAt * Projection;
    XMMATRIX XmMult = XMMatrixMultiply(XmLookAt, XmProjection);

    Float4x4Matrix;
    XMStoreFloat4x4(&Float4x4Matrix, XmMult);

    if (Mult != FMatrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
    {
        TEST_FAILED();
    }

    FMatrix4 _Mul0(2.0);
    _Mul0 *= FMatrix4(2.0);

    XMFLOAT4X4 _Mul1(
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 2.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 2.0f);
    XMFLOAT4X4 _Mul2 = _Mul1;

    XMMATRIX XmMult0 = XMLoadFloat4x4(&_Mul1);
    XMMATRIX XmMult1 = XMLoadFloat4x4(&_Mul2);
    XmMult0 = XMMatrixMultiply(XmMult0, XmMult1);

    Float4x4Matrix;
    XMStoreFloat4x4(&Float4x4Matrix, XmMult0);

    if (_Mul0 != FMatrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
    {
        TEST_FAILED();
    }

    // Inverse
    FMatrix4 Inverse = Mult.Invert();
    fDeterminant0 = Mult.Determinant();

    XMVECTOR XmDeterminant;
    XMMATRIX XmInverse = XMMatrixInverse(&XmDeterminant, XmMult);
    fDeterminant1 = XMVectorGetX(XmDeterminant);

    Float4x4Matrix;
    XMStoreFloat4x4(&Float4x4Matrix, XmInverse);

    if (Inverse != FMatrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
    {
        TEST_FAILED();
    }

    // Adjoint
    FMatrix4 Adjoint = Mult.Adjoint();
    FMatrix4 Inverse2 = Adjoint * (1.0f / fDeterminant0);

    if (Inverse != Inverse2)
    {
        TEST_FAILED();
    }

    FMatrix4 InvInverse = Inverse * fDeterminant0;
    FMatrix4 XmInvInverse = FMatrix4(reinterpret_cast<float*>(&Float4x4Matrix)) * fDeterminant1;

    if (InvInverse != XmInvInverse)
    {
        TEST_FAILED();
    }

    if (Adjoint != XmInvInverse)
    {
        TEST_FAILED();
    }

    // NaN
    FMatrix4 NaN(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, NAN);
    if (NaN.HasNan() != true)
    {
        TEST_FAILED();
    }

    // Infinity
    FMatrix4 Infinity(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, INFINITY);
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
    CVector4 Row = Infinity.GetRow(0);
    if (Row != CVector4(1.0f, 0.0f, 0.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // Column
    CVector4 Column = Infinity.GetColumn(0);
    if (Column != CVector4(1.0f, 0.0f, 0.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // SetIdentity
    Infinity.SetIdentity();

    FMatrix4 TempIdentity = FMatrix4::Identity();
    if (Infinity != FMatrix4::Identity())
    {
        TEST_FAILED();
    }

    // GetTranslation
    CVector3 Position = Infinity.GetTranslation();
    if (Position != CVector3(0.0f))
    {
        TEST_FAILED();
    }

    // GetRotationAndScale
    CMatrix3 RotationAndScale = Infinity.GetRotationAndScale();
    if (RotationAndScale != CMatrix3::Identity())
    {
        TEST_FAILED();
    }

    // GetData
    FMatrix4 Matrix0 = FMatrix4::Identity();
    FMatrix4 Matrix1 = FMatrix4(Matrix0.GetData());
    if (Matrix0 != Matrix1)
    {
        TEST_FAILED();
    }

    // Multiply a vector
    Translation = FMatrix4::Translation(5.0f, 5.0f, 5.0f);
    CVector4 TranslatedVector = Translation * CVector4(0.0f, 0.0f, 0.0f, 1.0f);

    if (TranslatedVector != CVector4(5.0f, 5.0f, 5.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Roll Pitch Yaw
    for (double Angle = -NMath::TWO_PI; Angle < NMath::TWO_PI; Angle += NMath::ONE_DEGREE)
    {
        FMatrix4 RollPitchYaw = FMatrix4::RotationRollPitchYaw((float)Angle, (float)Angle, (float)Angle);
        XMMATRIX XmRollPitchYaw = XMMatrixRotationRollPitchYaw((float)Angle, (float)Angle, (float)Angle);
        XMStoreFloat4x4(&Float4x4Matrix, XmRollPitchYaw);

        if (RollPitchYaw != FMatrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
        {
            TEST_FAILED();
        }
    }

    // RotationX
    for (double Angle = -NMath::TWO_PI; Angle < NMath::TWO_PI; Angle += NMath::ONE_DEGREE)
    {
        FMatrix4 Rotation = FMatrix4::RotationX((float)Angle);
        XMMATRIX XmRotation = XMMatrixRotationX((float)Angle);
        XMStoreFloat4x4(&Float4x4Matrix, XmRotation);

        if (Rotation != FMatrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
        {
            TEST_FAILED();
        }
    }

    // RotationY
    for (double Angle = -NMath::TWO_PI; Angle < NMath::TWO_PI; Angle += NMath::ONE_DEGREE)
    {
        FMatrix4 Rotation = FMatrix4::RotationY((float)Angle);
        XMMATRIX XmRotation = XMMatrixRotationY((float)Angle);
        XMStoreFloat4x4(&Float4x4Matrix, XmRotation);

        if (Rotation != FMatrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
        {
            TEST_FAILED();
        }
    }

    // RotationZ
    for (double Angle = -NMath::TWO_PI; Angle < NMath::TWO_PI; Angle += NMath::ONE_DEGREE)
    {
        FMatrix4 Rotation = FMatrix4::RotationZ((float)Angle);
        XMMATRIX XmRotation = XMMatrixRotationZ((float)Angle);
        XMStoreFloat4x4(&Float4x4Matrix, XmRotation);

        if (Rotation != FMatrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
        {
            TEST_FAILED();
        }
    }

    // Ortographic projection
    FMatrix4 Ortographic = FMatrix4::OrtographicProjection(Width, Height, Near, Far);
    XMMATRIX XmOrtographic = XMMatrixOrthographicLH(Width, Height, Near, Far);
    XMStoreFloat4x4(&Float4x4Matrix, XmOrtographic);

    if (Ortographic != FMatrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
    {
        TEST_FAILED();
    }

    float Left = -10.0f;
    float Right = 10.0f;
    float Bottom = -10.0f;
    float Top = 10.0f;

    Ortographic = FMatrix4::OrtographicProjection(Left, Right, Bottom, Top, Near, Far);
    XmOrtographic = XMMatrixOrthographicOffCenterLH(Left, Right, Bottom, Top, Near, Far);
    XMStoreFloat4x4(&Float4x4Matrix, XmOrtographic);

    if (Ortographic != FMatrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
    {
        TEST_FAILED();
    }

    return true;
}