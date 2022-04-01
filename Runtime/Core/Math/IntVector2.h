#pragma once
#include "MathCommon.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// A 2-D vector (x, y) using integers

class CIntVector2
{
public:

    /** Default constructor (Initialize components to zero) */
    FORCEINLINE CIntVector2() noexcept;

    /**
     * @brief: Constructor initializing all components with a corresponding value.
     *
     * @param InX: The x-coordinate
     * @param InY: The y-coordinate
     */
    FORCEINLINE explicit CIntVector2(int InX, int InY) noexcept;

    /**
     * @brief: Constructor initializing all components with an array.
     *
     * @param Arr: Array with 2 elements
     */
    FORCEINLINE explicit CIntVector2(const int* Arr) noexcept;

    /**
     * @brief: Constructor initializing all components with a single value.
     *
     * @param Scalar: Value to set all components to
     */
    FORCEINLINE explicit CIntVector2(int Scalar) noexcept;

    /**
     * @brief: Compares this vector with another vector
     *
     * @param Other: Vector to compare against
     * @return True if equal, false if not
     */
    FORCEINLINE bool IsEqual(const CIntVector2& Other) const noexcept;

public:
    /**
     * @brief: Returns a vector with the smallest of each component of two vectors
     *
     * @param Lhs: First vector to compare with
     * @param Rhs: Second vector to compare with
     * @return A vector with the smallest components of Lhs and Rhs
     */
    friend FORCEINLINE CIntVector2 Min(const CIntVector2& Lhs, const CIntVector2& Rhs) noexcept;

    /**
     * @brief: Returns a vector with the largest of each component of two vectors
     *
     * @param Lhs: First vector to compare with
     * @param Rhs: Second vector to compare with
     * @return A vector with the largest components of Lhs and Rhs
     */
    friend FORCEINLINE CIntVector2 Max(const CIntVector2& Lhs, const CIntVector2& Rhs) noexcept;

public:
    /**
     * @brief: Return a vector with component-wise negation of this vector
     *
     * @return A negated vector
     */
    FORCEINLINE CIntVector2 operator-() const noexcept;

    /**
     * @brief: Returns the result of component-wise adding this and another vector
     *
     * @param Rhs: The vector to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CIntVector2 operator+(const CIntVector2& Rhs) const noexcept;

    /**
     * @brief: Returns this vector after component-wise adding this with another vector
     *
     * @param Rhs: The vector to add
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator+=(const CIntVector2& Rhs) noexcept;

    /**
     * @brief: Returns the result of adding a scalar to each component of this vector
     *
     * @param Rhs: The scalar to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CIntVector2 operator+(int Rhs) const noexcept;

    /**
     * @brief: Returns this vector after adding a scalar to each component of this vector
     *
     * @param Rhs: The scalar to add
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator+=(int Rhs) noexcept;

    /**
     * @brief: Returns the result of component-wise subtraction between this and another vector
     *
     * @param Rhs: The vector to subtract
     * @return A vector with the result of subtraction
     */
    FORCEINLINE CIntVector2 operator-(const CIntVector2& Rhs) const noexcept;

    /**
     * @brief: Returns this vector after component-wise subtraction between this and another vector
     *
     * @param Rhs: The vector to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator-=(const CIntVector2& Rhs) noexcept;

    /**
     * @brief: Returns the result of subtracting each component of this vector with a scalar
     *
     * @param Rhs: The scalar to subtract
     * @return A vector with the result of the subtraction
     */
    FORCEINLINE CIntVector2 operator-(int Rhs) const noexcept;

    /**
     * @brief: Returns this vector after subtracting each component of this vector with a scalar
     *
     * @param Rhs: The scalar to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator-=(int Rhs) noexcept;

    /**
     * @brief: Returns the result of component-wise multiplication with this and another vector
     *
     * @param Rhs: The vector to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CIntVector2 operator*(const CIntVector2& Rhs) const noexcept;

    /**
     * @brief: Returns this vector after component-wise multiplication with this and another vector
     *
     * @param Rhs: The vector to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator*=(const CIntVector2& Rhs) noexcept;

    /**
     * @brief: Returns the result of multiplying each component of this vector with a scalar
     *
     * @param Rhs: The scalar to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CIntVector2 operator*(int Rhs) const noexcept;

    /**
     * @brief: Returns this vector after multiplying each component of this vector with a scalar
     *
     * @param Rhs: The scalar to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator*=(int Rhs) noexcept;

    /**
     * @brief: Returns the result of component-wise division with this and another vector
     *
     * @param Rhs: The vector to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CIntVector2 operator/(const CIntVector2& Rhs) const noexcept;

    /**
     * @brief: Returns this vector after component-wise division with this and another vector
     *
     * @param Rhs: The vector to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator/=(const CIntVector2& Rhs) noexcept;

    /**
     * @brief: Returns the result of dividing each component of this vector and a scalar
     *
     * @param Rhs: The scalar to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CIntVector2 operator/(int Rhs) const noexcept;

    /**
     * @brief: Returns this vector after dividing each component of this vector and a scalar
     *
     * @param Rhs: The scalar to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator/=(int Rhs) noexcept;

    /**
     * @brief: Returns the result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==(const CIntVector2& Other) const noexcept;

    /**
     * @brief: Returns the negated result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=(const CIntVector2& Other) const noexcept;

    /**
     * @brief: Returns the component specifed
     *
     * @param Index: The component index
     * @return The component
     */
    FORCEINLINE int& operator[](int Index) noexcept;

    /**
     * @brief: Returns the component specifed
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
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Implementation

FORCEINLINE CIntVector2::CIntVector2() noexcept
    : x(0)
    , y(0)
{
}

FORCEINLINE CIntVector2::CIntVector2(int InX, int InY) noexcept
    : x(InX)
    , y(InY)
{
}

FORCEINLINE CIntVector2::CIntVector2(const int* Arr) noexcept
    : x(Arr[0])
    , y(Arr[1])
{
}

FORCEINLINE CIntVector2::CIntVector2(int Scalar) noexcept
    : x(Scalar)
    , y(Scalar)
{
}

FORCEINLINE bool CIntVector2::IsEqual(const CIntVector2& Other) const noexcept
{
    return x == Other.x && y == Other.y;
}

FORCEINLINE CIntVector2 CIntVector2::operator-() const noexcept
{
    return CIntVector2(-x, -y);
}

FORCEINLINE CIntVector2 CIntVector2::operator+(const CIntVector2& Rhs) const noexcept
{
    return CIntVector2(x + Rhs.x, y + Rhs.y);
}

FORCEINLINE CIntVector2& CIntVector2::operator+=(const CIntVector2& Rhs) noexcept
{
    return *this = *this + Rhs;
}

FORCEINLINE CIntVector2 CIntVector2::operator+(int Rhs) const noexcept
{
    return CIntVector2(x + Rhs, y + Rhs);
}

FORCEINLINE CIntVector2& CIntVector2::operator+=(int Rhs) noexcept
{
    return *this = *this + Rhs;
}

FORCEINLINE CIntVector2 CIntVector2::operator-(const CIntVector2& Rhs) const noexcept
{
    return CIntVector2(x - Rhs.x, y - Rhs.y);
}

FORCEINLINE CIntVector2& CIntVector2::operator-=(const CIntVector2& Rhs) noexcept
{
    return *this = *this - Rhs;
}

FORCEINLINE CIntVector2 CIntVector2::operator-(int Rhs) const noexcept
{
    return CIntVector2(x - Rhs, y - Rhs);
}

FORCEINLINE CIntVector2& CIntVector2::operator-=(int Rhs) noexcept
{
    return *this = *this - Rhs;
}

FORCEINLINE CIntVector2 CIntVector2::operator*(const CIntVector2& Rhs) const noexcept
{
    return CIntVector2(x * Rhs.x, y * Rhs.y);
}

FORCEINLINE CIntVector2& CIntVector2::operator*=(const CIntVector2& Rhs) noexcept
{
    return *this = *this * Rhs;
}

FORCEINLINE CIntVector2 CIntVector2::operator*(int Rhs) const noexcept
{
    return CIntVector2(x * Rhs, y * Rhs);
}

FORCEINLINE CIntVector2& CIntVector2::operator*=(int Rhs) noexcept
{
    return *this = *this * Rhs;
}

FORCEINLINE CIntVector2 CIntVector2::operator/(const CIntVector2& Rhs) const noexcept
{
    return CIntVector2(x / Rhs.x, y / Rhs.y);
}

FORCEINLINE CIntVector2& CIntVector2::operator/=(const CIntVector2& Rhs) noexcept
{
    return *this = *this / Rhs;
}

FORCEINLINE CIntVector2 CIntVector2::operator/(int Rhs) const noexcept
{
    return CIntVector2(x / Rhs, y / Rhs);
}

FORCEINLINE CIntVector2& CIntVector2::operator/=(int Rhs) noexcept
{
    return *this = *this / Rhs;
}

FORCEINLINE bool CIntVector2::operator==(const CIntVector2& Other) const noexcept
{
    return IsEqual(Other);
}

FORCEINLINE bool CIntVector2::operator!=(const CIntVector2& Other) const noexcept
{
    return !IsEqual(Other);
}

FORCEINLINE int& CIntVector2::operator[](int Index) noexcept
{
    Check(Index < 2);
    return reinterpret_cast<int*>(this)[Index];
}

FORCEINLINE int CIntVector2::operator[](int Index) const noexcept
{
    Check(Index < 2);
    return reinterpret_cast<const int*>(this)[Index];
}

FORCEINLINE CIntVector2 Min(const CIntVector2& Lhs, const CIntVector2& Rhs) noexcept
{
    return CIntVector2(NMath::Min(Lhs.x, Rhs.x), NMath::Min(Lhs.y, Rhs.y));
}

FORCEINLINE CIntVector2 Max(const CIntVector2& Lhs, const CIntVector2& Rhs) noexcept
{
    return CIntVector2(NMath::Max(Lhs.x, Rhs.x), NMath::Max(Lhs.y, Rhs.y));
}
