#pragma once
#include "MathCommon.h"

#pragma once
#include "MathCommon.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// A 2-D vector (x, y) using integers

class CInt16Vector2
{
public:

    /** Default constructor (Initialize components to zero) */
    FORCEINLINE CInt16Vector2() noexcept
        : x(0)
        , y(0)
    { }

    /**
     * @brief: Constructor initializing all components with a corresponding value.
     *
     * @param InX: The x-coordinate
     * @param InY: The y-coordinate
     */
    FORCEINLINE explicit CInt16Vector2(int16 InX, int16 InY) noexcept
        : x(InX)
        , y(InY)
    { }

    /**
     * @brief: Constructor initializing all components with an array.
     *
     * @param Arr: Array with 2 elements
     */
    FORCEINLINE explicit CInt16Vector2(const int16* Arr) noexcept
        : x(Arr[0])
        , y(Arr[1])
    { }

    /**
     * @brief: Constructor initializing all components with a single value.
     *
     * @param Scalar: Value to set all components to
     */
    FORCEINLINE explicit CInt16Vector2(int16 Scalar) noexcept
        : x(Scalar)
        , y(Scalar)
    { }

    /**
     * @brief: Compares this vector with another vector
     *
     * @param Other: Vector to compare against
     * @return True if equal, false if not
     */
    FORCEINLINE bool IsEqual(const CInt16Vector2& Other) const noexcept
    {
        return (x == Other.x) && (y == Other.y);
    }

public:

    /**
     * @brief: Returns a vector with the smallest of each component of two vectors
     *
     * @param LHS: First vector to compare with
     * @param RHS: Second vector to compare with
     * @return A vector with the smallest components of LHS and RHS
     */
    friend FORCEINLINE CInt16Vector2 Min(const CInt16Vector2& LHS, const CInt16Vector2& RHS) noexcept
    {
        return CInt16Vector2(NMath::Min(LHS.x, RHS.x), NMath::Min(LHS.y, RHS.y));
    }

    /**
     * @brief: Returns a vector with the largest of each component of two vectors
     *
     * @param LHS: First vector to compare with
     * @param RHS: Second vector to compare with
     * @return A vector with the largest components of LHS and RHS
     */
    friend FORCEINLINE CInt16Vector2 Max(const CInt16Vector2& LHS, const CInt16Vector2& RHS) noexcept
    {
        return CInt16Vector2(NMath::Max(LHS.x, RHS.x), NMath::Max(LHS.y, RHS.y));
    }

public:

    /**
     * @brief: Return a vector with component-wise negation of this vector
     *
     * @return A negated vector
     */
    FORCEINLINE CInt16Vector2 operator-() const noexcept
    {
        return CInt16Vector2(-x, -y);
    }

    /**
     * @brief: Returns the result of component-wise adding this and another vector
     *
     * @param RHS: The vector to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CInt16Vector2 operator+(const CInt16Vector2& RHS) const noexcept
    {
        return CInt16Vector2(x + RHS.x, y + RHS.y);
    }

    /**
     * @brief: Returns this vector after component-wise adding this with another vector
     *
     * @param RHS: The vector to add
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector2& operator+=(const CInt16Vector2& RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief: Returns the result of adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CInt16Vector2 operator+(int16 RHS) const noexcept
    {
        return CInt16Vector2(x + RHS, y + RHS);
    }

    /**
     * @brief: Returns this vector after adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector2& operator+=(int16 RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief: Returns the result of component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A vector with the result of subtraction
     */
    FORCEINLINE CInt16Vector2 operator-(const CInt16Vector2& RHS) const noexcept
    {
        return CInt16Vector2(x - RHS.x, y - RHS.y);
    }

    /**
     * @brief: Returns this vector after component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector2& operator-=(const CInt16Vector2& RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief: Returns the result of subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A vector with the result of the subtraction
     */
    FORCEINLINE CInt16Vector2 operator-(int16 RHS) const noexcept
    {
        return CInt16Vector2(x - RHS, y - RHS);
    }

    /**
     * @brief: Returns this vector after subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector2& operator-=(int16 RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief: Returns the result of component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CInt16Vector2 operator*(const CInt16Vector2& RHS) const noexcept
    {
        return CInt16Vector2(x * RHS.x, y * RHS.y);
    }

    /**
     * @brief: Returns this vector after component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector2& operator*=(const CInt16Vector2& RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief: Returns the result of multiplying each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CInt16Vector2 operator*(int16 RHS) const noexcept
    {
        return CInt16Vector2(x * RHS, y * RHS);
    }

    /**
     * @brief: Returns this vector after multiplying each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector2& operator*=(int16 RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief: Returns the result of component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CInt16Vector2 operator/(const CInt16Vector2& RHS) const noexcept
    {
        return CInt16Vector2(x / RHS.x, y / RHS.y);
    }

    /**
     * @brief: Returns this vector after component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector2& operator/=(const CInt16Vector2& RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief: Returns the result of dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CInt16Vector2 operator/(int16 RHS) const noexcept
    {
        return CInt16Vector2(x / RHS, y / RHS);
    }

    /**
     * @brief: Returns this vector after dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector2& operator/=(int16 RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief: Returns the result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==(const CInt16Vector2& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief: Returns the negated result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=(const CInt16Vector2& Other) const noexcept
    {
        return !IsEqual(Other);
    }

    /**
     * @brief: Returns the component specified
     *
     * @param Index: The component index
     * @return The component
     */
    FORCEINLINE int16& operator[](int16 Index) noexcept
    {
        Check(Index < 2);
        return reinterpret_cast<int16*>(this)[Index];
    }

    /**
     * @brief: Returns the component specified
     *
     * @param Index: The component index
     * @return The component
     */
    FORCEINLINE int16 operator[](int16 Index) const noexcept
    {
        Check(Index < 2);
        return reinterpret_cast<const int16*>(this)[Index];
    }

public:

    /* The x-coordinate */
    int16 x;

    /* The y-coordinate */
    int16 y;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CIntVector2 - A 2-D vector (x, y) using 32-bit integers

class CIntVector2
{
public:

    /** Default constructor (Initialize components to zero) */
    FORCEINLINE CIntVector2() noexcept
        : x(0)
        , y(0)
    { }

    /**
     * @brief: Constructor initializing all components with a corresponding value.
     *
     * @param InX: The x-coordinate
     * @param InY: The y-coordinate
     */
    FORCEINLINE explicit CIntVector2(int32 InX, int32 InY) noexcept
        : x(InX)
        , y(InY)
    { }

    /**
     * @brief: Constructor initializing all components with an array.
     *
     * @param Arr: Array with 2 elements
     */
    FORCEINLINE explicit CIntVector2(const int32* Arr) noexcept
        : x(Arr[0])
        , y(Arr[1])
    { }

    /**
     * @brief: Constructor initializing all components with a single value.
     *
     * @param Scalar: Value to set all components to
     */
    FORCEINLINE explicit CIntVector2(int32 Scalar) noexcept
        : x(Scalar)
        , y(Scalar)
    { }

    /**
     * @brief: Compares this vector with another vector
     *
     * @param Other: Vector to compare against
     * @return True if equal, false if not
     */
    FORCEINLINE bool IsEqual(const CIntVector2& Other) const noexcept
    {
        return (x == Other.x) && (y == Other.y);
    }

public:
    
    /**
     * @brief: Returns a vector with the smallest of each component of two vectors
     *
     * @param LHS: First vector to compare with
     * @param RHS: Second vector to compare with
     * @return A vector with the smallest components of LHS and RHS
     */
    friend FORCEINLINE CIntVector2 Min(const CIntVector2& LHS, const CIntVector2& RHS) noexcept
    {
        return CIntVector2(NMath::Min(LHS.x, RHS.x), NMath::Min(LHS.y, RHS.y));
    }

    /**
     * @brief: Returns a vector with the largest of each component of two vectors
     *
     * @param LHS: First vector to compare with
     * @param RHS: Second vector to compare with
     * @return A vector with the largest components of LHS and RHS
     */
    friend FORCEINLINE CIntVector2 Max(const CIntVector2& LHS, const CIntVector2& RHS) noexcept
    {
        return CIntVector2(NMath::Max(LHS.x, RHS.x), NMath::Max(LHS.y, RHS.y));
    }

public:

    /**
     * @brief: Return a vector with component-wise negation of this vector
     *
     * @return A negated vector
     */
    FORCEINLINE CIntVector2 operator-() const noexcept
    {
        return CIntVector2(-x, -y);
    }

    /**
     * @brief: Returns the result of component-wise adding this and another vector
     *
     * @param RHS: The vector to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CIntVector2 operator+(const CIntVector2& RHS) const noexcept
    {
        return CIntVector2(x + RHS.x, y + RHS.y);
    }

    /**
     * @brief: Returns this vector after component-wise adding this with another vector
     *
     * @param RHS: The vector to add
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator+=(const CIntVector2& RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief: Returns the result of adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CIntVector2 operator+(int32 RHS) const noexcept
    {
        return CIntVector2(x + RHS, y + RHS);
    }

    /**
     * @brief: Returns this vector after adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator+=(int32 RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief: Returns the result of component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A vector with the result of subtraction
     */
    FORCEINLINE CIntVector2 operator-(const CIntVector2& RHS) const noexcept
    {
        return CIntVector2(x - RHS.x, y - RHS.y);
    }

    /**
     * @brief: Returns this vector after component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator-=(const CIntVector2& RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief: Returns the result of subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A vector with the result of the subtraction
     */
    FORCEINLINE CIntVector2 operator-(int32 RHS) const noexcept
    {
        return CIntVector2(x - RHS, y - RHS);
    }

    /**
     * @brief: Returns this vector after subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator-=(int32 RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief: Returns the result of component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CIntVector2 operator*(const CIntVector2& RHS) const noexcept
    {
        return CIntVector2(x * RHS.x, y * RHS.y);
    }

    /**
     * @brief: Returns this vector after component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator*=(const CIntVector2& RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief: Returns the result of multiplying each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CIntVector2 operator*(int32 RHS) const noexcept
    {
        return CIntVector2(x * RHS, y * RHS);
    }

    /**
     * @brief: Returns this vector after multiplying each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator*=(int32 RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief: Returns the result of component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CIntVector2 operator/(const CIntVector2& RHS) const noexcept
    {
        return CIntVector2(x / RHS.x, y / RHS.y);
    }

    /**
     * @brief: Returns this vector after component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator/=(const CIntVector2& RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief: Returns the result of dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CIntVector2 operator/(int32 RHS) const noexcept
    {
        return CIntVector2(x / RHS, y / RHS);
    }

    /**
     * @brief: Returns this vector after dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector2& operator/=(int32 RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief: Returns the result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==(const CIntVector2& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief: Returns the negated result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=(const CIntVector2& Other) const noexcept
    {
        return !IsEqual(Other);
    }

    /**
     * @brief: Returns the component specified
     *
     * @param Index: The component index
     * @return The component
     */
    FORCEINLINE int32& operator[](int32 Index) noexcept
    {
        Check(Index < 2);
        return reinterpret_cast<int32*>(this)[Index];
    }

    /**
     * @brief: Returns the component specified
     *
     * @param Index: The component index
     * @return The component
     */
    FORCEINLINE int32 operator[](int32 Index) const noexcept
    {
        Check(Index < 2);
        return reinterpret_cast<const int32*>(this)[Index];
    }

public:
    
    /* The x-coordinate */
    int32 x;

    /* The y-coordinate */
    int32 y;
};
