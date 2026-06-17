#include "MathTest.h"

#include <Core/Math/Matrix4.h>

#include <cstdio>

#define _XM_NO_INTRINSICS_
#include <DirectXMath.h>
using namespace DirectX;

bool TestMatrix4()
{
    // Identity
    Matrix4 Identity = Matrix4::Identity();
    if (Identity != Matrix4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Constructors
    Matrix4 Test = Matrix4(5.0f);
    if (Test != Matrix4(
        5.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 5.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 5.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 5.0f))
    {
        TEST_FAILED();
    }

    Test = Matrix4(
        Vector4(1.0f, 0.0f, 0.0f, 0.0f),
        Vector4(0.0f, 1.0f, 0.0f, 0.0f),
        Vector4(0.0f, 0.0f, 1.0f, 0.0f),
        Vector4(0.0f, 0.0f, 0.0f, 1.0f));
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

    Test = Matrix4(Arr);
    if (Test != Matrix4(
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f))
    {
        TEST_FAILED();
    }

    // Translation
    Matrix4 Translation = Matrix4::Translation(5.0f, 1.0f, -2.0f);
    if (Translation != Matrix4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        5.0f, 1.0f, -2.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Transformation
    Vector3 Vec0 = Vector3(1.0f, 1.0f, 1.0f);
    Vector3 Vec1 = Translation.TransformCoord(Vec0);
    if (Vec1 != Vector3(6.0f, 2.0f, -1.0f))
    {
        TEST_FAILED();
    }

    Vec1 = Translation.TransformNormal(Vec0);
    if (Vec1 != Vector3(1.0f, 1.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Transpose
    Test = Test.Transpose();
    if (Test != Matrix4(
        1.0f, 5.0f, 9.0f, 13.0f,
        2.0f, 6.0f, 10.0f, 14.0f,
        3.0f, 7.0f, 11.0f, 15.0f,
        4.0f, 8.0f, 12.0f, 16.0f))
    {
        TEST_FAILED();
    }

    // Determinant
    Matrix4 Scale = Matrix4::Scale(6.0f);
    float fDeterminant0 = Scale.Determinant();

    XMMATRIX XmScale = XMMatrixScaling(6.0f, 6.0f, 6.0f);
    float fDeterminant1 = XMVectorGetX(XMMatrixDeterminant(XmScale));

    if (fDeterminant0 != fDeterminant1)
    {
        TEST_FAILED();
    }

    // LookAt / Look To
    Matrix4 LookAt = Matrix4::LookAt(Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f), Vector3(0.0f, 1.0f, 0.0f));
    XMMATRIX XmLookAt = XMMatrixLookAtLH(
        XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
        XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

    XMFLOAT4X4 Float4x4Matrix;
    XMStoreFloat4x4(&Float4x4Matrix, XmLookAt);
    if (LookAt != Matrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
    {
        TEST_FAILED();
    }

    // Perspective Projection
    float Width  = 1920.0f;
    float Height = 1080.0f;
    float FOV    = Math::Constants::PI / 2.0f;
    float Near   = 0.01f;
    float Far    = 100.0f;

    Matrix4 Projection = Matrix4::PerspectiveProjection(FOV, Width, Height, Near, Far);
    XMMATRIX XmProjection = XMMatrixPerspectiveFovLH(FOV, Width / Height, Near, Far);

    Float4x4Matrix;
    XMStoreFloat4x4(&Float4x4Matrix, XmProjection);
    if (Projection != Matrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
    {
        TEST_FAILED();
    }

    // Multiplication
    Matrix4 Mult = LookAt * Projection;
    XMMATRIX XmMult = XMMatrixMultiply(XmLookAt, XmProjection);

    Float4x4Matrix;
    XMStoreFloat4x4(&Float4x4Matrix, XmMult);

    if (Mult != Matrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
    {
        TEST_FAILED();
    }

    Matrix4 _Mul0(2.0);
    _Mul0 *= Matrix4(2.0);

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

    if (_Mul0 != Matrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
    {
        TEST_FAILED();
    }

    // Inverse
    Matrix4 Inverse = Mult.Invert();
    fDeterminant0 = Mult.Determinant();

    XMVECTOR XmDeterminant;
    XMMATRIX XmInverse = XMMatrixInverse(&XmDeterminant, XmMult);
    fDeterminant1 = XMVectorGetX(XmDeterminant);

    Float4x4Matrix;
    XMStoreFloat4x4(&Float4x4Matrix, XmInverse);

    if (Inverse != Matrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
    {
        TEST_FAILED();
    }

    // Adjoint
    Matrix4 Adjoint = Mult.Adjoint();
    Matrix4 Inverse2 = Adjoint * (1.0f / fDeterminant0);

    if (Inverse != Inverse2)
    {
        TEST_FAILED();
    }

    Matrix4 InvInverse = Inverse * fDeterminant0;
    Matrix4 XmInvInverse = Matrix4(reinterpret_cast<float*>(&Float4x4Matrix)) * fDeterminant1;

    if (InvInverse != XmInvInverse)
    {
        TEST_FAILED();
    }

    if (Adjoint != XmInvInverse)
    {
        TEST_FAILED();
    }

    // NaN
    Matrix4 NaN(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, NAN);
    if (NaN.HasNaN() != true)
    {
        TEST_FAILED();
    }

    // Infinity
    Matrix4 Infinity(
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
    Vector4 Row = Infinity.GetRow(0);
    if (Row != Vector4(1.0f, 0.0f, 0.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // Column
    Vector4 Column = Infinity.GetColumn(0);
    if (Column != Vector4(1.0f, 0.0f, 0.0f, 0.0f))
    {
        TEST_FAILED();
    }

    // SetIdentity
    Infinity.SetIdentity();

    Matrix4 TempIdentity = Matrix4::Identity();
    if (Infinity != Matrix4::Identity())
    {
        TEST_FAILED();
    }

    // GetTranslation
    Vector3 Position = Infinity.GetTranslation();
    if (Position != Vector3(0.0f))
    {
        TEST_FAILED();
    }

    // GetRotationAndScale
    Matrix3 RotationAndScale = Infinity.GetRotationAndScale();
    if (RotationAndScale != Matrix3::Identity())
    {
        TEST_FAILED();
    }

    // Data
    Matrix4 Matrix0 = Matrix4::Identity();
    Matrix4 Matrix1 = Matrix4(Matrix0.Data());
    if (Matrix0 != Matrix1)
    {
        TEST_FAILED();
    }

    // Multiply a vector
    Translation = Matrix4::Translation(5.0f, 5.0f, 5.0f);
    Vector4 TranslatedVector = Translation * Vector4(0.0f, 0.0f, 0.0f, 1.0f);

    if (TranslatedVector != Vector4(5.0f, 5.0f, 5.0f, 1.0f))
    {
        TEST_FAILED();
    }

    // Roll Pitch Yaw
    for (double Angle = -Math::TwoPI; Angle < Math::TwoPI; Angle += Math::OneDegree)
    {
        Matrix4 RollPitchYaw = Matrix4::RotationRollPitchYaw((float)Angle, (float)Angle, (float)Angle);
        XMMATRIX XmRollPitchYaw = XMMatrixRotationRollPitchYaw((float)Angle, (float)Angle, (float)Angle);
        XMStoreFloat4x4(&Float4x4Matrix, XmRollPitchYaw);

        if (RollPitchYaw != Matrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
        {
            TEST_FAILED();
        }
    }

    // RotationX
    for (double Angle = -Math::TwoPI; Angle < Math::TwoPI; Angle += Math::OneDegree)
    {
        Matrix4 Rotation = Matrix4::RotationX((float)Angle);
        XMMATRIX XmRotation = XMMatrixRotationX((float)Angle);
        XMStoreFloat4x4(&Float4x4Matrix, XmRotation);

        if (Rotation != Matrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
        {
            TEST_FAILED();
        }
    }

    // RotationY
    for (double Angle = -Math::TwoPI; Angle < Math::TwoPI; Angle += Math::OneDegree)
    {
        Matrix4 Rotation = Matrix4::RotationY((float)Angle);
        XMMATRIX XmRotation = XMMatrixRotationY((float)Angle);
        XMStoreFloat4x4(&Float4x4Matrix, XmRotation);

        if (Rotation != Matrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
        {
            TEST_FAILED();
        }
    }

    // RotationZ
    for (double Angle = -Math::TwoPI; Angle < Math::TwoPI; Angle += Math::OneDegree)
    {
        Matrix4 Rotation = Matrix4::RotationZ((float)Angle);
        XMMATRIX XmRotation = XMMatrixRotationZ((float)Angle);
        XMStoreFloat4x4(&Float4x4Matrix, XmRotation);

        if (Rotation != Matrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
        {
            TEST_FAILED();
        }
    }

    // Ortographic projection
    Matrix4 Ortographic = Matrix4::OrtographicProjection(Width, Height, Near, Far);
    XMMATRIX XmOrtographic = XMMatrixOrthographicLH(Width, Height, Near, Far);
    XMStoreFloat4x4(&Float4x4Matrix, XmOrtographic);

    if (Ortographic != Matrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
    {
        TEST_FAILED();
    }

    float Left = -10.0f;
    float Right = 10.0f;
    float Bottom = -10.0f;
    float Top = 10.0f;

    Ortographic = Matrix4::OrtographicProjection(Left, Right, Bottom, Top, Near, Far);
    XmOrtographic = XMMatrixOrthographicOffCenterLH(Left, Right, Bottom, Top, Near, Far);
    XMStoreFloat4x4(&Float4x4Matrix, XmOrtographic);

    if (Ortographic != Matrix4(reinterpret_cast<float*>(&Float4x4Matrix)))
    {
        TEST_FAILED();
    }

    return true;
}