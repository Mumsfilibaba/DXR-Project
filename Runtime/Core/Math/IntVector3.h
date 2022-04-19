#pragma once
#include "MathCommon.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// A 3-D vector (x, y, z) using 16-bit integers

class CInt16Vector3
{
public:

    /**
     * @brief: Default constructor (Initial1ze components to zero)
     */
    FORCEINLINE CInt16Vector3() noexcept
        : x(0)
        , y(0)
        , z(0)
    { }

    /**
     * @brief: Constructor initializing all components with a corresponding value.
     *
     * @param InX: The x-coordinate
     * @param InY: The y-coordinate
     * @param InZ: The z-coordinate
     */
    FORCEINLINE explicit CInt16Vector3(int16 InX, int16 InY, int16 InZ) noexcept
        : x(InX)
        , y(InY)
        , z(InZ)
    { }

    /**
     * @brief: Constructor initializing all components with an array.
     *
     * @param Arr: Array with 3 elements
     */
    FORCEINLINE explicit CInt16Vector3(const int16* Array) noexcept
        : x(Array[0])
        , y(Array[1])
        , z(Array[2])
    { }

    /**
     * @brief: Constructor initializing all components with a single value.
     *
     * @param Scalar: Value to set all components to
     */
    FORCEINLINE explicit CInt16Vector3(int16 Scalar) noexcept
        : x(Scalar)
        , y(Scalar)
        , z(Scalar)
    { }

    /**
     * @brief: Compares this vector with another vector
     *
     * @param Other: Vector to compare against
     * @return True if equal, false if not
     */
    FORCEINLINE bool IsEqual(const CInt16Vector3& Other) const noexcept
    {
        return (x == Other.x) && (y == Other.y) && (z == Other.z);
    }

public:

    /**
     * @brief: Returns a vector with the smallest of each component of two vectors
     *
     * @param LHS: First vector to compare with
     * @param RHS: Second vector to compare with
     * @return A vector with the smallest components of LHS and RHS
     */
    friend FORCEINLINE CInt16Vector3 Min(const CInt16Vector3& LHS, const CInt16Vector3& RHS) noexcept
    {
        return CInt16Vector3(NMath::Min(LHS.x, RHS.x), NMath::Min(LHS.y, RHS.y), NMath::Min(LHS.z, RHS.z));
    }

    /**
     * @brief: Returns a vector with the largest of each component of two vectors
     *
     * @param LHS: First vector to compare with
     * @param RHS: Second vector to compare with
     * @return A vector with the largest components of LHS and RHS
     */
    friend FORCEINLINE CInt16Vector3 Max(const CInt16Vector3& LHS, const CInt16Vector3& RHS) noexcept
    {
        return CInt16Vector3(NMath::Max(LHS.x, RHS.x), NMath::Max(LHS.y, RHS.y), NMath::Max(LHS.z, RHS.z));
    }

public:

    /**
     * @brief: Return a vector with component-wise negation of this vector
     *
     * @return A negated vector
     */
    FORCEINLINE CInt16Vector3 operator-() const noexcept
    {
        return CInt16Vector3(-x, -y, -z);
    }

    /**
     * @brief: Returns the result of component-wise adding this and another vector
     *
     * @param RHS: The vector to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CInt16Vector3 operator+(const CInt16Vector3& RHS) const noexcept
    {
        return CInt16Vector3(x + RHS.x, y + RHS.y, z + RHS.z);
    }

    /**
     * @brief: Returns this vector after component-wise adding this with another vector
     *
     * @param RHS: The vector to add
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector3& operator+=(const CInt16Vector3& RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief: Returns the result of adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CInt16Vector3 operator+(int16 RHS) const noexcept
    {
        return CInt16Vector3(x + RHS, y + RHS, z + RHS);
    }

    /**
     * @brief: Returns this vector after adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector3& operator+=(int16 RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief: Returns the result of component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A vector with the result of subtraction
     */
    FORCEINLINE CInt16Vector3 operator-(const CInt16Vector3& RHS) const noexcept
    {
        return CInt16Vector3(x - RHS.x, y - RHS.y, z - RHS.z);
    }

    /**
     * @brief: Returns this vector after component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector3& operator-=(const CInt16Vector3& RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief: Returns the result of subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A vector with the result of the subtraction
     */
    FORCEINLINE CInt16Vector3 operator-(int16 RHS) const noexcept
    {
        return CInt16Vector3(x - RHS, y - RHS, z - RHS);
    }

    /**
     * @brief: Returns this vector after subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector3& operator-=(int16 RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief: Returns the result of component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CInt16Vector3 operator*(const CInt16Vector3& RHS) const noexcept
    {
        return CInt16Vector3(x * RHS.x, y * RHS.y, z * RHS.z);
    }

    /**
     * @brief: Returns this vector after component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector3& operator*=(const CInt16Vector3& RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief: Returns the result of multiplying each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CInt16Vector3 operator*(int16 RHS) const noexcept
    {
        return CInt16Vector3(x * RHS, y * RHS, z * RHS);
    }

    /**
     * @brief: Returns this vector after multiplying each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector3& operator*=(int16 RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief: Returns the result of component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CInt16Vector3 operator/(const CInt16Vector3& RHS) const noexcept
    {
        return CInt16Vector3(x / RHS.x, y / RHS.y, z / RHS.z);
    }

    /**
     * @brief: Returns this vector after component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector3& operator/=(const CInt16Vector3& RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief: Returns the result of dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CInt16Vector3 operator/(int16 RHS) const noexcept
    {
        return CInt16Vector3(x / RHS, y / RHS, z / RHS);
    }

    /**
     * @brief: Returns this vector after dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CInt16Vector3& operator/=(int16 RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief: Returns the result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==(const CInt16Vector3& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief: Returns the negated result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=(const CInt16Vector3& Other) const noexcept
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
        Check(Index < 3);
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
        Check(Index < 3);
        return reinterpret_cast<const int16*>(this)[Index];
    }

public:

    /* The x-coordinate */
    int16 x;

    /* The y-coordinate */
    int16 y;

    /* The z-coordinate */
    int16 z;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// A 3-D vector (x, y, z) using integers

class CIntVector3
{
public:

    /** 
     * @brief: Default constructor (Initial1ze components to zero) 
     */
    FORCEINLINE CIntVector3() noexcept
        : x(0)
        , y(0)
        , z(0)
    { }

    /**
     * @brief: Constructor initializing all components with a corresponding value.
     *
     * @param InX: The x-coordinate
     * @param InY: The y-coordinate
     * @param InZ: The z-coordinate
     */
    FORCEINLINE explicit CIntVector3(int32 InX, int32 InY, int32 InZ) noexcept
        : x(InX)
        , y(InY)
        , z(InZ)
    { }

    /**
     * @brief: Constructor initializing all components with an array.
     *
     * @param Arr: Array with 3 elements
     */
    FORCEINLINE explicit CIntVector3(const int32* Array) noexcept
        : x(Array[0])
        , y(Array[1])
        , z(Array[2])
    { }

    /**
     * @brief: Constructor initializing all components with a single value.
     *
     * @param Scalar: Value to set all components to
     */
    FORCEINLINE explicit CIntVector3(int32 Scalar) noexcept
        : x(Scalar)
        , y(Scalar)
        , z(Scalar)
    { }

    /**
     * @brief: Compares this vector with another vector
     *
     * @param Other: Vector to compare against
     * @return True if equal, false if not
     */
    FORCEINLINE bool IsEqual(const CIntVector3& Other) const noexcept
    {
        return (x == Other.x) && (y == Other.y) && (z == Other.z);
    }

public:

    /**
     * @brief: Returns a vector with the smallest of each component of two vectors
     *
     * @param LHS: First vector to compare with
     * @param RHS: Second vector to compare with
     * @return A vector with the smallest components of LHS and RHS
     */
    friend FORCEINLINE CIntVector3 Min(const CIntVector3& LHS, const CIntVector3& RHS) noexcept
    {
        return CIntVector3(NMath::Min(LHS.x, RHS.x), NMath::Min(LHS.y, RHS.y), NMath::Min(LHS.z, RHS.z));
    }

    /**
     * @brief: Returns a vector with the largest of each component of two vectors
     *
     * @param LHS: First vector to compare with
     * @param RHS: Second vector to compare with
     * @return A vector with the largest components of LHS and RHS
     */
    friend FORCEINLINE CIntVector3 Max(const CIntVector3& LHS, const CIntVector3& RHS) noexcept
    {
        return CIntVector3(NMath::Max(LHS.x, RHS.x), NMath::Max(LHS.y, RHS.y), NMath::Max(LHS.z, RHS.z));
    }

public:
    /**
     * @brief: Return a vector with component-wise negation of this vector
     *
     * @return A negated vector
     */
    FORCEINLINE CIntVector3 operator-() const noexcept
    {
        return CIntVector3(-x, -y, -z);
    }

    /**
     * @brief: Returns the result of component-wise adding this and another vector
     *
     * @param RHS: The vector to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CIntVector3 operator+(const CIntVector3& RHS) const noexcept
    {
        return CIntVector3(x + RHS.x, y + RHS.y, z + RHS.z);
    }

    /**
     * @brief: Returns this vector after component-wise adding this with another vector
     *
     * @param RHS: The vector to add
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator+=(const CIntVector3& RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief: Returns the result of adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CIntVector3 operator+(int32 RHS) const noexcept
    {
        return CIntVector3(x + RHS, y + RHS, z + RHS);
    }

    /**
     * @brief: Returns this vector after adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator+=(int32 RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief: Returns the result of component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A vector with the result of subtraction
     */
    FORCEINLINE CIntVector3 operator-(const CIntVector3& RHS) const noexcept
    {
        return CIntVector3(x - RHS.x, y - RHS.y, z - RHS.z);
    }

    /**
     * @brief: Returns this vector after component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator-=(const CIntVector3& RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief: Returns the result of subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A vector with the result of the subtraction
     */
    FORCEINLINE CIntVector3 operator-(int32 RHS) const noexcept
    {
        return CIntVector3(x - RHS, y - RHS, z - RHS);
    }

    /**
     * @brief: Returns this vector after subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator-=(int32 RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief: Returns the result of component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CIntVector3 operator*(const CIntVector3& RHS) const noexcept
    {
        return CIntVector3(x * RHS.x, y * RHS.y, z * RHS.z);
    }

    /**
     * @brief: Returns this vector after component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator*=(const CIntVector3& RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief: Returns the result of multiplying each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CIntVector3 operator*(int32 RHS) const noexcept
    {
        return CIntVector3(x * RHS, y * RHS, z * RHS);
    }

    /**
     * @brief: Returns this vector after multiplying each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator*=(int32 RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief: Returns the result of component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CIntVector3 operator/(const CIntVector3& RHS) const noexcept
    {
        return CIntVector3(x / RHS.x, y / RHS.y, z / RHS.z);
    }

    /**
     * @brief: Returns this vector after component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator/=(const CIntVector3& RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief: Returns the result of dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CIntVector3 operator/(int32 RHS) const noexcept
    {
        return CIntVector3(x / RHS, y / RHS, z / RHS);
    }

    /**
     * @brief: Returns this vector after dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CIntVector3& operator/=(int32 RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief: Returns the result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==(const CIntVector3& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief: Returns the negated result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=(const CIntVector3& Other) const noexcept
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
        Check(Index < 3);
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
        Check(Index < 3);
        return reinterpret_cast<const int32*>(this)[Index];
    }

public:

    /* The x-coordinate */
    int32 x;

    /* The y-coordinate */
    int32 y;
    
    /* The z-coordinate */
    int32 z;
};
