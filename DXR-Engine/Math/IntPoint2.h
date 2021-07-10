#pragma once
#include "MathCommon.h"

/* A 2-D vector (x, y) using integers */
class CIntPoint2
{
public:
    /* Default constructor (Initialize components to zero) */
    FORCEINLINE CIntPoint2() noexcept;

    /**
     * Constructor initializing all components with a corresponding value.
     *
     * @param InX: The x-coordinate
     * @param InY: The y-coordinate
     */
    FORCEINLINE explicit CIntPoint2( int InX, int InY ) noexcept;

    /**
     * Constructor initializing all components with an array.
     *
     * @param Arr: Array with 2 elements
     */
    FORCEINLINE explicit CIntPoint2( const int* Arr ) noexcept;

    /**
     * Constructor initializing all components with a single value.
     *
     * @param Scalar: Value to set all components to
     */
    FORCEINLINE explicit CIntPoint2( int Scalar ) noexcept;

    /**
     * Compares this vector with another vector
     *
     * @param Other: Vector to compare against
     * @return True if equal, false if not
     */
    FORCEINLINE bool IsEqual( const CIntPoint2& Other ) const noexcept;

public:
    /**
     * Returns a vector with the smallest of each component of two vectors
     *
     * @param LHS: First vector to compare with
     * @param RHS: Second vector to compare with
     * @return A vector with the smallest components of LHS and RHS
     */
    friend FORCEINLINE CIntPoint2 Min( const CIntPoint2& LHS, const CIntPoint2& RHS ) noexcept;

    /**
     * Returns a vector with the largest of each component of two vectors
     *
     * @param LHS: First vector to compare with
     * @param RHS: Second vector to compare with
     * @return A vector with the largest components of LHS and RHS
     */
    friend FORCEINLINE CIntPoint2 Max( const CIntPoint2& LHS, const CIntPoint2& RHS ) noexcept;

public:
    /**
     * Return a vector with component-wise negation of this vector
     *
     * @return A negated vector
     */
    FORCEINLINE CIntPoint2 operator-() const noexcept;

    /**
     * Returns the result of component-wise adding this and another vector
     *
     * @param RHS: The vector to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CIntPoint2 operator+( const CIntPoint2& RHS ) const noexcept;

    /**
     * Returns this vector after component-wise adding this with another vector
     *
     * @param RHS: The vector to add
     * @return A reference to this vector
     */
    FORCEINLINE CIntPoint2& operator+=( const CIntPoint2& RHS ) noexcept;

    /**
     * Returns the result of adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CIntPoint2 operator+( int RHS ) const noexcept;

    /**
     * Returns this vector after adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A reference to this vector
     */
    FORCEINLINE CIntPoint2& operator+=( int RHS ) noexcept;

    /**
     * Returns the result of component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A vector with the result of subtraction
     */
    FORCEINLINE CIntPoint2 operator-( const CIntPoint2& RHS ) const noexcept;

    /**
     * Returns this vector after component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CIntPoint2& operator-=( const CIntPoint2& RHS ) noexcept;

    /**
     * Returns the result of subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A vector with the result of the subtraction
     */
    FORCEINLINE CIntPoint2 operator-( int RHS ) const noexcept;

    /**
     * Returns this vector after subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CIntPoint2& operator-=( int RHS ) noexcept;

    /**
     * Returns the result of component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CIntPoint2 operator*( const CIntPoint2& RHS ) const noexcept;

    /**
     * Returns this vector after component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CIntPoint2& operator*=( const CIntPoint2& RHS ) noexcept;

    /**
     * Returns the result of multipling each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CIntPoint2 operator*( int RHS ) const noexcept;

    /**
     * Returns this vector after multipling each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CIntPoint2& operator*=( int RHS ) noexcept;

    /**
     * Returns the result of component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CIntPoint2 operator/( const CIntPoint2& RHS ) const noexcept;

    /**
     * Returns this vector after component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CIntPoint2& operator/=( const CIntPoint2& RHS ) noexcept;

    /**
     * Returns the result of dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CIntPoint2 operator/( int RHS ) const noexcept;

    /**
     * Returns this vector after dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CIntPoint2& operator/=( int RHS ) noexcept;

    /**
     * Returns the result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==( const CIntPoint2& Other ) const noexcept;

    /**
     * Returns the negated result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=( const CIntPoint2& Other ) const noexcept;

    /**
     * Returns the component specifed
     *
     * @param Index: The component index
     * @return The component
     */
    FORCEINLINE int& operator[]( int Index ) noexcept;

    /**
     * Returns the component specifed
     *
     * @param Index: The component index
     * @return The component
     */
    FORCEINLINE int operator[]( int Index ) const noexcept;

public:
    /* The x-coordinate */
    int x;

    /* The y-coordinate */
    int y;
};

FORCEINLINE CIntPoint2::CIntPoint2() noexcept
    : x( 0 )
    , y( 0 )
{
}

FORCEINLINE CIntPoint2::CIntPoint2( int InX, int InY ) noexcept
    : x( InX )
    , y( InY )
{
}

FORCEINLINE CIntPoint2::CIntPoint2( const int* Arr ) noexcept
    : x( Arr[0] )
    , y( Arr[1] )
{
}

FORCEINLINE CIntPoint2::CIntPoint2( int Scalar ) noexcept
    : x( Scalar )
    , y( Scalar )
{
}

FORCEINLINE bool CIntPoint2::IsEqual( const CIntPoint2& Other ) const noexcept
{
    return x == Other.x && y == Other.y;
}

FORCEINLINE CIntPoint2 CIntPoint2::operator-() const noexcept
{
    return CIntPoint2( -x, -y );
}

FORCEINLINE CIntPoint2 CIntPoint2::operator+( const CIntPoint2& RHS ) const noexcept
{
    return CIntPoint2( x + RHS.x, y + RHS.y );
}

FORCEINLINE CIntPoint2& CIntPoint2::operator+=( const CIntPoint2& RHS ) noexcept
{
    return *this = *this + RHS;
}

FORCEINLINE CIntPoint2 CIntPoint2::operator+( int RHS ) const noexcept
{
    return CIntPoint2( x + RHS, y + RHS );
}

FORCEINLINE CIntPoint2& CIntPoint2::operator+=( int RHS ) noexcept
{
    return *this = *this + RHS;
}

FORCEINLINE CIntPoint2 CIntPoint2::operator-( const CIntPoint2& RHS ) const noexcept
{
    return CIntPoint2( x - RHS.x, y - RHS.y );
}

FORCEINLINE CIntPoint2& CIntPoint2::operator-=( const CIntPoint2& RHS ) noexcept
{
    return *this = *this - RHS;
}

FORCEINLINE CIntPoint2 CIntPoint2::operator-( int RHS ) const noexcept
{
    return CIntPoint2( x - RHS, y - RHS );
}

FORCEINLINE CIntPoint2& CIntPoint2::operator-=( int RHS ) noexcept
{
    return *this = *this - RHS;
}

FORCEINLINE CIntPoint2 CIntPoint2::operator*( const CIntPoint2& RHS ) const noexcept
{
    return CIntPoint2( x * RHS.x, y * RHS.y );
}

FORCEINLINE CIntPoint2& CIntPoint2::operator*=( const CIntPoint2& RHS ) noexcept
{
    return *this = *this * RHS;
}

FORCEINLINE CIntPoint2 CIntPoint2::operator*( int RHS ) const noexcept
{
    return CIntPoint2( x * RHS, y * RHS );
}

FORCEINLINE CIntPoint2& CIntPoint2::operator*=( int RHS ) noexcept
{
    return *this = *this * RHS;
}

FORCEINLINE CIntPoint2 CIntPoint2::operator/( const CIntPoint2& RHS ) const noexcept
{
    return CIntPoint2( x / RHS.x, y / RHS.y );
}

FORCEINLINE CIntPoint2& CIntPoint2::operator/=( const CIntPoint2& RHS ) noexcept
{
    return *this = *this / RHS;
}

FORCEINLINE CIntPoint2 CIntPoint2::operator/( int RHS ) const noexcept
{
    return CIntPoint2( x / RHS, y / RHS );
}

FORCEINLINE CIntPoint2& CIntPoint2::operator/=( int RHS ) noexcept
{
    return *this = *this / RHS;
}

FORCEINLINE bool CIntPoint2::operator==( const CIntPoint2& Other ) const noexcept
{
    return IsEqual( Other );
}

FORCEINLINE bool CIntPoint2::operator!=( const CIntPoint2& Other ) const noexcept
{
    return !IsEqual( Other );
}

FORCEINLINE int& CIntPoint2::operator[]( int Index ) noexcept
{
    ASSERT( Index < 2 );
    return reinterpret_cast<int*>(this)[Index];
}

FORCEINLINE int CIntPoint2::operator[]( int Index ) const noexcept
{
    ASSERT( Index < 2 );
    return reinterpret_cast<const int*>(this)[Index];
}

FORCEINLINE CIntPoint2 Min( const CIntPoint2& LHS, const CIntPoint2& RHS ) noexcept
{
    return CIntPoint2( std::min( LHS.x, RHS.x ), std::min( LHS.y, RHS.y ) );
}

FORCEINLINE CIntPoint2 Max( const CIntPoint2& LHS, const CIntPoint2& RHS ) noexcept
{
    return CIntPoint2( std::max( LHS.x, RHS.x ), std::max( LHS.y, RHS.y ) );
}
