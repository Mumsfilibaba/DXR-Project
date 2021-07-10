#pragma once
#include "Vector3.h"

/* A 3x3 matrix */
class CMatrix3
{
public:
    /* Default constructor (Initialize components to zero) */
    FORCEINLINE CMatrix3() noexcept;

    /**
     * Constructor initializing all values on the diagonal with a single value. The other values are set to zero.
     *
     * @param Diagonal: Value to set on the diagonal
     */
    FORCEINLINE explicit CMatrix3( float Diagonal ) noexcept;

    /**
     * Constructor initializing all values with vectors specifying each row
     *
     * @param Row0: Vector to set the first row to
     * @param Row1: Vector to set the second row to
     * @param Row2: Vector to set the third row to
     */
    FORCEINLINE explicit CMatrix3( const CVector3& Row0, const CVector3& Row1, const CVector3& Row2 ) noexcept;

    /**
     * Constructor initializing all values with corresponding value
     *
     * @param In00: Value to set on row 0 and column 0
     * @param In01: Value to set on row 0 and column 1
     * @param In02: Value to set on row 0 and column 2
     * @param In10: Value to set on row 1 and column 0
     * @param In11: Value to set on row 1 and column 1
     * @param In12: Value to set on row 1 and column 2
     * @param In20: Value to set on row 2 and column 0
     * @param In21: Value to set on row 2 and column 1
     * @param In22: Value to set on row 2 and column 2
     */
    FORCEINLINE explicit CMatrix3(
        float In00, float In01, float In02,
        float In10, float In11, float In12,
        float In20, float In21, float In22 ) noexcept;

    /**
     * Constructor initializing all components with an array
     *
     * @param Arr: Array with at least 9 elements
     */
    FORCEINLINE explicit CMatrix3( const float* Arr ) noexcept;

    /**
     * Returns the transposed version of this matrix
     *
     * @return Transposed matrix
     */
    inline CMatrix3 Transpose() const noexcept;

    /**
     * Returns the inverted version of this matrix
     *
     * @return Inverse matrix
     */
    inline CMatrix3 Invert() const noexcept;

    /**
     * Returns the adjuagte of this matrix
     *
     * @return Adjugate matrix
     */
    inline CMatrix3 Adjoint() const noexcept;

    /**
     * Returns the determinant of this matrix
     *
     * @return The determinant
     */
    inline float Determinant() const noexcept;

    /**
     * Checks weather this matrix has any value that equals NaN
     *
     * @return True if the any value equals NaN, false if not
     */
    inline bool HasNan() const noexcept;

    /**
     * Checks weather this matrix has any value that equals infinity
     *
     * @return True if the any value equals infinity, false if not
     */
    inline bool HasInfinity() const noexcept;

    /**
     * Checks weather this matrix has any value that equals infinity or NaN
     *
     * @return False if the any value equals infinity or NaN, true if not
     */
    FORCEINLINE bool IsValid() const noexcept;

    /**
     * Compares, within a threshold Epsilon, this matrix with another matrix
     *
     * @param Other: matrix to compare against
     * @return True if equal, false if not
     */
    inline bool IsEqual( const CMatrix3& Other, float Epsilon = NMath::IS_EQUAL_EPISILON ) const noexcept;

    /* Sets this matrix to an identity matrix */
    FORCEINLINE void SetIdentity() noexcept;

    /**
     * Returns a row of this matrix
     *
     * @param Row: The row to retrive
     * @return A vector containing the specified row
     */
    FORCEINLINE CVector3 GetRow( int Row ) const noexcept;

    /**
     * Returns a column of this matrix
     *
     * @param Column: The column to retrive
     * @return A vector containing the specified column
     */
    FORCEINLINE CVector3 GetColumn( int Column ) const noexcept;

    /**
     * Returns the data of this matrix as a pointer
     *
     * @return A pointer to the data
     */
    FORCEINLINE float* GetData() noexcept;

    /**
     * Returns the data of this matrix as a pointer
     *
     * @return A pointer to the data
     */
    FORCEINLINE const float* GetData() const noexcept;

public:
    /**
     * Transforms a 3-D vector
     *
     * @param RHS: The vector to transform
     * @return A vector containing the transformation
     */
    FORCEINLINE CVector3 operator*( const CVector3& RHS ) const noexcept;

    /**
     * Multiplies a matrix with another matrix
     *
     * @param RHS: The other matrix
     * @return A matrix containing the result of the multiplication
     */
    FORCEINLINE CMatrix3 operator*( const CMatrix3& RHS ) const noexcept;

    /**
     * Multiplies this matrix with another matrix
     *
     * @param RHS: The other matrix
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix3& operator*=( const CMatrix3& RHS ) noexcept;

    /**
     * Multiplies a matrix component-wise with a scalar
     *
     * @param RHS: The scalar
     * @return A matrix containing the result of the multiplication
     */
    FORCEINLINE CMatrix3 operator*( float RHS ) const noexcept;

    /**
     * Multiplies this matrix component-wise with a scalar
     *
     * @param RHS: The scalar
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix3& operator*=( float RHS ) noexcept;

    /**
     * Adds a matrix component-wise with another matrix
     *
     * @param RHS: The other matrix
     * @return A matrix containing the result of the addition
     */
    FORCEINLINE CMatrix3 operator+( const CMatrix3& RHS ) const noexcept;

    /**
     * Adds this matrix component-wise with another matrix
     *
     * @param RHS: The other matrix
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix3& operator+=( const CMatrix3& RHS ) noexcept;

    /**
     * Adds a matrix component-wise with a scalar
     *
     * @param RHS: The scalar
     * @return A matrix containing the result of the addition
     */
    FORCEINLINE CMatrix3 operator+( float RHS ) const noexcept;

    /**
     * Adds this matrix component-wise with a scalar
     *
     * @param RHS: The scalar
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix3& operator+=( float RHS ) noexcept;

    /**
     * Subtracts a matrix component-wise with another matrix
     *
     * @param RHS: The other matrix
     * @return A matrix containing the result of the subtraction
     */
    FORCEINLINE CMatrix3 operator-( const CMatrix3& RHS ) const noexcept;

    /**
     * Subtracts this matrix component-wise with another matrix
     *
     * @param RHS: The other matrix
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix3& operator-=( const CMatrix3& RHS ) noexcept;

    /**
     * Subtracts a matrix component-wise with a scalar
     *
     * @param RHS: The scalar
     * @return A matrix containing the result of the subtraction
     */
    FORCEINLINE CMatrix3 operator-( float RHS ) const noexcept;

    /**
     * Subtracts this matrix component-wise with a scalar
     *
     * @param RHS: The scalar
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix3& operator-=( float RHS ) noexcept;

    /**
     * Divides a matrix component-wise with a scalar
     *
     * @param RHS: The scalar
     * @return A matrix containing the result of the division
     */
    FORCEINLINE CMatrix3 operator/( float RHS ) const noexcept;

    /**
     * Divides this matrix component-wise with a scalar
     *
     * @param RHS: The scalar
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix3& operator/=( float RHS ) noexcept;

    /**
     * Returns the result after comparing this and another matrix
     *
     * @param Other: The matrix to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==( const CMatrix3& Other ) const noexcept;

    /**
     * Returns the negated result after comparing this and another matrix
     *
     * @param Other: The matrix to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=( const CMatrix3& Other ) const noexcept;

public:
    /**
     * Creates and returns a identity matrix
     *
     * @return A identity matrix
     */
    inline static CMatrix3 Identity() noexcept;

    /**
     * Creates and returns a uniform scale matrix
     *
     * @param Scale: Uniform scale that represents this matrix
     * @return A scale matrix
     */
    inline static CMatrix3 Scale( float Scale ) noexcept;

    /**
     * Creates and returns a scale matrix for each axis
     *
     * @param x: Scale for the x-axis
     * @param y: Scale for the y-axis
     * @param z: Scale for the z-axis
     * @return A scale matrix
     */
    inline static CMatrix3 Scale( float x, float y, float z ) noexcept;

    /**
     * Creates and returns a scale matrix for each axis
     *
     * @param VectorWithScale: A vector containing the scale for each axis in the x-, y-, z-components
     * @return A scale matrix
     */
    inline static CMatrix3 Scale( const CVector3& VectorWithScale ) noexcept;

    /**
     * Creates and returns a rotation matrix from Roll, pitch, and Yaw in radians
     *
     * @param Pitch: Rotation around the x-axis in radians
     * @param Yaw: Rotation around the y-axis in radians
     * @param Roll: Rotation around the z-axis in radians
     * @return A rotation matrix
     */
    inline static CMatrix3 RotationRollPitchYaw( float Pitch, float Yaw, float Roll ) noexcept;

    /**
     * Creates and returns a rotation matrix around the x-axis
     *
     * @param x: Rotation around the x-axis in radians
     * @return A rotation matrix
     */
    inline static CMatrix3 RotationX( float x ) noexcept;

    /**
     * Creates and returns a rotation matrix around the y-axis
     *
     * @param y: Rotation around the y-axis in radians
     * @return A rotation matrix
     */
    inline static CMatrix3 RotationY( float y ) noexcept;

    /**
     * Creates and returns a rotation matrix around the z-axis
     *
     * @param z: Rotation around the z-axis in radians
     * @return A rotation matrix
     */
    inline static CMatrix3 RotationZ( float z ) noexcept;

public:
    union
    {
        /* Each element of the matrix */
        struct
        {
            float m00, m01, m02;
            float m10, m11, m12;
            float m20, m21, m22;
        };

        /* 2-D array of the matrix */
        float f[3][3];
    };
};

FORCEINLINE CMatrix3::CMatrix3() noexcept
    : m00( 0.0f ), m01( 0.0f ), m02( 0.0f )
    , m10( 0.0f ), m11( 0.0f ), m12( 0.0f )
    , m20( 0.0f ), m21( 0.0f ), m22( 0.0f )
{
}

FORCEINLINE CMatrix3::CMatrix3( float Diagonal ) noexcept
    : m00( Diagonal ), m01( 0.0f ), m02( 0.0f )
    , m10( 0.0f ), m11( Diagonal ), m12( 0.0f )
    , m20( 0.0f ), m21( 0.0f ), m22( Diagonal )
{
}

FORCEINLINE CMatrix3::CMatrix3( const CVector3& Row0, const CVector3& Row1, const CVector3& Row2 ) noexcept
    : m00( Row0.x ), m01( Row0.y ), m02( Row0.z )
    , m10( Row1.x ), m11( Row1.y ), m12( Row1.z )
    , m20( Row2.x ), m21( Row2.y ), m22( Row2.z )
{
}

FORCEINLINE CMatrix3::CMatrix3(
    float In00, float In01, float In02,
    float In10, float In11, float In12,
    float In20, float In21, float In22 ) noexcept
    : m00( In00 ), m01( In01 ), m02( In02 )
    , m10( In10 ), m11( In11 ), m12( In12 )
    , m20( In20 ), m21( In21 ), m22( In22 )
{
}

FORCEINLINE CMatrix3::CMatrix3( const float* Arr ) noexcept
    : m00( Arr[0] ), m01( Arr[1] ), m02( Arr[2] )
    , m10( Arr[3] ), m11( Arr[4] ), m12( Arr[5] )
    , m20( Arr[6] ), m21( Arr[7] ), m22( Arr[8] )
{
}

inline CMatrix3 CMatrix3::Transpose() const noexcept
{
    CMatrix3 Transpose;
    Transpose.f[0][0] = f[0][0];
    Transpose.f[0][1] = f[1][0];
    Transpose.f[0][2] = f[2][0];

    Transpose.f[1][0] = f[0][1];
    Transpose.f[1][1] = f[1][1];
    Transpose.f[1][2] = f[2][1];

    Transpose.f[2][0] = f[0][2];
    Transpose.f[2][1] = f[1][2];
    Transpose.f[2][2] = f[2][2];
    return Transpose;
}

inline CMatrix3 CMatrix3::Invert() const noexcept
{
    CMatrix3 Inverse;

    //d11
    Inverse.m00 = (m11 * m22) - (m12 * m21);
    //d12
    Inverse.m10 = -((m10 * m22) - (m12 * m20));
    //d13
    Inverse.m20 = (m10 * m21) - (m11 * m20);

    const float Determinant = (m00 * Inverse.m00) - (m01 * Inverse.m10) + (m02 * Inverse.m20);
    const float RecipDeterminant = 1.0f / Determinant;

    Inverse.m00 *= RecipDeterminant;
    Inverse.m10 *= RecipDeterminant;
    Inverse.m20 *= RecipDeterminant;

    //d21 
    Inverse.m01 = -((m01 * m22) - (m02 * m21)) * RecipDeterminant;
    //d22
    Inverse.m11 = ((m00 * m22) - (m02 * m20)) * RecipDeterminant;
    //d23
    Inverse.m21 = -((m00 * m21) - (m01 * m20)) * RecipDeterminant;

    //d31
    Inverse.m02 = ((m01 * m12) - (m02 * m11)) * RecipDeterminant;
    //d32
    Inverse.m12 = -((m00 * m12) - (m02 * m10)) * RecipDeterminant;
    //d33
    Inverse.m22 = ((m00 * m11) - (m01 * m10)) * RecipDeterminant;
    return Inverse;
}

inline CMatrix3 CMatrix3::Adjoint() const noexcept
{
    CMatrix3 Adjugate;

    //d11
    Adjugate.m00 = ((m11 * m22) - (m12 * m21));
    //d12
    Adjugate.m10 = -((m10 * m22) - (m12 * m20));
    //d13
    Adjugate.m20 = ((m10 * m21) - (m11 * m20));

    //d21 
    Adjugate.m01 = -((m01 * m22) - (m02 * m21));
    //d22
    Adjugate.m11 = ((m00 * m22) - (m02 * m20));
    //d23
    Adjugate.m21 = -((m00 * m21) - (m01 * m20));

    //d31
    Adjugate.m02 = ((m01 * m12) - (m02 * m11));
    //d32
    Adjugate.m12 = -((m00 * m12) - (m02 * m10));
    //d33
    Adjugate.m22 = ((m00 * m11) - (m01 * m10));
    return Adjugate;
}

inline float CMatrix3::Determinant() const noexcept
{
    float a = m00 * ((m11 * m22) - (m12 * m21));
    float b = m01 * ((m10 * m22) - (m12 * m20));
    float c = m02 * ((m10 * m21) - (m11 * m20));
    return a - b + c;
}

inline bool CMatrix3::HasNan() const noexcept
{
    for ( int i = 0; i < 9; i++ )
    {
        if ( isnan( reinterpret_cast<const float*>(this)[i] ) )
        {
            return true;
        }
    }

    return false;
}

inline bool CMatrix3::HasInfinity() const noexcept
{
    for ( int i = 0; i < 9; i++ )
    {
        if ( isinf( reinterpret_cast<const float*>(this)[i] ) )
        {
            return true;
        }
    }

    return false;
}

FORCEINLINE bool CMatrix3::IsValid() const noexcept
{
    return !HasNan() && !HasInfinity();
}

inline bool CMatrix3::IsEqual( const CMatrix3& Other, float Epsilon ) const noexcept
{
    Epsilon = fabsf( Epsilon );

    for ( int i = 0; i < 9; i++ )
    {
        float Diff = reinterpret_cast<const float*>(this)[i] - reinterpret_cast<const float*>(&Other)[i];
        if ( fabsf( Diff ) > Epsilon )
        {
            return false;
        }
    }

    return true;
}

FORCEINLINE void CMatrix3::SetIdentity() noexcept
{
    m00 = 1.0f;
    m01 = 0.0f;
    m02 = 0.0f;

    m10 = 0.0f;
    m11 = 1.0f;
    m12 = 0.0f;

    m20 = 0.0f;
    m21 = 0.0f;
    m22 = 1.0f;
}

FORCEINLINE CVector3 CMatrix3::GetRow( int Row ) const noexcept
{
    ASSERT( Row < 3 );
    return CVector3( f[Row] );
}

FORCEINLINE CVector3 CMatrix3::GetColumn( int Column ) const noexcept
{
    ASSERT( Column < 3 );
    return CVector3( f[0][Column], f[1][Column], f[2][Column] );
}

FORCEINLINE float* CMatrix3::GetData() noexcept
{
    return reinterpret_cast<float*>(this);
}

FORCEINLINE const float* CMatrix3::GetData() const noexcept
{
    return reinterpret_cast<const float*>(this);
}

FORCEINLINE bool CMatrix3::operator==( const CMatrix3& Other ) const noexcept
{
    return IsEqual( Other );
}

FORCEINLINE bool CMatrix3::operator!=( const CMatrix3& Other ) const noexcept
{
    return !IsEqual( Other );
}

FORCEINLINE CVector3 CMatrix3::operator*( const CVector3& RHS ) const noexcept
{
    CVector3 Result;
    Result.x = (RHS[0] * m00) + (RHS[1] * m10) + (RHS[2] * m20);
    Result.y = (RHS[0] * m01) + (RHS[1] * m11) + (RHS[2] * m21);
    Result.z = (RHS[0] * m02) + (RHS[1] * m12) + (RHS[2] * m22);
    return Result;
}

FORCEINLINE CMatrix3 CMatrix3::operator*( const CMatrix3& RHS ) const noexcept
{
    CMatrix3 Result;
    Result.m00 = (m00 * RHS.m00) + (m01 * RHS.m10) + (m02 * RHS.m20);
    Result.m01 = (m00 * RHS.m01) + (m01 * RHS.m11) + (m02 * RHS.m21);
    Result.m02 = (m00 * RHS.m02) + (m01 * RHS.m12) + (m02 * RHS.m22);

    Result.m10 = (m10 * RHS.m00) + (m11 * RHS.m10) + (m12 * RHS.m20);
    Result.m11 = (m10 * RHS.m01) + (m11 * RHS.m11) + (m12 * RHS.m21);
    Result.m12 = (m10 * RHS.m02) + (m11 * RHS.m12) + (m12 * RHS.m22);

    Result.m20 = (m20 * RHS.m00) + (m21 * RHS.m10) + (m22 * RHS.m20);
    Result.m21 = (m20 * RHS.m01) + (m21 * RHS.m11) + (m22 * RHS.m21);
    Result.m22 = (m20 * RHS.m02) + (m21 * RHS.m12) + (m22 * RHS.m22);
    return Result;
}

FORCEINLINE CMatrix3& CMatrix3::operator*=( const CMatrix3& RHS ) noexcept
{
    return *this = *this * RHS;
}

FORCEINLINE CMatrix3 CMatrix3::operator*( float RHS ) const noexcept
{
    CMatrix3 Result;
    Result.m00 = m00 * RHS;
    Result.m01 = m01 * RHS;
    Result.m02 = m02 * RHS;

    Result.m10 = m10 * RHS;
    Result.m11 = m11 * RHS;
    Result.m12 = m12 * RHS;

    Result.m20 = m20 * RHS;
    Result.m21 = m21 * RHS;
    Result.m22 = m22 * RHS;
    return Result;
}

FORCEINLINE CMatrix3& CMatrix3::operator*=( float RHS ) noexcept
{
    return *this = *this * RHS;
}

FORCEINLINE CMatrix3 CMatrix3::operator+( const CMatrix3& RHS ) const noexcept
{
    CMatrix3 Result;
    Result.m00 = m00 + RHS.m00;
    Result.m01 = m01 + RHS.m01;
    Result.m02 = m02 + RHS.m02;

    Result.m10 = m10 + RHS.m10;
    Result.m11 = m11 + RHS.m11;
    Result.m12 = m12 + RHS.m12;

    Result.m20 = m20 + RHS.m20;
    Result.m21 = m21 + RHS.m21;
    Result.m22 = m22 + RHS.m22;
    return Result;
}

FORCEINLINE CMatrix3& CMatrix3::operator+=( const CMatrix3& RHS ) noexcept
{
    return *this = *this + RHS;
}

FORCEINLINE CMatrix3 CMatrix3::operator+( float RHS ) const noexcept
{
    CMatrix3 Result;
    Result.m00 = m00 + RHS;
    Result.m01 = m01 + RHS;
    Result.m02 = m02 + RHS;

    Result.m10 = m10 + RHS;
    Result.m11 = m11 + RHS;
    Result.m12 = m12 + RHS;

    Result.m20 = m20 + RHS;
    Result.m21 = m21 + RHS;
    Result.m22 = m22 + RHS;
    return Result;
}

FORCEINLINE CMatrix3& CMatrix3::operator+=( float RHS ) noexcept
{
    return *this = *this + RHS;
}

FORCEINLINE CMatrix3 CMatrix3::operator-( const CMatrix3& RHS ) const noexcept
{
    CMatrix3 Result;
    Result.m00 = m00 - RHS.m00;
    Result.m01 = m01 - RHS.m01;
    Result.m02 = m02 - RHS.m02;

    Result.m10 = m10 - RHS.m10;
    Result.m11 = m11 - RHS.m11;
    Result.m12 = m12 - RHS.m12;

    Result.m20 = m20 - RHS.m20;
    Result.m21 = m21 - RHS.m21;
    Result.m22 = m22 - RHS.m22;
    return Result;
}

FORCEINLINE CMatrix3& CMatrix3::operator-=( const CMatrix3& RHS ) noexcept
{
    return *this = *this - RHS;
}

FORCEINLINE CMatrix3 CMatrix3::operator-( float RHS ) const noexcept
{
    CMatrix3 Result;
    Result.m00 = m00 - RHS;
    Result.m01 = m01 - RHS;
    Result.m02 = m02 - RHS;

    Result.m10 = m10 - RHS;
    Result.m11 = m11 - RHS;
    Result.m12 = m12 - RHS;

    Result.m20 = m20 - RHS;
    Result.m21 = m21 - RHS;
    Result.m22 = m22 - RHS;
    return Result;
}

FORCEINLINE CMatrix3& CMatrix3::operator-=( float RHS ) noexcept
{
    return *this = *this - RHS;
}

FORCEINLINE CMatrix3 CMatrix3::operator/( float RHS ) const noexcept
{
    const float Recip = 1.0f / RHS;

    CMatrix3 Result;
    Result.m00 = m00 * Recip;
    Result.m01 = m01 * Recip;
    Result.m02 = m02 * Recip;

    Result.m10 = m10 * Recip;
    Result.m11 = m11 * Recip;
    Result.m12 = m12 * Recip;

    Result.m20 = m20 * Recip;
    Result.m21 = m21 * Recip;
    Result.m22 = m22 * Recip;
    return Result;
}

FORCEINLINE CMatrix3& CMatrix3::operator/=( float RHS ) noexcept
{
    return *this = *this / RHS;
}

inline CMatrix3 CMatrix3::Identity() noexcept
{
    return CMatrix3( 1.0f );
}

inline CMatrix3 CMatrix3::Scale( float Scale ) noexcept
{
    return CMatrix3( Scale );
}

inline CMatrix3 CMatrix3::Scale( float x, float y, float z ) noexcept
{
    return CMatrix3(
        x, 0.0f, 0.0f,
        0.0f, y, 0.0f,
        0.0f, 0.0f, z );
}

inline CMatrix3 CMatrix3::Scale( const CVector3& VectorWithScale ) noexcept
{
    return Scale( VectorWithScale.x, VectorWithScale.y, VectorWithScale.z );
}

inline CMatrix3 CMatrix3::RotationRollPitchYaw( float Pitch, float Yaw, float Roll ) noexcept
{
    const float SinP = sinf( Pitch );
    const float SinY = sinf( Yaw );
    const float SinR = sinf( Roll );
    const float CosP = cosf( Pitch );
    const float CosY = cosf( Yaw );
    const float CosR = cosf( Roll );

    const float SinRSinP = SinR * SinP;
    const float CosRSinP = CosR * SinP;

    return CMatrix3(
        (CosR * CosY) + (SinRSinP * SinY), (SinR * CosP), (SinRSinP * CosY) - (CosR * SinY),
        (CosRSinP * SinY) - (SinR * CosY), (CosR * CosP), (SinR * SinY) + (CosRSinP * CosY),
        (CosP * SinY), -SinP, (CosP * CosY) );
}

inline CMatrix3 CMatrix3::RotationX( float x ) noexcept
{
    const float SinX = sinf( x );
    const float CosX = cosf( x );

    return CMatrix3(
        1.0f, 0.0f, 0.0f,
        0.0f, CosX, SinX,
        0.0f, -SinX, CosX );
}

inline CMatrix3 CMatrix3::RotationY( float y ) noexcept
{
    const float SinY = sinf( y );
    const float CosY = cosf( y );

    return CMatrix3(
        CosY, 0.0f, -SinY,
        0.0f, 1.0f, 0.0f,
        SinY, 0.0f, CosY );
}

inline CMatrix3 CMatrix3::RotationZ( float z ) noexcept
{
    const float SinZ = sinf( z );
    const float CosZ = cosf( z );

    return CMatrix3(
        CosZ, SinZ, 0.0f,
        -SinZ, CosZ, 0.0f,
        0.0f, 0.0f, 1.0f );
}
