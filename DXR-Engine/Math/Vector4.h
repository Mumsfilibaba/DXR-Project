#pragma once
#include "Vector3.h"
#include "SIMD.h"

/* A 4-D floating point vector (x, y, z, w) with SIMD capabilities */
class ALIGN_16 CVector4
{
public:
    /* Default constructor (Initialize components to zero) */
    FORCEINLINE CVector4() noexcept;

    /**
     * Constructor initializing all components with a corresponding value.
     *
     * @param InX: The x-coordinate
     * @param InY: The y-coordinate
     * @param InZ: The z-coordinate
     * @param InW: The w-coordinate
     */
    FORCEINLINE explicit CVector4( float InX, float InY, float InZ, float InW ) noexcept;

    /**
     * Constructor initializing all components with an array.
     *
     * @param Arr: Array with 4 elements
     */
    FORCEINLINE explicit CVector4( const float* Arr ) noexcept;

    /**
     * Constructor initializing all components with a single value.
     *
     * @param Scalar: Value to set all components to
     */
    FORCEINLINE explicit CVector4( float Scalar ) noexcept;

    /**
     * Constructor copying a 3-D vector (x, y, z) into the first components, setting w-component to zero
     *
     * @param XYZ: Value to set first components to
     */
    FORCEINLINE CVector4( const CVector3& XYZ ) noexcept;

    /**
     * Constructor copying a 3-D vector (x, y, z) into the first components, setting w-component to a specific value
     *
     * @param XYZ: Value to set first components to
     * @param InW: Value to set the w-component to
     */
    FORCEINLINE explicit CVector4( const CVector3& XYZ, float InW ) noexcept;

    /* Normalized this vector */
    inline void Normalize() noexcept;

    /**
     * Returns a normalized version of this vector
     *
     * @return A copy of this vector normalized
     */
    inline CVector4 GetNormalized() const noexcept;

    /**
     * Compares, within a threshold Epsilon, this vector with another vector
     *
     * @param Other: vector to compare against
     * @return True if equal, false if not
     */
    inline bool IsEqual( const CVector4& Other, float Epsilon = NMath::IS_EQUAL_EPISILON ) const noexcept;

    /**
     * Checks weather this vector is a unit vector not
     *
     * @return True if the length equals one, false if not
     */
    FORCEINLINE bool IsUnitVector() const noexcept;

    /**
     * Checks weather this vector has any component that equals NaN
     *
     * @return True if the any component equals NaN, false if not
     */
    FORCEINLINE bool HasNan() const noexcept;

    /**
     * Checks weather this vector has any component that equals infinity
     *
     * @return True if the any component equals infinity, false if not
     */
    FORCEINLINE bool HasInfinity() const noexcept;

    /**
     * Checks weather this vector has any value that equals infinity or NaN
     *
     * @return False if the any value equals infinity or NaN, true if not
     */
    FORCEINLINE bool IsValid() const noexcept;

    /**
     * Returns the length of this vector
     *
     * @return The length of the vector
     */
    FORCEINLINE float Length() const noexcept;

    /**
     * Returns the length of this vector squared
     *
     * @return The length of the vector squared
     */
    FORCEINLINE float LengthSquared() const noexcept;

    /**
     * Returns the dot product between this and another vector
     *
     * @param Other: The vector to perform dot product with
     * @return The dot product
     */
    FORCEINLINE float DotProduct( const CVector4& Other ) const noexcept;

    /**
     * Returns the cross product of this vector and another vector.
     * This function does not take the w-component into account
     *
     * @param Other: The vector to perform cross product with
     * @return The cross product
     */
    inline CVector4 CrossProduct( const CVector4& Other ) const noexcept;

    /**
     * Returns the resulting vector after projecting this vector onto another.
     *
     * @param Other: The vector to project onto
     * @return The projected vector
     */
    inline CVector4 ProjectOn( const CVector4& Other ) const noexcept;

    /**
     * Returns the reflected vector after reflecting this vector around a normal.
     *
     * @param Normal: Vector to reflect around
     * @return The reflected vector
     */
    inline CVector4 Reflect( const CVector4& Normal ) const noexcept;

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
     * Returns a vector with the smallest of each component of two vectors
     *
     * @param First: First vector to compare with
     * @param Second: Second vector to compare with
     * @return A vector with the smallest components of First and Second
     */
    friend FORCEINLINE CVector4 Min( const CVector4& First, const CVector4& Second ) noexcept;

    /**
     * Returns a vector with the largest of each component of two vectors
     *
     * @param First: First vector to compare with
     * @param Second: Second vector to compare with
     * @return A vector with the largest components of First and Second
     */
    friend FORCEINLINE CVector4 Max( const CVector4& First, const CVector4& Second ) noexcept;

    /**
     * Returns the linear interpolation between two vectors
     *
     * @param First: First vector to interpolate
     * @param Second: Second vector to interpolate
     * @param Factor: Factor to interpolate with. Zero returns First, One returns seconds
     * @return A vector with the result of interpolation
     */
    friend FORCEINLINE CVector4 Lerp( const CVector4& First, const CVector4& Second, float t ) noexcept;

    /**
     * Returns a vector with all the components within the range of a min and max value
     *
     * @param Min: Vector with minimum values
     * @param Max: Vector with maximum values
     * @param Value: Vector to clamp
     * @return A vector with the result of clamping
     */
    friend FORCEINLINE CVector4 Clamp( const CVector4& Min, const CVector4& Max, const CVector4& Value ) noexcept;

    /**
     * Returns a vector with all the components within the range zero and one
     *
     * @param Value: Value to saturate
     * @return A vector with the result of saturation
     */
    friend FORCEINLINE CVector4 Saturate( const CVector4& Value ) noexcept;

public:
    /**
     * Return a vector with component-wise negation of this vector
     *
     * @return A negated vector
     */
    FORCEINLINE CVector4 operator-() const noexcept;

    /**
     * Returns the result of component-wise adding this and another vector
     *
     * @param RHS: The vector to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CVector4 operator+( const CVector4& RHS ) const noexcept;

    /**
     * Returns this vector after component-wise adding this with another vector
     *
     * @param RHS: The vector to add
     * @return A reference to this vector
     */
    FORCEINLINE CVector4& operator+=( const CVector4& RHS ) noexcept;

    /**
     * Returns the result of adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CVector4 operator+( float RHS ) const noexcept;

    /**
     * Returns this vector after adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A reference to this vector
     */
    FORCEINLINE CVector4& operator+=( float RHS ) noexcept;

    /**
     * Returns the result of component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A vector with the result of subtraction
     */
    FORCEINLINE CVector4 operator-( const CVector4& RHS ) const noexcept;

    /**
     * Returns this vector after component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CVector4& operator-=( const CVector4& RHS ) noexcept;

    /**
     * Returns the result of subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A vector with the result of the subtraction
     */
    FORCEINLINE CVector4 operator-( float RHS ) const noexcept;

    /**
     * Returns this vector after subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CVector4& operator-=( float RHS ) noexcept;

    /**
     * Returns the result of component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CVector4 operator*( const CVector4& RHS ) const noexcept;

    /**
     * Returns this vector after component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CVector4& operator*=( const CVector4& RHS ) noexcept;

    /**
     * Returns the result of multipling each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CVector4 operator*( float RHS ) const noexcept;

    /**
     * Returns the result of multipling each component of a vector with a scalar
     *
     * @param LHS: The scalar to multiply with
     * @param RHS: The vector to multiply with
     * @return A vector with the result of the multiplication
     */
    friend FORCEINLINE CVector4 operator*( float LHS, const CVector4& RHS ) noexcept;

    /**
     * Returns this vector after multipling each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CVector4 operator*=( float RHS ) noexcept;

    /**
     * Returns the result of component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CVector4 operator/( const CVector4& RHS ) const noexcept;

    /**
     * Returns this vector after component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CVector4& operator/=( const CVector4& RHS ) noexcept;

    /**
     * Returns the result of dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CVector4 operator/( float RHS ) const noexcept;

    /**
     * Returns this vector after dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CVector4& operator/=( float RHS ) noexcept;

    /**
     * Returns the result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==( const CVector4& Other ) const noexcept;

    /**
     * Returns the negated result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=( const CVector4& Other ) const noexcept;

    /**
     * Returns the component specifed
     *
     * @param Index: The component index
     * @return The component
     */
    FORCEINLINE float& operator[]( int Index ) noexcept;

    /**
     * Returns the component specifed
     *
     * @param Index: The component index
     * @return The component
     */
    FORCEINLINE float operator[]( int Index ) const noexcept;

public:
    /* The x-coordinate */
    float x;

    /* The y-coordinate */
    float y;

    /* The z-coordinate */
    float z;

    /* The w-coordinate */
    float w;
};

FORCEINLINE CVector4::CVector4() noexcept
    : x( 0.0f )
    , y( 0.0f )
    , z( 0.0f )
    , w( 0.0f )
{
}

FORCEINLINE CVector4::CVector4( float InX, float InY, float InZ, float InW ) noexcept
    : x( InX )
    , y( InY )
    , z( InZ )
    , w( InW )
{
}

FORCEINLINE CVector4::CVector4( const float* Arr ) noexcept
    : x( Arr[0] )
    , y( Arr[1] )
    , z( Arr[2] )
    , w( Arr[3] )
{
}

FORCEINLINE CVector4::CVector4( float Scalar ) noexcept
    : x( Scalar )
    , y( Scalar )
    , z( Scalar )
    , w( Scalar )
{
}

FORCEINLINE CVector4::CVector4( const CVector3& XYZ ) noexcept
    : x( XYZ.x )
    , y( XYZ.y )
    , z( XYZ.z )
    , w( 0.0f )
{
}

FORCEINLINE CVector4::CVector4( const CVector3& XYZ, float InW ) noexcept
    : x( XYZ.x )
    , y( XYZ.y )
    , z( XYZ.z )
    , w( InW )
{
}

inline void CVector4::Normalize() noexcept
{
#if defined(DISABLE_SIMD)

    float fLength = Length();

    ASSERT( fLength != 0 );

    float fReprLength = 1.0f / fLength;
    x = x * fReprLength;
    y = y * fReprLength;
    z = z * fReprLength;
    w = w * fReprLength;

#else

    NSIMD::Float128 Reg0 = NSIMD::LoadAligned( this );
    NSIMD::Float128 Reg1 = NSIMD::Dot( Reg0, Reg0 );
    Reg1 = NSIMD::Shuffle<0, 1, 0, 1>( Reg1 );
    Reg1 = NSIMD::RecipSqrt( Reg1 );
    Reg0 = NSIMD::Mul( Reg0, Reg1 );
    NSIMD::StoreAligned( Reg0, this );

#endif
}

NOINLINE CVector4 CVector4::GetNormalized() const noexcept
{
    CVector4 Result( *this );
    Result.Normalize();
    return Result;
}

FORCEINLINE bool CVector4::IsEqual( const CVector4& Other, float Epsilon ) const noexcept
{
#if defined(DISABLE_SIMD)

    Epsilon = fabsf( Epsilon );

    for ( int i = 0; i < 4; i++ )
    {
        float Diff = reinterpret_cast<const float*>(this)[i] - reinterpret_cast<const float*>(&Other)[i];
        if ( fabsf( Diff ) > Epsilon )
        {
            return false;
        }
    }

    return true;

#else

    NSIMD::Float128 Espilon128 = NSIMD::Load( Epsilon );
    Espilon128 = NSIMD::Abs( Espilon128 );

    NSIMD::Float128 Diff = NSIMD::Sub( this, &Other );
    Diff = NSIMD::Abs( Diff );

    return NSIMD::LessThan( Diff, Espilon128 );

#endif
}

NOINLINE bool CVector4::IsUnitVector() const noexcept
{
    // LengthSquared should be the same as length if this is a unit vector
    // However, this way the sqrt can be avoided
    float fLengthSquared = fabsf( 1.0f - LengthSquared() );
    return (fLengthSquared < NMath::IS_EQUAL_EPISILON);
}

FORCEINLINE bool CVector4::HasNan() const noexcept
{
    for ( int i = 0; i < 4; i++ )
    {
        if ( isnan( reinterpret_cast<const float*>(this)[i] ) )
        {
            return true;
        }
    }

    return false;
}

FORCEINLINE bool CVector4::HasInfinity() const noexcept
{
    for ( int i = 0; i < 4; i++ )
    {
        if ( isinf( reinterpret_cast<const float*>(this)[i] ) )
        {
            return true;
        }
    }

    return false;
}

FORCEINLINE bool CVector4::IsValid() const noexcept
{
    return !HasNan() && !HasInfinity();
}

NOINLINE float CVector4::Length() const noexcept
{
    float fLengthSquared = LengthSquared();
    return NMath::Sqrt( fLengthSquared );
}

FORCEINLINE float CVector4::LengthSquared() const noexcept
{
    return DotProduct( *this );
}

FORCEINLINE float CVector4::DotProduct( const CVector4& Other ) const noexcept
{
#if defined(DISABLE_SIMD)

    return (x * Other.x) + (y * Other.y) + (z * Other.z) + (w * Other.w);

#else

    NSIMD::Float128 Reg0 = NSIMD::LoadAligned( this );
    NSIMD::Float128 Reg1 = NSIMD::LoadAligned( &Other );
    NSIMD::Float128 Dot = NSIMD::Dot( Reg0, Reg1 );
    return NSIMD::GetX( Dot );

#endif
}

FORCEINLINE CVector4 CVector4::CrossProduct( const CVector4& Other ) const noexcept
{
#if defined(DISABLE_SIMD)

    float NewX = (y * Other.z) - (z * Other.y);
    float NewY = (z * Other.x) - (x * Other.z);
    float NewZ = (x * Other.y) - (y * Other.x);
    return CVector4( NewX, NewY, NewZ, 0.0f );

#else

    CVector4 Cross;

    NSIMD::Float128 Reg0 = NSIMD::LoadAligned( this );
    NSIMD::Float128 Reg1 = NSIMD::LoadAligned( &Other );
    NSIMD::Float128 Result = NSIMD::Cross( Reg0, Reg1 );
    NSIMD::Int128   Mask = NSIMD::Load( ~0, ~0, ~0, 0 );
    Result = NSIMD::And( Result, NSIMD::CastIntToFloat( Mask ) );

    NSIMD::StoreAligned( Result, &Cross );
    return Cross;

#endif
}

inline CVector4 CVector4::ProjectOn( const CVector4& Other ) const noexcept
{
#if defined(DISABLE_SIMD)

    float AdotB = this->DotProduct( Other );
    float BdotB = Other.LengthSquared();
    return (AdotB / BdotB) * Other;

#else

    CVector4 Projected;

    NSIMD::Float128 Reg0 = NSIMD::LoadAligned( this );
    NSIMD::Float128 Reg1 = NSIMD::LoadAligned( &Other );

    NSIMD::Float128 AdotB = NSIMD::Dot( Reg0, Reg1 );
    AdotB = NSIMD::Shuffle<0, 1, 0, 1>( AdotB );

    NSIMD::Float128 BdotB = NSIMD::Dot( Reg1, Reg1 );
    BdotB = NSIMD::Shuffle<0, 1, 0, 1>( BdotB );
    BdotB = NSIMD::Div( AdotB, BdotB );
    BdotB = NSIMD::Mul( BdotB, Reg1 );

    NSIMD::StoreAligned( BdotB, &Projected );
    return Projected;

#endif
}

inline CVector4 CVector4::Reflect( const CVector4& Normal ) const noexcept
{
#if defined(DISABLE_SIMD)

    float VdotN = this->DotProduct( Normal );
    float NdotN = Normal.LengthSquared();
    return *this - ((2.0f * (VdotN / NdotN)) * Normal);

#else

    CVector4 Reflected;

    NSIMD::Float128 Reg0 = NSIMD::LoadAligned( this );
    NSIMD::Float128 Reg1 = NSIMD::LoadAligned( reinterpret_cast<const float*>(&Normal) );

    NSIMD::Float128 VdotN = NSIMD::Dot( Reg0, Reg1 );
    VdotN = NSIMD::Shuffle<0, 1, 0, 1>( VdotN );

    NSIMD::Float128 NdotN = NSIMD::Dot( Reg1, Reg1 );
    NdotN = NSIMD::Shuffle<0, 1, 0, 1>( NdotN );

    NSIMD::Float128 Reg2 = NSIMD::Load( 2.0f );
    VdotN = NSIMD::Div( VdotN, NdotN );
    VdotN = NSIMD::Mul( Reg2, VdotN );
    Reg1 = NSIMD::Mul( VdotN, Reg1 );
    Reg0 = NSIMD::Sub( Reg0, Reg1 );

    NSIMD::StoreAligned( Reg0, &Reflected );
    return Reflected;

#endif
}

FORCEINLINE float* CVector4::GetData() noexcept
{
    return reinterpret_cast<float*>(this);
}

FORCEINLINE const float* CVector4::GetData() const noexcept
{
    return reinterpret_cast<const float*>(this);
}

FORCEINLINE CVector4 Min( const CVector4& LHS, const CVector4& RHS ) noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4( fminf( LHS.x, RHS.x ), fminf( LHS.y, RHS.y ), fminf( LHS.z, RHS.z ), fminf( LHS.w, RHS.w ) );

#else

    CVector4 MinVector;

    NSIMD::Float128 Reg0 = NSIMD::LoadAligned( reinterpret_cast<const float*>(&LHS) );
    NSIMD::Float128 Reg1 = NSIMD::LoadAligned( &RHS );
    Reg0 = NSIMD::Min( Reg0, Reg1 );

    NSIMD::StoreAligned( Reg0, &MinVector );
    return MinVector;

#endif
}

FORCEINLINE CVector4 Max( const CVector4& LHS, const CVector4& RHS ) noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4( fmaxf( LHS.x, RHS.x ), fmaxf( LHS.y, RHS.y ), fmaxf( LHS.z, RHS.z ), fmaxf( LHS.w, RHS.w ) );

#else

    CVector4 MaxVector;

    NSIMD::Float128 Reg0 = NSIMD::LoadAligned( reinterpret_cast<const float*>(&LHS) );
    NSIMD::Float128 Reg1 = NSIMD::LoadAligned( &RHS );
    Reg0 = NSIMD::Max( Reg0, Reg1 );

    NSIMD::StoreAligned( Reg0, &MaxVector );
    return MaxVector;

#endif
}

FORCEINLINE CVector4 Lerp( const CVector4& First, const CVector4& Second, float t ) noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(
        (1.0f - t) * First.x + t * Second.x,
        (1.0f - t) * First.y + t * Second.y,
        (1.0f - t) * First.z + t * Second.z,
        (1.0f - t) * First.w + t * Second.w );

#else

    CVector4 Lerped;

    NSIMD::Float128 Reg0 = NSIMD::LoadAligned( reinterpret_cast<const float*>(&First) );
    NSIMD::Float128 Reg1 = NSIMD::LoadAligned( reinterpret_cast<const float*>(&Second) );
    NSIMD::Float128 Reg3 = NSIMD::Load( t );
    NSIMD::Float128 Ones = NSIMD::MakeOnes();
    NSIMD::Float128 Reg4 = NSIMD::Sub( Ones, Reg3 );
    Reg0 = NSIMD::Mul( Reg0, Reg4 );
    Reg1 = NSIMD::Mul( Reg1, Reg3 );
    Reg0 = NSIMD::Add( Reg0, Reg1 );

    NSIMD::StoreAligned( Reg0, &Lerped );
    return Lerped;

#endif
}

FORCEINLINE CVector4 Clamp( const CVector4& Min, const CVector4& Max, const CVector4& Value ) noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(
        fminf( fmaxf( Value.x, Min.x ), Max.x ),
        fminf( fmaxf( Value.y, Min.y ), Max.y ),
        fminf( fmaxf( Value.z, Min.z ), Max.z ),
        fminf( fmaxf( Value.w, Min.w ), Max.w ) );

#else

    CVector4 Clamped;

    NSIMD::Float128 Min128 = NSIMD::LoadAligned( reinterpret_cast<const float*>(&Min) );
    NSIMD::Float128 Max128 = NSIMD::LoadAligned( reinterpret_cast<const float*>(&Max) );
    NSIMD::Float128 Value128 = NSIMD::LoadAligned( &Value );
    Value128 = NSIMD::Max( Value128, Min128 );
    Value128 = NSIMD::Min( Value128, Max128 );

    NSIMD::StoreAligned( Value128, &Clamped );
    return Clamped;

#endif
}

FORCEINLINE CVector4 Saturate( const CVector4& Value ) noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(
        fminf( fmaxf( Value.x, 0.0f ), 1.0f ),
        fminf( fmaxf( Value.y, 0.0f ), 1.0f ),
        fminf( fmaxf( Value.z, 0.0f ), 1.0f ),
        fminf( fmaxf( Value.w, 0.0f ), 1.0f ) );

#else

    CVector4 Saturated;

    NSIMD::Float128 Zeros = NSIMD::MakeZeros();
    NSIMD::Float128 Ones = NSIMD::MakeOnes();
    NSIMD::Float128 VectorValue = NSIMD::LoadAligned( &Value );
    VectorValue = NSIMD::Max( VectorValue, Zeros );
    VectorValue = NSIMD::Min( VectorValue, Ones );

    NSIMD::StoreAligned( VectorValue, &Saturated );
    return Saturated;

#endif
}

FORCEINLINE CVector4 CVector4::operator-() const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4( -x, -y, -z, -w );

#else

    CVector4 Negated;

    NSIMD::Float128 Zeros = NSIMD::MakeZeros();
    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    This = NSIMD::Sub( Zeros, This );

    NSIMD::StoreAligned( This, &Negated );
    return Negated;

#endif
}

FORCEINLINE CVector4 CVector4::operator+( const CVector4& RHS ) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4( x + RHS.x, y + RHS.y, z + RHS.z, w + RHS.w );

#else

    CVector4 Result;

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Other = NSIMD::LoadAligned( &RHS );
    This = NSIMD::Add( This, Other );

    NSIMD::StoreAligned( This, &Result );
    return Result;

#endif
}

FORCEINLINE CVector4& CVector4::operator+=( const CVector4& RHS ) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this + RHS;

#else

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Other = NSIMD::LoadAligned( &RHS );
    This = NSIMD::Add( This, Other );

    NSIMD::StoreAligned( This, this );
    return *this;

#endif
}

FORCEINLINE CVector4 CVector4::operator+( float RHS ) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4( x + RHS, y + RHS, z + RHS, w + RHS );

#else

    CVector4 Result;

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Scalars = NSIMD::Load( RHS );
    This = NSIMD::Add( This, Scalars );

    NSIMD::StoreAligned( This, &Result );
    return Result;

#endif
}

FORCEINLINE CVector4& CVector4::operator+=( float RHS ) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this + RHS;

#else

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Scalars = NSIMD::Load( RHS );
    This = NSIMD::Add( This, Scalars );

    NSIMD::StoreAligned( This, this );
    return *this;

#endif
}

FORCEINLINE CVector4 CVector4::operator-( const CVector4& RHS ) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4( x - RHS.x, y - RHS.y, z - RHS.z, w - RHS.w );

#else

    CVector4 Result;

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Other = NSIMD::LoadAligned( &RHS );
    This = NSIMD::Sub( This, Other );

    NSIMD::StoreAligned( This, &Result );
    return Result;

#endif
}

FORCEINLINE CVector4& CVector4::operator-=( const CVector4& RHS ) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this - RHS;

#else

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Other = NSIMD::LoadAligned( &RHS );
    This = NSIMD::Sub( This, Other );

    NSIMD::StoreAligned( This, this );
    return *this;

#endif
}

FORCEINLINE CVector4 CVector4::operator-( float RHS ) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4( x - RHS, y - RHS, z - RHS, w - RHS );

#else

    CVector4 Result;

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Scalars = NSIMD::Load( RHS );
    This = NSIMD::Sub( This, Scalars );

    NSIMD::StoreAligned( This, &Result );
    return Result;

#endif
}

FORCEINLINE CVector4& CVector4::operator-=( float RHS ) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this - RHS;

#else

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Scalars = NSIMD::Load( RHS );
    This = NSIMD::Sub( This, Scalars );

    NSIMD::StoreAligned( This, this );
    return *this;

#endif
}

FORCEINLINE CVector4 CVector4::operator*( const CVector4& RHS ) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4( x * RHS.x, y * RHS.y, z * RHS.z, w * RHS.w );

#else

    CVector4 Result;

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Other = NSIMD::LoadAligned( &RHS );
    This = NSIMD::Mul( This, Other );

    NSIMD::StoreAligned( This, &Result );
    return Result;

#endif
}

FORCEINLINE CVector4& CVector4::operator*=( const CVector4& RHS ) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this * RHS;

#else

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Other = NSIMD::LoadAligned( &RHS );
    This = NSIMD::Mul( This, Other );

    NSIMD::StoreAligned( This, this );
    return *this;

#endif
}

FORCEINLINE CVector4 CVector4::operator*( float RHS ) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4( x * RHS, y * RHS, z * RHS, w * RHS );

#else

    CVector4 Result;

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Scalars = NSIMD::Load( RHS );
    This = NSIMD::Mul( This, Scalars );

    NSIMD::StoreAligned( This, &Result );
    return Result;

#endif
}

FORCEINLINE CVector4 operator*( float LHS, const CVector4& RHS ) noexcept
{
    return RHS * LHS;
}

FORCEINLINE CVector4 CVector4::operator*=( float RHS ) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this * RHS;

#else

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Scalars = NSIMD::Load( RHS );
    This = NSIMD::Mul( This, Scalars );

    NSIMD::StoreAligned( This, this );
    return *this;

#endif
}

FORCEINLINE CVector4 CVector4::operator/( const CVector4& RHS ) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4( x / RHS.x, y / RHS.y, z / RHS.z, w / RHS.w );

#else

    CVector4 Result;

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Other = NSIMD::LoadAligned( &RHS );
    This = NSIMD::Div( This, Other );

    NSIMD::StoreAligned( This, &Result );
    return Result;

#endif
}

FORCEINLINE CVector4& CVector4::operator/=( const CVector4& RHS ) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this / RHS;

#else

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Other = NSIMD::LoadAligned( &RHS );
    This = NSIMD::Div( This, Other );

    NSIMD::StoreAligned( This, this );
    return *this;

#endif
}

FORCEINLINE CVector4 CVector4::operator/( float RHS ) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4( x / RHS, y / RHS, z / RHS, w / RHS );

#else

    CVector4 Result;

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Scalars = NSIMD::Load( RHS );
    This = NSIMD::Div( This, Scalars );

    NSIMD::StoreAligned( This, &Result );
    return Result;

#endif
}

FORCEINLINE CVector4& CVector4::operator/=( float RHS ) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this / RHS;

#else

    NSIMD::Float128 This = NSIMD::LoadAligned( this );
    NSIMD::Float128 Scalars = NSIMD::Load( RHS );
    This = NSIMD::Div( This, Scalars );

    NSIMD::StoreAligned( This, this );
    return *this;

#endif
}

FORCEINLINE float& CVector4::operator[]( int Index ) noexcept
{
    Assert( Index < 4 );
    return reinterpret_cast<float*>(this)[Index];
}

FORCEINLINE float CVector4::operator[]( int Index ) const noexcept
{
    Assert( Index < 4 );
    return reinterpret_cast<const float*>(this)[Index];
}

FORCEINLINE bool CVector4::operator==( const CVector4& Other ) const noexcept
{
    return IsEqual( Other );
}

FORCEINLINE bool CVector4::operator!=( const CVector4& Other ) const noexcept
{
    return !IsEqual( Other );
}
