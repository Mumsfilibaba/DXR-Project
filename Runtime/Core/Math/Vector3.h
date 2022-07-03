#pragma once
#include "MathCommon.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FVector3

class FVector3
{
public:

    /** 
     * @brief: Default constructor (Initialize components to zero) 
     */
    FORCEINLINE FVector3() noexcept
        : x(0.0f)
        , y(0.0f)
        , z(0.0f)
    { }

    /**
     * @brief: Constructor initializing all components with a corresponding value.
     *
     * @param InX: The x-coordinate
     * @param InY: The y-coordinate
     * @param InZ: The z-coordinate
     */
    FORCEINLINE explicit FVector3(float InX, float InY, float InZ) noexcept
        : x(InX)
        , y(InY)
        , z(InZ)
    { }

    /**
     * @brief: Constructor initializing all components with an array.
     *
     * @param Arr: Array with 3 elements
     */
    FORCEINLINE explicit FVector3(const float* Arr) noexcept
        : x(Arr[0])
        , y(Arr[1])
        , z(Arr[2])
    { }

    /**
     * @brief: Constructor initializing all components with a single value.
     *
     * @param Scalar: Value to set all components to
     */
    FORCEINLINE explicit FVector3(float Scalar) noexcept
        : x(Scalar)
        , y(Scalar)
        , z(Scalar)
    { }

     /** @brief: Normalized this vector */
    inline void Normalize() noexcept
    {
        const float fLengthSquared = LengthSquared();
        if (fLengthSquared != 0.0f)
        {
            const float fRecipLength = 1.0f / NMath::Sqrt(fLengthSquared);
            x *= fRecipLength;
            y *= fRecipLength;
            z *= fRecipLength;
        }
    }

    /**
     * @brief: Returns a normalized version of this vector
     *
     * @return: A copy of this vector normalized
     */
    FORCEINLINE FVector3 GetNormalized() const noexcept
    {
        FVector3 Result(*this);
        Result.Normalize();
        return Result;
    }

    /**
     * @brief: Compares, within a threshold Epsilon, this vector with another vector
     *
     * @param Other: vector to compare against
     * @return: True if equal, false if not
     */
    inline bool IsEqual(const FVector3& Other, float Epsilon = NMath::kIsEqualEpsilon) const noexcept
    {
        Epsilon = NMath::Abs(Epsilon);

        for (int32 i = 0; i < 3; i++)
        {
            float Diff = reinterpret_cast<const float*>(this)[i] - reinterpret_cast<const float*>(&Other)[i];
            if (NMath::Abs(Diff) > Epsilon)
            {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief: Checks weather this vector is a unit vector not
     *
     * @return: True if the length equals one, false if not
     */
    FORCEINLINE bool IsUnitVector() const noexcept
    {
        const float fLengthSquared = NMath::Abs(1.0f - LengthSquared());
        return (fLengthSquared < NMath::kIsEqualEpsilon);
    }

    /**
     * @brief: Checks weather this vector has any component that equals NaN
     *
     * @return: True if the any component equals NaN, false if not
     */
    FORCEINLINE bool HasNaN() const noexcept
    {
        for (int32 Index = 0; Index < 3; ++Index)
        {
            if (NMath::IsNaN(reinterpret_cast<const float*>(this)[Index]))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief: Checks weather this vector has any component that equals infinity
     *
     * @return: True if the any component equals infinity, false if not
     */
    FORCEINLINE bool HasInfinity() const noexcept
    {
        for (int32 Index = 0; Index < 3; ++Index)
        {
            if (NMath::IsInfinity(reinterpret_cast<const float*>(this)[Index]))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief: Checks weather this vector has any value that equals infinity or NaN
     *
     * @return: False if the any value equals infinity or NaN, true if not
     */
    FORCEINLINE bool IsValid() const noexcept
    {
        return !HasNaN() && !HasInfinity();
    }

    /**
     * @brief: Returns the length of this vector
     *
     * @return: The length of the vector
     */
    FORCEINLINE float Length() const noexcept
    {
        const float fLengthSquared = LengthSquared();
        return NMath::Sqrt(fLengthSquared);
    }

    /**
     * @brief: Returns the length of this vector squared
     *
     * @return: The length of the vector squared
     */
    FORCEINLINE float LengthSquared() const noexcept
    {
        return DotProduct(*this);
    }

    /**
     * @brief: Returns the dot product between this and another vector
     *
     * @param Other: The vector to perform dot product with
     * @return: The dot product
     */
    FORCEINLINE float DotProduct(const FVector3& Other) const noexcept
    {
        return (x * Other.x) + (y * Other.y) + (z * Other.z);
    }

    /**
     * @brief: Returns the cross product of this vector and another vector.
     *
     * @param Other: The vector to perform cross product with
     * @return: The cross product
     */
    inline FVector3 CrossProduct(const FVector3& Other) const noexcept
    {
        return FVector3((y * Other.z) - (z * Other.y)
                       ,(z * Other.x) - (x * Other.z)
                       ,(x * Other.y) - (y * Other.x));
    }

    /**
     * @brief: Returns the resulting vector after projecting this vector onto another.
     *
     * @param Other: The vector to project onto
     * @return: The projected vector
     */
    inline FVector3 ProjectOn(const FVector3& Other) const noexcept
    {
        float AdotB = DotProduct(Other);
        float BdotB = Other.LengthSquared();
        return (AdotB / BdotB) * Other;
    }

    /**
     * @brief: Returns the reflected vector after reflecting this vector around a normal.
     *
     * @param Normal: Vector to reflect around
     * @return: The reflected vector
     */
    inline FVector3 Reflect(const FVector3& Normal) const noexcept
    {
        float VdotN = DotProduct(Normal);
        float NdotN = Normal.LengthSquared();
        return *this - ((2.0f * (VdotN / NdotN)) * Normal);
    }

    /**
     * @brief: Returns the data of this matrix as a pointer
     *
     * @return: A pointer to the data
     */
    FORCEINLINE float* Data() noexcept
    {
        return reinterpret_cast<float*>(this);
    }

    /**
     * @brief: Returns the data of this matrix as a pointer
     *
     * @return: A pointer to the data
     */
    FORCEINLINE const float* Data() const noexcept
    {
        return reinterpret_cast<const float*>(this);
    }

public:

    /**
     * @brief: Returns a vector with the smallest of each component of two vectors
     *
     * @param First: First vector to compare with
     * @param Second: Second vector to compare with
     * @return: A vector with the smallest components of First and Second
     */
    friend FORCEINLINE FVector3 Min(const FVector3& First, const FVector3& Second) noexcept
    {
        return FVector3(NMath::Min(First.x, Second.x), NMath::Min(First.y, Second.y), NMath::Min(First.z, Second.z));
    }

    /**
     * @brief: Returns a vector with the largest of each component of two vectors
     *
     * @param First: First vector to compare with
     * @param Second: Second vector to compare with
     * @return: A vector with the largest components of First and Second
     */
    friend FORCEINLINE FVector3 Max(const FVector3& First, const FVector3& Second) noexcept
    {
        return FVector3(NMath::Max(First.x, Second.x), NMath::Max(First.y, Second.y), NMath::Max(First.z, Second.z));
    }

    /**
     * @brief: Returns the linear interpolation between two vectors
     *
     * @param First: First vector to interpolate
     * @param Second: Second vector to interpolate
     * @param Factor: Factor to interpolate with. Zero returns First, One returns seconds
     * @return: A vector with the result of interpolation
     */
    friend FORCEINLINE FVector3 Lerp(const FVector3& First, const FVector3& Second, float Factor) noexcept
    {
        return FVector3((1.0f - Factor) * First.x + Factor * Second.x
                       ,(1.0f - Factor) * First.y + Factor * Second.y
                       ,(1.0f - Factor) * First.z + Factor * Second.z);
    }

    /**
     * @brief: Returns a vector with all the components within the range of a min and max value
     *
     * @param Min: Vector with minimum values
     * @param Max: Vector with maximum values
     * @param Value: Vector to clamp
     * @return: A vector with the result of clamping
     */
    friend FORCEINLINE FVector3 Clamp(const FVector3& Min, const FVector3& Max, const FVector3& Value) noexcept
    {
        return FVector3(NMath::Min(NMath::Max(Value.x, Min.x), Max.x)
                       ,NMath::Min(NMath::Max(Value.y, Min.y), Max.y)
                       ,NMath::Min(NMath::Max(Value.z, Min.z), Max.z));
    }

    /**
     * @brief: Returns a vector with all the components within the range zero and one
     *
     * @param Value: Value to saturate
     * @return: A vector with the result of saturation
     */
    friend FORCEINLINE FVector3 Saturate(const FVector3& Value) noexcept
    {
        return FVector3(NMath::Min(NMath::Max(Value.x, 0.0f), 1.0f)
                       ,NMath::Min(NMath::Max(Value.y, 0.0f), 1.0f)
                       ,NMath::Min(NMath::Max(Value.z, 0.0f), 1.0f));
    }

public:

    /**
     * @brief: Return a vector with component-wise negation of this vector
     *
     * @return: A negated vector
     */
    FORCEINLINE FVector3 operator-() const noexcept
    {
        return FVector3(-x, -y, -z);
    }

    /**
     * @brief: Returns the result of component-wise adding this and another vector
     *
     * @param RHS: The vector to add
     * @return: A vector with the result of addition
     */
    FORCEINLINE FVector3 operator+(const FVector3& RHS) const noexcept
    {
        return FVector3(x + RHS.x, y + RHS.y, z + RHS.z);
    }

    /**
     * @brief: Returns this vector after component-wise adding this with another vector
     *
     * @param RHS: The vector to add
     * @return: A reference to this vector
     */
    FORCEINLINE FVector3& operator+=(const FVector3& RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief: Returns the result of adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return: A vector with the result of addition
     */
    FORCEINLINE FVector3 operator+(float RHS) const noexcept
    {
        return FVector3(x + RHS, y + RHS, z + RHS);
    }

    /**
     * @brief: Returns this vector after adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return: A reference to this vector
     */
    FORCEINLINE FVector3& operator+=(float RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief: Returns the result of component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return: A vector with the result of subtraction
     */
    FORCEINLINE FVector3 operator-(const FVector3& RHS) const noexcept
    {
        return FVector3(x - RHS.x, y - RHS.y, z - RHS.z);
    }

    /**
     * @brief: Returns this vector after component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return: A reference to this vector
     */
    FORCEINLINE FVector3& operator-=(const FVector3& RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief: Returns the result of subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return: A vector with the result of the subtraction
     */
    FORCEINLINE FVector3 operator-(float RHS) const noexcept
    {
        return FVector3(x - RHS, y - RHS, z - RHS);
    }

    /**
     * @brief: Returns this vector after subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return: A reference to this vector
     */
    FORCEINLINE FVector3& operator-=(float RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief: Returns the result of component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return: A vector with the result of the multiplication
     */
    FORCEINLINE FVector3 operator*(const FVector3& RHS) const noexcept
    {
        return FVector3(x * RHS.x, y * RHS.y, z * RHS.z);
    }

    /**
     * @brief: Returns this vector after component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return: A reference to this vector
     */
    FORCEINLINE FVector3& operator*=(const FVector3& RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief: Returns the result of multiplying each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return: A vector with the result of the multiplication
     */
    FORCEINLINE FVector3 operator*(float RHS) const noexcept
    {
        return FVector3(x * RHS, y * RHS, z * RHS);
    }

    /**
     * @brief: Returns the result of multiplying each component of a vector with a scalar
     *
     * @param LHS: The scalar to multiply with
     * @param RHS: The vector to multiply with
     * @return: A vector with the result of the multiplication
     */
    friend FORCEINLINE FVector3 operator*(float LHS, const FVector3& RHS) noexcept
    {
        return FVector3(LHS * RHS.x, LHS * RHS.y, LHS * RHS.z);
    }

    /**
     * @brief: Returns this vector after multiplying each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return: A reference to this vector
     */
    FORCEINLINE FVector3 operator*=(float RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief: Returns the result of component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return: A vector with the result of the division
     */
    FORCEINLINE FVector3 operator/(const FVector3& RHS) const noexcept
    {
        return FVector3(x / RHS.x, y / RHS.y, z / RHS.z);
    }

    /**
     * @brief: Returns this vector after component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return: A reference to this vector
     */
    FORCEINLINE FVector3& operator/=(const FVector3& RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief: Returns the result of dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return: A vector with the result of the division
     */
    FORCEINLINE FVector3 operator/(float RHS) const noexcept
    {
        return FVector3(x / RHS, y / RHS, z / RHS);
    }

    /**
     * @brief: Returns this vector after dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return: A reference to this vector
     */
    FORCEINLINE FVector3& operator/=(float RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief: Returns the result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return: True if equal, false if not
     */
    FORCEINLINE bool operator==(const FVector3& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief: Returns the negated result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return: False if equal, true if not
     */
    FORCEINLINE bool operator!=(const FVector3& Other) const noexcept
    {
        return !IsEqual(Other);
    }

    /**
     * @brief: Returns the component specified
     *
     * @param Index: The component index
     * @return: The component
     */
    FORCEINLINE float& operator[](int32 Index) noexcept
    {
        Check(Index < 3);
        return reinterpret_cast<float*>(this)[Index];
    }

    /**
     * @brief: Returns the component specified
     *
     * @param Index: The component index
     * @return: The component
     */
    FORCEINLINE float operator[](int32 Index) const noexcept
    {
        Check(Index < 3);
        return reinterpret_cast<const float*>(this)[Index];
    }

public:

     /** @brief: The x-coordinate */
    float x;

     /** @brief: The y-coordinate */
    float y;
    
    /** @brief: The z-coordinate */
    float z;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Radians and Degree conversion

namespace NMath
{
    template<>
    FORCEINLINE FVector3 ToDegrees(FVector3 Radians)
    {
        return FVector3(ToDegrees(Radians.x), ToDegrees(Radians.y), ToDegrees(Radians.z));
    }

    template<>
    FORCEINLINE FVector3 ToRadians(FVector3 Degrees)
    {
        return FVector3(ToRadians(Degrees.x), ToRadians(Degrees.y), ToRadians(Degrees.z));
    }
}
