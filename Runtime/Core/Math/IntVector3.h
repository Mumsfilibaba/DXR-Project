#pragma once
#include "MathCommon.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// A 3-D vector (x, y, z) using integers

class CIntVector3
{
public:

    /* Default constructor (Initial1ze components to zero) */
    FORCEINLINE CIntVector3() noexcept;

    /**
     * Constructor initializing all components with a corresponding value.
     *
     * @param InX: The x-coordinate
     * @param InY: The y-coordinate
     * @param InZ: The z-coordinate
     */
    FORCEINLINE explicit CIntVector3(int InX, int InY, int InZ) noexcept;

    /**
     * Constructor initializing all components with an array.
     *
     * @param Arr: Array with 3 elements
     */
    FORCEINLINE explicit CIntVector3(const int* Arr) noexcept;

    /**
     * Constructor initializing all components with a single value.
     *
     * @param Scalar: Value to set all components to
     */
    FORCEINLINE explicit CIntVector3(int Scalar) noexcept;

    /**
     * Compares this vector with another vector
     *
     * @param Other: Vector to compare against
     * @return True if equal, false if not
     */
    FORCEINLINE bool IsEqual(const CIntVector3& Other) const noexcept;

public:
    /**
     * Returns a vector with the smallest of each component of two vectors
     *
     * @param LHS: First vector to compare with
     * @param RHS: Second vector to compare with
     * @return A vector with the smallest components of LHS and RHS
     */
    friend FORCEINLINE CIntVector3 Min(const CIntVector3& LHS, const CIntVector3& RHS) noexcept;

    /**
     * Returns a vector with the largest of each component of two vectors
     *
     * @param LHS: First vector to compare with
     * @param RHS: Second vector to compare with
     * @return A vector with the largest components of LHS and RHS
     */
    friend FORCEINLINE CIntVector3 Max(const CIntVector3& LHS, const CIntVector3& RHS) noexcept;

public:
    /**
     * Return a vector with component-wise negation of this vector
     *
     * @return A negated vector
     */
    FORCEINLINE CIntVector3 operator-() const noexcept;

    /**
     * Returns the result of component-wise adding this and another vector
     *
     * @param RHS: The vector to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CIntVector3 operator+(const CIntVector3& RHS) const noexcept;

    /**
     * Returns this vector after component-wise adding this with another vector
     *
     * @param RHS: The vector to add
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator+=(const CIntVector3& RHS) noexcept;

    /**
     * Returns the result of adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CIntVector3 operator+(int RHS) const noexcept;

    /**
     * Returns this vector after adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator+=(int RHS) noexcept;

    /**
     * Returns the result of component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A vector with the result of subtraction
     */
    FORCEINLINE CIntVector3 operator-(const CIntVector3& RHS) const noexcept;

    /**
     * Returns this vector after component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator-=(const CIntVector3& RHS) noexcept;

    /**
     * Returns the result of subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A vector with the result of the subtraction
     */
    FORCEINLINE CIntVector3 operator-(int RHS) const noexcept;

    /**
     * Returns this vector after subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator-=(int RHS) noexcept;

    /**
     * Returns the result of component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CIntVector3 operator*(const CIntVector3& RHS) const noexcept;

    /**
     * Returns this vector after component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator*=(const CIntVector3& RHS) noexcept;

    /**
     * Returns the result of multipling each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CIntVector3 operator*(int RHS) const noexcept;

    /**
     * Returns this vector after multiplying each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator*=(int RHS) noexcept;

    /**
     * Returns the result of component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CIntVector3 operator/(const CIntVector3& RHS) const noexcept;

    /**
     * Returns this vector after component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator/=(const CIntVector3& RHS) noexcept;

    /**
     * Returns the result of dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CIntVector3 operator/(int RHS) const noexcept;

    /**
     * Returns this vector after dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator/=(int RHS) noexcept;

    /**
     * Returns the result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==(const CIntVector3& Other) const noexcept;

    /**
     * Returns the negated result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=(const CIntVector3& Other) const noexcept;

    /**
     * Returns the component specified
     *
     * @param Index: The component index
     * @return The component
     */
    FORCEINLINE int& operator[](int Index) noexcept;

    /**
     * Returns the component specified
     *
     * @param Index: The component index
     * @return The component
     */
    FORCEINLINE int operator[](int Index) const noexcept;

public:

    /* The x-coordinate */
    int x;
    /* The y-coordinate */
    int y;
    /* The z-coordinate */
    int z;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Implementation

FORCEINLINE CIntVector3::CIntVector3() noexcept
    : x(0)
    , y(0)
    , z(0)
{
}

FORCEINLINE CIntVector3::CIntVector3(int InX, int InY, int InZ) noexcept
    : x(InX)
    , y(InY)
    , z(InZ)
{
}

FORCEINLINE CIntVector3::CIntVector3(const int* Arr) noexcept
    : x(Arr[0])
    , y(Arr[1])
    , z(Arr[2])
{
}

FORCEINLINE CIntVector3::CIntVector3(int Scalar) noexcept
    : x(Scalar)
    , y(Scalar)
    , z(Scalar)
{
}

FORCEINLINE bool CIntVector3::IsEqual(const CIntVector3& Other) const noexcept
{
    return (x == Other.x) && (y == Other.y) && (z == Other.z);
}

FORCEINLINE CIntVector3 CIntVector3::operator-() const noexcept
{
    return CIntVector3(-x, -y, -z);
}

FORCEINLINE CIntVector3 CIntVector3::operator+(const CIntVector3& RHS) const noexcept
{
    return CIntVector3(x + RHS.x, y + RHS.y, z + RHS.z);
}

FORCEINLINE CIntVector3& CIntVector3::operator+=(const CIntVector3& RHS) noexcept
{
    return *this = *this + RHS;
}

FORCEINLINE CIntVector3 CIntVector3::operator+(int RHS) const noexcept
{
    return CIntVector3(x + RHS, y + RHS, z + RHS);
}

FORCEINLINE CIntVector3& CIntVector3::operator+=(int RHS) noexcept
{
    return *this = *this + RHS;
}

FORCEINLINE CIntVector3 CIntVector3::operator-(const CIntVector3& RHS) const noexcept
{
    return CIntVector3(x - RHS.x, y - RHS.y, z - RHS.z);
}

FORCEINLINE CIntVector3& CIntVector3::operator-=(const CIntVector3& RHS) noexcept
{
    return *this = *this - RHS;
}

FORCEINLINE CIntVector3 CIntVector3::operator-(int RHS) const noexcept
{
    return CIntVector3(x - RHS, y - RHS, z - RHS);
}

FORCEINLINE CIntVector3& CIntVector3::operator-=(int RHS) noexcept
{
    return *this = *this - RHS;
}

FORCEINLINE CIntVector3 CIntVector3::operator*(const CIntVector3& RHS) const noexcept
{
    return CIntVector3(x * RHS.x, y * RHS.y, z * RHS.z);
}

FORCEINLINE CIntVector3& CIntVector3::operator*=(const CIntVector3& RHS) noexcept
{
    return *this = *this * RHS;
}

FORCEINLINE CIntVector3 CIntVector3::operator*(int RHS) const noexcept
{
    return CIntVector3(x * RHS, y * RHS, z * RHS);
}

FORCEINLINE CIntVector3& CIntVector3::operator*=(int RHS) noexcept
{
    return *this = *this * RHS;
}

FORCEINLINE CIntVector3 CIntVector3::operator/(const CIntVector3& RHS) const noexcept
{
    return CIntVector3(x / RHS.x, y / RHS.y, z / RHS.z);
}

FORCEINLINE CIntVector3& CIntVector3::operator/=(const CIntVector3& RHS) noexcept
{
    return *this = *this / RHS;
}

FORCEINLINE CIntVector3 CIntVector3::operator/(int RHS) const noexcept
{
    return CIntVector3(x / RHS, y / RHS, z / RHS);
}

FORCEINLINE CIntVector3& CIntVector3::operator/=(int RHS) noexcept
{
    return *this = *this / RHS;
}

FORCEINLINE int& CIntVector3::operator[](int Index) noexcept
{
    Assert(Index < 3);
    return reinterpret_cast<int*>(this)[Index];
}

FORCEINLINE int CIntVector3::operator[](int Index) const noexcept
{
    Assert(Index < 3);
    return reinterpret_cast<const int*>(this)[Index];
}

FORCEINLINE bool CIntVector3::operator==(const CIntVector3& Other) const noexcept
{
    return IsEqual(Other);
}

FORCEINLINE bool CIntVector3::operator!=(const CIntVector3& Other) const noexcept
{
    return !IsEqual(Other);
}

FORCEINLINE CIntVector3 Min(const CIntVector3& LHS, const CIntVector3& RHS) noexcept
{
    return CIntVector3(NMath::Min(LHS.x, RHS.x), NMath::Min(LHS.y, RHS.y), NMath::Min(LHS.z, RHS.z));
}

FORCEINLINE CIntVector3 Max(const CIntVector3& LHS, const CIntVector3& RHS) noexcept
{
    return CIntVector3(NMath::Max(LHS.x, RHS.x), NMath::Max(LHS.y, RHS.y), NMath::Max(LHS.z, RHS.z));
}
