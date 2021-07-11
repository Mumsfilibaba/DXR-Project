#pragma once
#include "MathCommon.h"

/* A 2-D floating point vector (x, y) */
class CVector2
{
public:
    /* Default constructor (Initialize components to zero) */
    FORCEINLINE CVector2() noexcept;

    /**
     * Constructor initializing all components with a corresponding value.
     *
     * @param InX: The x-coordinate
     * @param InY: The y-coordinate
     */
    FORCEINLINE explicit CVector2( float InX, float InY ) noexcept;

    /**
     * Constructor initializing all components with an array.
     *
     * @param Arr: Array with 2 elements
     */
    FORCEINLINE explicit CVector2( const float* Arr ) noexcept;

    /**
     * Constructor initializing all components with a single value.
     *
     * @param Scalar: Value to set all components to
     */
    FORCEINLINE explicit CVector2( float Scalar ) noexcept;

    /* Normalized this vector */
    inline void Normalize() noexcept;

    /**
     * Returns a normalized version of this vector
     *
     * @return A copy of this vector normalized
     */
    FORCEINLINE CVector2 GetNormalized() const noexcept;

    /**
     * Compares, within a threshold Epsilon, this vector with another vector
     *
     * @param Other: vector to compare against
     * @return True if equal, false if not
     */
    inline bool IsEqual( const CVector2& Other, float Epsilon = NMath::IS_EQUAL_EPISILON ) const noexcept;

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
    FORCEINLINE float DotProduct( const CVector2& Other ) const noexcept;

    /**
     * Returns the resulting vector after projecting this vector onto another.
     *
     * @param Other: The vector to project onto
     * @return The projected vector
     */
    inline CVector2 ProjectOn( const CVector2& Other ) const noexcept;

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
    friend FORCEINLINE CVector2 Min( const CVector2& First, const CVector2& Second ) noexcept;

    /**
     * Returns a vector with the largest of each component of two vectors
     *
     * @param First: First vector to compare with
     * @param Second: Second vector to compare with
     * @return A vector with the largest components of First and Second
     */
    friend FORCEINLINE CVector2 Max( const CVector2& First, const CVector2& Second ) noexcept;

    /**
     * Returns the linear interpolation between two vectors
     *
     * @param First: First vector to interpolate
     * @param Second: Second vector to interpolate
     * @param Factor: Factor to interpolate with. Zero returns First, One returns seconds
     * @return A vector with the result of interpolation
     */
    friend FORCEINLINE CVector2 Lerp( const CVector2& First, const CVector2& Second, float t ) noexcept;

    /**
     * Returns a vector with all the components within the range of a min and max value
     *
     * @param Min: Vector with minimum values
     * @param Max: Vector with maximum values
     * @param Value: Vector to clamp
     * @return A vector with the result of clamping
     */
    friend FORCEINLINE CVector2 Clamp( const CVector2& Min, const CVector2& Max, const CVector2& Value ) noexcept;

    /**
     * Returns a vector with all the components within the range zero and one
     *
     * @param Value: Value to saturate
     * @return A vector with the result of saturation
     */
    friend FORCEINLINE CVector2 Saturate( const CVector2& Value ) noexcept;

public:
    /**
     * Return a vector with component-wise negation of this vector
     *
     * @return A negated vector
     */
    FORCEINLINE CVector2 operator-() const noexcept;

    /**
     * Returns the result of component-wise adding this and another vector
     *
     * @param RHS: The vector to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CVector2 operator+( const CVector2& RHS ) const noexcept;

    /**
     * Returns this vector after component-wise adding this with another vector
     *
     * @param RHS: The vector to add
     * @return A reference to this vector
     */
    FORCEINLINE CVector2& operator+=( const CVector2& RHS ) noexcept;

    /**
     * Returns the result of adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CVector2 operator+( float RHS ) const noexcept;

    /**
     * Returns this vector after adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A reference to this vector
     */
    FORCEINLINE CVector2& operator+=( float RHS ) noexcept;

    /**
     * Returns the result of component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A vector with the result of subtraction
     */
    FORCEINLINE CVector2 operator-( const CVector2& RHS ) const noexcept;

    /**
     * Returns this vector after component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CVector2& operator-=( const CVector2& RHS ) noexcept;

    /**
     * Returns the result of subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A vector with the result of the subtraction
     */
    FORCEINLINE CVector2 operator-( float RHS ) const noexcept;

    /**
     * Returns this vector after subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CVector2& operator-=( float RHS ) noexcept;

    /**
     * Returns the result of component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CVector2 operator*( const CVector2& RHS ) const noexcept;

    /**
     * Returns this vector after component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CVector2& operator*=( const CVector2& RHS ) noexcept;

    /**
     * Returns the result of multipling each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CVector2 operator*( float RHS ) const noexcept;

    /**
     * Returns the result of multipling each component of a vector with a scalar
     *
     * @param LHS: The scalar to multiply with
     * @param RHS: The vector to multiply with
     * @return A vector with the result of the multiplication
     */
    friend FORCEINLINE CVector2 operator*( float LHS, const CVector2& RHS ) noexcept;

    /**
     * Returns this vector after multipling each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CVector2 operator*=( float RHS ) noexcept;

    /**
     * Returns the result of component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CVector2 operator/( const CVector2& RHS ) const noexcept;

    /**
     * Returns this vector after component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CVector2& operator/=( const CVector2& RHS ) noexcept;

    /**
     * Returns the result of dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CVector2 operator/( float RHS ) const noexcept;

    /**
     * Returns this vector after dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CVector2& operator/=( float RHS ) noexcept;

    /**
     * Returns the result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==( const CVector2& Other ) const noexcept;

    /**
     * Returns the negated result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=( const CVector2& Other ) const noexcept;

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
};

FORCEINLINE CVector2::CVector2() noexcept
    : x( 0.0f )
    , y( 0.0f )
{
}

FORCEINLINE CVector2::CVector2( float InX, float InY ) noexcept
    : x( InX )
    , y( InY )
{
}

FORCEINLINE CVector2::CVector2( const float* Arr ) noexcept
    : x( Arr[0] )
    , y( Arr[1] )
{
}

FORCEINLINE CVector2::CVector2( float Scalar ) noexcept
    : x( Scalar )
    , y( Scalar )
{
}

inline void CVector2::Normalize() noexcept
{
    float fLength = Length();
    Assert( fLength != 0 );

    float fRecipLength = 1.0f / fLength;
    x = x * fRecipLength;
    y = y * fRecipLength;
}

FORCEINLINE CVector2 CVector2::GetNormalized() const noexcept
{
    CVector2 Result( *this );
    Result.Normalize();
    return Result;
}

FORCEINLINE bool CVector2::IsEqual( const CVector2& Other, float Epsilon ) const noexcept
{
    Epsilon = fabsf( Epsilon );

    for ( int i = 0; i < 2; i++ )
    {
        float Diff = reinterpret_cast<const float*>(this)[i] - reinterpret_cast<const float*>(&Other)[i];
        if ( fabsf( Diff ) > Epsilon )
        {
            return false;
        }
    }

    return true;
}

FORCEINLINE bool CVector2::IsUnitVector() const noexcept
{
    // LengthSquared should be the same as length if this is a unit vector
    // However, this way the sqrt can be avoided
    float fLengthSquared = fabsf( 1.0f - LengthSquared() );
    return (fLengthSquared < NMath::IS_EQUAL_EPISILON);
}

FORCEINLINE bool CVector2::HasNan() const noexcept
{
    for ( int i = 0; i < 2; i++ )
    {
        if ( isnan( reinterpret_cast<const float*>(this)[i] ) )
        {
            return true;
        }
    }

    return false;
}

FORCEINLINE bool CVector2::HasInfinity() const noexcept
{
    for ( int i = 0; i < 2; i++ )
    {
        if ( isinf( reinterpret_cast<const float*>(this)[i] ) )
        {
            return true;
        }
    }

    return false;
}

FORCEINLINE bool CVector2::IsValid() const noexcept
{
    return !HasNan() && !HasInfinity();
}

FORCEINLINE float CVector2::Length() const noexcept
{
    float fLengthSquared = LengthSquared();
    return NMath::Sqrt( fLengthSquared );
}

FORCEINLINE float CVector2::LengthSquared() const noexcept
{
    return DotProduct( *this );
}

FORCEINLINE float CVector2::DotProduct( const CVector2& Other ) const noexcept
{
    return (x * Other.x) + (y * Other.y);
}

inline CVector2 CVector2::ProjectOn( const CVector2& Other ) const noexcept
{
    float AdotB = this->DotProduct( Other );
    float BdotB = Other.LengthSquared();
    return (AdotB / BdotB) * Other;
}

FORCEINLINE float* CVector2::GetData() noexcept
{
    return reinterpret_cast<float*>(this);
}

FORCEINLINE const float* CVector2::GetData() const noexcept
{
    return reinterpret_cast<const float*>(this);
}

FORCEINLINE CVector2 Min( const CVector2& LHS, const CVector2& RHS ) noexcept
{
    return CVector2( fminf( LHS.x, RHS.x ), fminf( LHS.y, RHS.y ) );
}

FORCEINLINE CVector2 Max( const CVector2& LHS, const CVector2& RHS ) noexcept
{
    return CVector2( fmaxf( LHS.x, RHS.x ), fmaxf( LHS.y, RHS.y ) );
}

FORCEINLINE CVector2 Lerp( const CVector2& First, const CVector2& Second, float t ) noexcept
{
    return CVector2(
        (1.0f - t) * First.x + t * Second.x,
        (1.0f - t) * First.y + t * Second.y );
}

FORCEINLINE CVector2 Clamp( const CVector2& Min, const CVector2& Max, const CVector2& Value ) noexcept
{
    return CVector2(
        fminf( fmaxf( Value.x, Min.x ), Max.x ),
        fminf( fmaxf( Value.y, Min.y ), Max.y ) );
}

FORCEINLINE CVector2 Saturate( const CVector2& Value ) noexcept
{
    return CVector2(
        fminf( fmaxf( Value.x, 0.0f ), 1.0f ),
        fminf( fmaxf( Value.y, 0.0f ), 1.0f ) );
}

FORCEINLINE CVector2 CVector2::operator-() const noexcept
{
    return CVector2( -x, -y );
}

FORCEINLINE CVector2 CVector2::operator+( const CVector2& RHS ) const noexcept
{
    return CVector2( x + RHS.x, y + RHS.y );
}

FORCEINLINE CVector2& CVector2::operator+=( const CVector2& RHS ) noexcept
{
    return *this = *this + RHS;
}

FORCEINLINE CVector2 CVector2::operator+( float RHS ) const noexcept
{
    return CVector2( x + RHS, y + RHS );
}

FORCEINLINE CVector2& CVector2::operator+=( float RHS ) noexcept
{
    return *this = *this + RHS;
}

FORCEINLINE CVector2 CVector2::operator-( const CVector2& RHS ) const noexcept
{
    return CVector2( x - RHS.x, y - RHS.y );
}

FORCEINLINE CVector2& CVector2::operator-=( const CVector2& RHS ) noexcept
{
    return *this = *this - RHS;
}

FORCEINLINE CVector2 CVector2::operator-( float RHS ) const noexcept
{
    return CVector2( x - RHS, y - RHS );
}

FORCEINLINE CVector2& CVector2::operator-=( float RHS ) noexcept
{
    return *this = *this - RHS;
}

FORCEINLINE CVector2 CVector2::operator*( const CVector2& RHS ) const noexcept
{
    return CVector2( x * RHS.x, y * RHS.y );
}

FORCEINLINE CVector2& CVector2::operator*=( const CVector2& RHS ) noexcept
{
    return *this = *this * RHS;
}

FORCEINLINE CVector2 CVector2::operator*( float RHS ) const noexcept
{
    return CVector2( x * RHS, y * RHS );
}

FORCEINLINE CVector2 operator*( float LHS, const CVector2& RHS ) noexcept
{
    return CVector2( LHS * RHS.x, LHS * RHS.y );
}

FORCEINLINE CVector2 CVector2::operator*=( float RHS ) noexcept
{
    return *this = *this * RHS;
}

FORCEINLINE CVector2 CVector2::operator/( const CVector2& RHS ) const noexcept
{
    return CVector2( x / RHS.x, y / RHS.y );
}

FORCEINLINE CVector2& CVector2::operator/=( const CVector2& RHS ) noexcept
{
    return *this = *this / RHS;
}

FORCEINLINE CVector2 CVector2::operator/( float RHS ) const noexcept
{
    return CVector2( x / RHS, y / RHS );
}

FORCEINLINE CVector2& CVector2::operator/=( float RHS ) noexcept
{
    return *this = *this / RHS;
}

FORCEINLINE float& CVector2::operator[]( int Index ) noexcept
{
    Assert( Index < 2 );
    return reinterpret_cast<float*>(this)[Index];
}

FORCEINLINE float CVector2::operator[]( int Index ) const noexcept
{
    Assert( Index < 2 );
    return reinterpret_cast<const float*>(this)[Index];
}

FORCEINLINE bool CVector2::operator==( const CVector2& Other ) const noexcept
{
    return IsEqual( Other );
}

FORCEINLINE bool CVector2::operator!=( const CVector2& Other ) const noexcept
{
    return !IsEqual( Other );
}
