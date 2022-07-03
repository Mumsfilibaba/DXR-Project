#include <Core/Math/Matrix3.h>

#include <cstdio>

#define _XM_NO_INTRINSICS_
#include <DirectXMath.h>
using namespace DirectX;

bool TestMatrix3()
{
    // Identity
    FMatrix3 Identity = FMatrix3::Identity();
    if ( Identity != FMatrix3(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f ) )
    {
        assert( false ); return false;
    }

    // Constructors
    FMatrix3 Test = FMatrix3( 5.0f );
    if ( Test != FMatrix3(
        5.0f, 0.0f, 0.0f,
        0.0f, 5.0f, 0.0f,
        0.0f, 0.0f, 5.0f ) )
    {
        assert( false ); return false;
    }

    Test = FMatrix3(
        FVector3( 1.0f, 0.0f, 0.0f ),
        FVector3( 0.0f, 1.0f, 0.0f ),
        FVector3( 0.0f, 0.0f, 1.0f ) );
    if ( Identity != Test )
    {
        assert( false ); return false;
    }

    float Arr[9] =
    {
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f,
    };

    Test = FMatrix3( Arr );
    if ( Test != FMatrix3(
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f ) )
    {
        assert( false ); return false;
    }

    // Transpose
    Test = Test.Transpose();
    if ( Test != FMatrix3(
        1.0f, 4.0f, 7.0f,
        2.0f, 5.0f, 8.0f,
        3.0f, 6.0f, 9.0f ) )
    {
        assert( false ); return false;
    }

    // Determinant
    FMatrix3 Scale = FMatrix3::Scale( 6.0f );
    float fDeterminant0 = Scale.Determinant();

    XMMATRIX XmScale = XMMatrixScaling( 6.0f, 6.0f, 6.0f );
    float fDeterminant1 = XMVectorGetX( XMMatrixDeterminant( XmScale ) );

    if ( fDeterminant0 != fDeterminant1 )
    {
        assert( false ); return false;
    }

    XMFLOAT3X3 Float3x3Matrix;

    // Roll Pitch Yaw
    for ( double Angle = -NMath::TWO_PI; Angle < NMath::TWO_PI; Angle += NMath::ONE_DEGREE )
    {
        FMatrix3 RollPitchYaw = FMatrix3::RotationRollPitchYaw( (float)Angle, (float)Angle, (float)Angle );
        XMMATRIX XmRollPitchYaw = XMMatrixRotationRollPitchYaw( (float)Angle, (float)Angle, (float)Angle );

        XMStoreFloat3x3( &Float3x3Matrix, XmRollPitchYaw );

        if ( RollPitchYaw != FMatrix3( reinterpret_cast<float*>(&Float3x3Matrix) ) )
        {
            assert( false ); return false;
        }
    }

    // RotationX
    for ( double Angle = -NMath::TWO_PI; Angle < NMath::TWO_PI; Angle += NMath::ONE_DEGREE )
    {
        FMatrix3 Rotation = FMatrix3::RotationX( (float)Angle );
        XMMATRIX XmRotation = XMMatrixRotationX( (float)Angle );

        XMStoreFloat3x3( &Float3x3Matrix, XmRotation );

        if ( Rotation != FMatrix3( reinterpret_cast<float*>(&Float3x3Matrix) ) )
        {
            assert( false ); return false;
        }
    }

    // RotationY
    for ( double Angle = -NMath::TWO_PI; Angle < NMath::TWO_PI; Angle += NMath::ONE_DEGREE )
    {
        FMatrix3 Rotation = FMatrix3::RotationY( (float)Angle );
        XMMATRIX XmRotation = XMMatrixRotationY( (float)Angle );

        XMStoreFloat3x3( &Float3x3Matrix, XmRotation );

        if ( Rotation != FMatrix3( reinterpret_cast<float*>(&Float3x3Matrix) ) )
        {
            assert( false ); return false;
        }
    }

    // RotationZ
    for ( double Angle = -NMath::TWO_PI; Angle < NMath::TWO_PI; Angle += NMath::ONE_DEGREE )
    {
        FMatrix3 Rotation = FMatrix3::RotationZ( (float)Angle );
        XMMATRIX XmRotation = XMMatrixRotationZ( (float)Angle );

        XMStoreFloat3x3( &Float3x3Matrix, XmRotation );

        if ( Rotation != FMatrix3( reinterpret_cast<float*>(&Float3x3Matrix) ) )
        {
            assert( false ); return false;
        }
    }

    // Multiplication
    FMatrix3 Mat0 = FMatrix3::RotationX( NMath::kHalfPI_f );
    FMatrix3 Mat1 = FMatrix3::RotationY( NMath::kHalfPI_f );
    FMatrix3 Mult = Mat0 * Mat1;

    XMMATRIX XmMat0 = XMMatrixRotationX( NMath::kHalfPI_f );
    XMMATRIX XmMat1 = XMMatrixRotationY( NMath::kHalfPI_f );
    XMMATRIX XmMult = XMMatrixMultiply( XmMat0, XmMat1 );

    XMStoreFloat3x3( &Float3x3Matrix, XmMult );

    if ( Mult != FMatrix3( reinterpret_cast<float*>(&Float3x3Matrix) ) )
    {
        assert( false ); return false;
    }

    // Inverse
    FMatrix3 Inverse = Mult.Invert();
    fDeterminant0 = Mult.Determinant();

    XMVECTOR XmDeterminant;
    XMMATRIX XmInverse = XMMatrixInverse( &XmDeterminant, XmMult );
    fDeterminant1 = XMVectorGetX( XmDeterminant );

    XMStoreFloat3x3( &Float3x3Matrix, XmInverse );

    if ( Inverse != FMatrix3( reinterpret_cast<float*>(&Float3x3Matrix) ) )
    {
        assert( false ); return false;
    }

    // Adjoint
    FMatrix3 Adjoint = Mult.Adjoint();
    FMatrix3 Inverse2 = Adjoint * (1.0f / fDeterminant0);

    if ( Inverse != Inverse2 )
    {
        assert( false ); return false;
    }

    FMatrix3 InvInverse = Inverse * fDeterminant0;
    FMatrix3 XmInvInverse = FMatrix3( reinterpret_cast<float*>(&Float3x3Matrix) ) * fDeterminant1;

    if ( InvInverse != XmInvInverse )
    {
        assert( false ); return false;
    }

    if ( Adjoint != XmInvInverse )
    {
        assert( false ); return false;
    }

    // NaN
    FMatrix3 NaN(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, NAN );
    if ( NaN.HasNan() != true )
    {
        assert( false ); return false;
    }

    // Infinity
    FMatrix3 Infinity(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, INFINITY );
    if ( Infinity.HasInfinity() != true )
    {
        assert( false ); return false;
    }

    // Valid
    if ( NaN.IsValid() || Infinity.IsValid() )
    {
        assert( false ); return false;
    }

    // Get Row
    FVector3 Row = Infinity.GetRow( 0 );
    if ( Row != FVector3( 1.0f, 0.0f, 0.0f ) )
    {
        assert( false ); return false;
    }

    // Column
    FVector3 Column = Infinity.GetColumn( 0 );
    if ( Column != FVector3( 1.0f, 0.0f, 0.0f ) )
    {
        assert( false ); return false;
    }

    // SetIdentity
    Infinity.SetIdentity();
    if ( Infinity != FMatrix3::Identity() )
    {
        assert( false ); return false;
    }

    // GetData
    FMatrix3 Matrix0 = FMatrix3::Identity();
    FMatrix3 Matrix1 = FMatrix3( Matrix0.GetData() );

    if ( Matrix0 != Matrix1 )
    {
        assert( false ); return false;
    }

    // Multiply a vector
    FMatrix3 Rot = FMatrix3::RotationX( NMath::kHalfPI_f );
    FVector3 TranslatedVector = Rot * FVector3( 1.0f, 1.0f, 1.0f );

    XMVECTOR XmTranslatedVector = XMVectorSet( 1.0f, 1.0f, 1.0f, 0.0f );
    XMMATRIX XmRot = XMMatrixRotationX( NMath::kHalfPI_f );
    XmTranslatedVector = XMVector3Transform( XmTranslatedVector, XmRot );

    XMFLOAT3 XmFloat3;
    XMStoreFloat3( &XmFloat3, XmTranslatedVector );

    if ( TranslatedVector != FVector3( reinterpret_cast<float*>(&XmFloat3) ) )
    {
        assert( false ); return false;
    }

    return true;
}