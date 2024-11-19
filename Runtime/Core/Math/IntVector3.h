#pragma once
#include "Core/Math/MathCommon.h"

class FInt16Vector3
{
public:

    /**
     * @brief - Default constructor (Initial1ze components to zero)
     */
    FORCEINLINE FInt16Vector3() noexcept
        : x(0)
        , y(0)
        , z(0)
    {
    }

    /**
     * @brief     - Constructor initializing all components with a corresponding value.
     * @param InX - The x-coordinate
     * @param InY - The y-coordinate
     * @param InZ - The z-coordinate
     */
    FORCEINLINE explicit FInt16Vector3(int16 InX, int16 InY, int16 InZ) noexcept
        : x(InX)
        , y(InY)
        , z(InZ)
    {
    }

    /**
     * @brief     - Constructor initializing all components with an array.
     * @param Arr - Array with 3 elements
     */
    FORCEINLINE explicit FInt16Vector3(const int16* Array) noexcept
        : x(Array[0])
        , y(Array[1])
        , z(Array[2])
    {
    }

    /**
     * @brief        - Constructor initializing all components with a single value.
     * @param Scalar - Value to set all components to
     */
    FORCEINLINE explicit FInt16Vector3(int16 Scalar) noexcept
        : x(Scalar)
        , y(Scalar)
        , z(Scalar)
    {
    }

    /**
     * @brief       - Compares this vector with another vector
     * @param Other - Vector to compare against
     * @return      - True if equal, false if not
     */
    FORCEINLINE bool IsEqual(const FInt16Vector3& Other) const noexcept
    {
        return (x == Other.x) && (y == Other.y) && (z == Other.z);
    }

public:

    /**
     * @brief     - Returns a vector with the smallest of each component of two vectors
     * @param LHS - First vector to compare with
     * @param RHS - Second vector to compare with
     * @return    - A vector with the smallest components of LHS and RHS
     */
    friend FORCEINLINE FInt16Vector3 Min(const FInt16Vector3& LHS, const FInt16Vector3& RHS) noexcept
    {
        return FInt16Vector3(FMath::Min(LHS.x, RHS.x), FMath::Min(LHS.y, RHS.y), FMath::Min(LHS.z, RHS.z));
    }

    /**
     * @brief     - Returns a vector with the largest of each component of two vectors
     * @param LHS - First vector to compare with
     * @param RHS - Second vector to compare with
     * @return    - A vector with the largest components of LHS and RHS
     */
    friend FORCEINLINE FInt16Vector3 Max(const FInt16Vector3& LHS, const FInt16Vector3& RHS) noexcept
    {
        return FInt16Vector3(FMath::Max(LHS.x, RHS.x), FMath::Max(LHS.y, RHS.y), FMath::Max(LHS.z, RHS.z));
    }

public:

    /**
     * @brief  - Return a vector with component-wise negation of this vector
     * @return - A negated vector
     */
    FORCEINLINE FInt16Vector3 operator-() const noexcept
    {
        return FInt16Vector3(-x, -y, -z);
    }

    /**
     * @brief     - Returns the result of component-wise adding this and another vector
     * @param RHS - The vector to add
     * @return    - A vector with the result of addition
     */
    FORCEINLINE FInt16Vector3 operator+(const FInt16Vector3& RHS) const noexcept
    {
        return FInt16Vector3(x + RHS.x, y + RHS.y, z + RHS.z);
    }

    /**
     * @brief     - Returns this vector after component-wise adding this with another vector
     * @param RHS - The vector to add
     * @return    - A reference to this vector
     */
    FORCEINLINE FInt16Vector3& operator+=(const FInt16Vector3& RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief     - Returns the result of adding a scalar to each component of this vector
     * @param RHS - The scalar to add
     * @return    - A vector with the result of addition
     */
    FORCEINLINE FInt16Vector3 operator+(int16 RHS) const noexcept
    {
        return FInt16Vector3(x + RHS, y + RHS, z + RHS);
    }

    /**
     * @brief     - Returns this vector after adding a scalar to each component of this vector
     * @param RHS - The scalar to add
     * @return    - A reference to this vector
     */
    FORCEINLINE FInt16Vector3& operator+=(int16 RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief     - Returns the result of component-wise subtraction between this and another vector
     * @param RHS - The vector to subtract
     * @return    - A vector with the result of subtraction
     */
    FORCEINLINE FInt16Vector3 operator-(const FInt16Vector3& RHS) const noexcept
    {
        return FInt16Vector3(x - RHS.x, y - RHS.y, z - RHS.z);
    }

    /**
     * @brief     - Returns this vector after component-wise subtraction between this and another vector
     * @param RHS - The vector to subtract
     * @return    - A reference to this vector
     */
    FORCEINLINE FInt16Vector3& operator-=(const FInt16Vector3& RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief     - Returns the result of subtracting each component of this vector with a scalar
     * @param RHS - The scalar to subtract
     * @return    - A vector with the result of the subtraction
     */
    FORCEINLINE FInt16Vector3 operator-(int16 RHS) const noexcept
    {
        return FInt16Vector3(x - RHS, y - RHS, z - RHS);
    }

    /**
     * @brief     - Returns this vector after subtracting each component of this vector with a scalar
     * @param RHS - The scalar to subtract
     * @return    - A reference to this vector
     */
    FORCEINLINE FInt16Vector3& operator-=(int16 RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief     - Returns the result of component-wise multiplication with this and another vector
     * @param RHS - The vector to multiply with
     * @return    - A vector with the result of the multiplication
     */
    FORCEINLINE FInt16Vector3 operator*(const FInt16Vector3& RHS) const noexcept
    {
        return FInt16Vector3(x * RHS.x, y * RHS.y, z * RHS.z);
    }

    /**
     * @brief     - Returns this vector after component-wise multiplication with this and another vector
     * @param RHS - The vector to multiply with
     * @return    - A reference to this vector
     */
    FORCEINLINE FInt16Vector3& operator*=(const FInt16Vector3& RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief     - Returns the result of multiplying each component of this vector with a scalar
     * @param RHS - The scalar to multiply with
     * @return    - A vector with the result of the multiplication
     */
    FORCEINLINE FInt16Vector3 operator*(int16 RHS) const noexcept
    {
        return FInt16Vector3(x * RHS, y * RHS, z * RHS);
    }

    /**
     * @brief     - Returns this vector after multiplying each component of this vector with a scalar
     * @param RHS - The scalar to multiply with
     * @return    - A reference to this vector
     */
    FORCEINLINE FInt16Vector3& operator*=(int16 RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief     - Returns the result of component-wise division with this and another vector
     * @param RHS - The vector to divide with
     * @return    - A vector with the result of the division
     */
    FORCEINLINE FInt16Vector3 operator/(const FInt16Vector3& RHS) const noexcept
    {
        return FInt16Vector3(x / RHS.x, y / RHS.y, z / RHS.z);
    }

    /**
     * @brief     - Returns this vector after component-wise division with this and another vector
     * @param RHS - The vector to divide with
     * @return    - A reference to this vector
     */
    FORCEINLINE FInt16Vector3& operator/=(const FInt16Vector3& RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief     - Returns the result of dividing each component of this vector and a scalar
     * @param RHS - The scalar to divide with
     * @return    - A vector with the result of the division
     */
    FORCEINLINE FInt16Vector3 operator/(int16 RHS) const noexcept
    {
        return FInt16Vector3(x / RHS, y / RHS, z / RHS);
    }

    /**
     * @brief     - Returns this vector after dividing each component of this vector and a scalar
     * @param RHS - The scalar to divide with
     * @return    - A reference to this vector
     */
    FORCEINLINE FInt16Vector3& operator/=(int16 RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief       - Returns the result after comparing this and another vector
     * @param Other - The vector to compare with
     * @return      - True if equal, false if not
     */
    FORCEINLINE bool operator==(const FInt16Vector3& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief       - Returns the negated result after comparing this and another vector
     * @param Other - The vector to compare with
     * @return      - False if equal, true if not
     */
    FORCEINLINE bool operator!=(const FInt16Vector3& Other) const noexcept
    {
        return !IsEqual(Other);
    }

    /**
     * @brief       - Returns the component specified
     * @param Index - The component index
     * @return      - The component
     */
    FORCEINLINE int16& operator[](int16 Index) noexcept
    {
        CHECK(Index < 3);
        return reinterpret_cast<int16*>(this)[Index];
    }

    /**
     * @brief       - Returns the component specified
     * @param Index - The component index
     * @return      - The component
     */
    FORCEINLINE int16 operator[](int16 Index) const noexcept
    {
        CHECK(Index < 3);
        return reinterpret_cast<const int16*>(this)[Index];
    }

public:

     /** @brief - The x-coordinate */
    int16 x;

     /** @brief - The y-coordinate */
    int16 y;

     /** @brief - The z-coordinate */
    int16 z;
};

MARK_AS_REALLOCATABLE(FInt16Vector3);


class FIntVector3
{
public:

    /**
     * @brief - Default constructor (Initial1ze components to zero)
     */
    FORCEINLINE FIntVector3() noexcept
        : x(0)
        , y(0)
        , z(0)
    {
    }

    /**
     * @brief     - Constructor initializing all components with a corresponding value.
     * @param InX - The x-coordinate
     * @param InY - The y-coordinate
     * @param InZ - The z-coordinate
     */
    FORCEINLINE explicit FIntVector3(int32 InX, int32 InY, int32 InZ) noexcept
        : x(InX)
        , y(InY)
        , z(InZ)
    {
    }

    /**
     * @brief     - Constructor initializing all components with an array.
     * @param Arr - Array with 3 elements
     */
    FORCEINLINE explicit FIntVector3(const int32* Array) noexcept
        : x(Array[0])
        , y(Array[1])
        , z(Array[2])
    {
    }

    /**
     * @brief        - Constructor initializing all components with a single value.
     * @param Scalar - Value to set all components to
     */
    FORCEINLINE explicit FIntVector3(int32 Scalar) noexcept
        : x(Scalar)
        , y(Scalar)
        , z(Scalar)
    {
    }

    /**
     * @brief       - Compares this vector with another vector
     * @param Other - Vector to compare against
     * @return      - True if equal, false if not
     */
    FORCEINLINE bool IsEqual(const FIntVector3& Other) const noexcept
    {
        return (x == Other.x) && (y == Other.y) && (z == Other.z);
    }

public:

    /**
     * @brief     - Returns a vector with the smallest of each component of two vectors
     * @param LHS - First vector to compare with
     * @param RHS - Second vector to compare with
     * @return    - A vector with the smallest components of LHS and RHS
     */
    friend FORCEINLINE FIntVector3 Min(const FIntVector3& LHS, const FIntVector3& RHS) noexcept
    {
        return FIntVector3(FMath::Min(LHS.x, RHS.x), FMath::Min(LHS.y, RHS.y), FMath::Min(LHS.z, RHS.z));
    }

    /**
     * @brief     - Returns a vector with the largest of each component of two vectors
     * @param LHS - First vector to compare with
     * @param RHS - Second vector to compare with
     * @return    - A vector with the largest components of LHS and RHS
     */
    friend FORCEINLINE FIntVector3 Max(const FIntVector3& LHS, const FIntVector3& RHS) noexcept
    {
        return FIntVector3(FMath::Max(LHS.x, RHS.x), FMath::Max(LHS.y, RHS.y), FMath::Max(LHS.z, RHS.z));
    }

public:

    /**
     * @brief  - Return a vector with component-wise negation of this vector
     * @return - A negated vector
     */
    FORCEINLINE FIntVector3 operator-() const noexcept
    {
        return FIntVector3(-x, -y, -z);
    }

    /**
     * @brief     - Returns the result of component-wise adding this and another vector
     * @param RHS - The vector to add
     * @return    - A vector with the result of addition
     */
    FORCEINLINE FIntVector3 operator+(const FIntVector3& RHS) const noexcept
    {
        return FIntVector3(x + RHS.x, y + RHS.y, z + RHS.z);
    }

    /**
     * @brief     - Returns this vector after component-wise adding this with another vector
     * @param RHS - The vector to add
     * @return    - A reference to this vector
     */
    FORCEINLINE FIntVector3& operator+=(const FIntVector3& RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief     - Returns the result of adding a scalar to each component of this vector
     * @param RHS - The scalar to add
     * @return    - A vector with the result of addition
     */
    FORCEINLINE FIntVector3 operator+(int32 RHS) const noexcept
    {
        return FIntVector3(x + RHS, y + RHS, z + RHS);
    }

    /**
     * @brief     - Returns this vector after adding a scalar to each component of this vector
     * @param RHS - The scalar to add
     * @return    - A reference to this vector
     */
    FORCEINLINE FIntVector3& operator+=(int32 RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief     - Returns the result of component-wise subtraction between this and another vector
     * @param RHS - The vector to subtract
     * @return    - A vector with the result of subtraction
     */
    FORCEINLINE FIntVector3 operator-(const FIntVector3& RHS) const noexcept
    {
        return FIntVector3(x - RHS.x, y - RHS.y, z - RHS.z);
    }

    /**
     * @brief     - Returns this vector after component-wise subtraction between this and another vector
     * @param RHS - The vector to subtract
     * @return    - A reference to this vector
     */
    FORCEINLINE FIntVector3& operator-=(const FIntVector3& RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief     - Returns the result of subtracting each component of this vector with a scalar
     * @param RHS - The scalar to subtract
     * @return    - A vector with the result of the subtraction
     */
    FORCEINLINE FIntVector3 operator-(int32 RHS) const noexcept
    {
        return FIntVector3(x - RHS, y - RHS, z - RHS);
    }

    /**
     * @brief     - Returns this vector after subtracting each component of this vector with a scalar
     * @param RHS - The scalar to subtract
     * @return    - A reference to this vector
     */
    FORCEINLINE FIntVector3& operator-=(int32 RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief     - Returns the result of component-wise multiplication with this and another vector
     * @param RHS - The vector to multiply with
     * @return    - A vector with the result of the multiplication
     */
    FORCEINLINE FIntVector3 operator*(const FIntVector3& RHS) const noexcept
    {
        return FIntVector3(x * RHS.x, y * RHS.y, z * RHS.z);
    }

    /**
     * @brief     - Returns this vector after component-wise multiplication with this and another vector
     * @param RHS - The vector to multiply with
     * @return    - A reference to this vector
     */
    FORCEINLINE FIntVector3& operator*=(const FIntVector3& RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief     - Returns the result of multiplying each component of this vector with a scalar
     * @param RHS - The scalar to multiply with
     * @return    - A vector with the result of the multiplication
     */
    FORCEINLINE FIntVector3 operator*(int32 RHS) const noexcept
    {
        return FIntVector3(x * RHS, y * RHS, z * RHS);
    }

    /**
     * @brief     - Returns this vector after multiplying each component of this vector with a scalar
     * @param RHS - The scalar to multiply with
     * @return    - A reference to this vector
     */
    FORCEINLINE FIntVector3& operator*=(int32 RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief     - Returns the result of component-wise division with this and another vector
     * @param RHS - The vector to divide with
     * @return    - A vector with the result of the division
     */
    FORCEINLINE FIntVector3 operator/(const FIntVector3& RHS) const noexcept
    {
        return FIntVector3(x / RHS.x, y / RHS.y, z / RHS.z);
    }

    /**
     * @brief     - Returns this vector after component-wise division with this and another vector
     * @param RHS - The vector to divide with
     * @return    - A reference to this vector
     */
    FORCEINLINE FIntVector3& operator/=(const FIntVector3& RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief     - Returns the result of dividing each component of this vector and a scalar
     * @param RHS - The scalar to divide with
     * @return    - A vector with the result of the division
     */
    FORCEINLINE FIntVector3 operator/(int32 RHS) const noexcept
    {
        return FIntVector3(x / RHS, y / RHS, z / RHS);
    }

    /**
     * @brief     - Returns this vector after dividing each component of this vector and a scalar
     * @param RHS - The scalar to divide with
     * @return    - A reference to this vector
     */
    FORCEINLINE FIntVector3& operator/=(int32 RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief       - Returns the result after comparing this and another vector
     * @param Other - The vector to compare with
     * @return      - True if equal, false if not
     */
    FORCEINLINE bool operator==(const FIntVector3& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief       - Returns the negated result after comparing this and another vector
     * @param Other - The vector to compare with
     * @return      - False if equal, true if not
     */
    FORCEINLINE bool operator!=(const FIntVector3& Other) const noexcept
    {
        return !IsEqual(Other);
    }

    /**
     * @brief       - Returns the component specified
     * @param Index - The component index
     * @return      - The component
     */
    FORCEINLINE int32& operator[](int32 Index) noexcept
    {
        CHECK(Index < 3);
        return reinterpret_cast<int32*>(this)[Index];
    }

    /**
     * @brief       - Returns the component specified
     * @param Index - The component index
     * @return      - The component
     */
    FORCEINLINE int32 operator[](int32 Index) const noexcept
    {
        CHECK(Index < 3);
        return reinterpret_cast<const int32*>(this)[Index];
    }

public:

     /** @brief - The x-coordinate */
    int32 x;

     /** @brief - The y-coordinate */
    int32 y;

     /** @brief - The z-coordinate */
    int32 z;
};

MARK_AS_REALLOCATABLE(FIntVector3);