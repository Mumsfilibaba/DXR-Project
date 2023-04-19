#pragma once
#include "MathCommon.h"

class FVector2
{
public:

    /** 
     * @brief - Default constructor (Initialize components to zero) 
     */
    FORCEINLINE FVector2() noexcept
        : x(0.0f)
        , y(0.0f)
    {
    }

    /**
     * @brief     - Constructor initializing all components with a corresponding value.
     * @param InX - The x-coordinate
     * @param InY - The y-coordinate
     */
    FORCEINLINE explicit FVector2(float InX, float InY) noexcept
        : x(InX)
        , y(InY)
    {
    }

    /**
     * @brief     - Constructor initializing all components with an array.
     * @param Arr - Array with 2 elements
     */
    FORCEINLINE explicit FVector2(const float* Arr) noexcept
        : x(Arr[0])
        , y(Arr[1])
    {
    }

    /**
     * @brief        - Constructor initializing all components with a single value.
     * @param Scalar - Value to set all components to
     */
    FORCEINLINE explicit FVector2(float Scalar) noexcept
        : x(Scalar)
        , y(Scalar)
    {
    }

     /** @brief - Normalized this vector */
    inline void Normalize() noexcept
    {
        const float fLengthSquared = LengthSquared();
        if (fLengthSquared != 0.0f)
        {
            const float fRecipLength = 1.0f / NMath::Sqrt(fLengthSquared);
            x = x * fRecipLength;
            y = y * fRecipLength;
        }
    }

    /**
     * @brief  - Returns a normalized version of this vector
     * @return - A copy of this vector normalized
     */
    FORCEINLINE FVector2 GetNormalized() const noexcept
    {
        FVector2 Result(*this);
        Result.Normalize();
        return Result;
    }

    /**
     * @brief       - Compares, within a threshold Epsilon, this vector with another vector
     * @param Other - vector to compare against
     * @return      - True if equal, false if not
     */
    inline bool IsEqual(const FVector2& Other, float Epsilon = NMath::kIsEqualEpsilon) const noexcept
    {
        Epsilon = NMath::Abs(Epsilon);

        for (int32 Index = 0; Index < 2; ++Index)
        {
            float Diff = reinterpret_cast<const float*>(this)[Index] - reinterpret_cast<const float*>(&Other)[Index];
            if (NMath::Abs(Diff) > Epsilon)
            {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief  - Checks weather this vector is a unit vector not
     * @return - True if the length equals one, false if not
     */
    FORCEINLINE bool IsUnitVector() const noexcept
    {
        const float fLengthSquared = NMath::Abs(1.0f - LengthSquared());
        return (fLengthSquared < NMath::kIsEqualEpsilon);
    }

    /**
     * @brief  - Checks weather this vector has any component that equals NaN
     * @return - True if the any component equals NaN, false if not
     */
    FORCEINLINE bool HasNaN() const noexcept
    {
        for (int32 Index = 0; Index < 2; ++Index)
        {
            if (NMath::IsNaN(reinterpret_cast<const float*>(this)[Index]))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief  - Checks weather this vector has any component that equals infinity
     * @return - True if the any component equals infinity, false if not
     */
    FORCEINLINE bool HasInfinity() const noexcept
    {
        for (int32 Index = 0; Index < 2; ++Index)
        {
            if (NMath::IsInfinity(reinterpret_cast<const float*>(this)[Index]))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief  - Checks weather this vector has any value that equals infinity or NaN
     * @return - False if the any value equals infinity or NaN, true if not
     */
    FORCEINLINE bool IsValid() const noexcept
    {
        return !HasNaN() && !HasInfinity();
    }

    /**
     * @brief  - Returns the length of this vector
     * @return - The length of the vector
     */
    FORCEINLINE float GetLength() const noexcept
    {
        const float fLengthSquared = LengthSquared();
        return NMath::Sqrt(fLengthSquared);
    }

    /**
     * @brief  - Returns the length of this vector squared
     * @return - The length of the vector squared
     */
    FORCEINLINE float LengthSquared() const noexcept
    {
        return DotProduct(*this);
    }

    /**
     * @brief       - Returns the dot product between this and another vector
     * @param Other - The vector to perform dot product with
     * @return      - The dot product
     */
    FORCEINLINE float DotProduct(const FVector2& Other) const noexcept
    {
        return (x * Other.x) + (y * Other.y);
    }

    /**
     * @brief       - Returns the resulting vector after projecting this vector onto another.
     * @param Other - The vector to project onto
     * @return      - The projected vector
     */
    inline FVector2 ProjectOn(const FVector2& Other) const noexcept
    {
        float AdotB = DotProduct(Other);
        float BdotB = Other.LengthSquared();
        return (AdotB / BdotB) * Other;
    }

    /**
     * @brief  - Returns the data of this matrix as a pointer
     * @return - A pointer to the data
     */
    FORCEINLINE float* Data() noexcept
    {
        return reinterpret_cast<float*>(this);
    }

    /**
     * @brief  - Returns the data of this matrix as a pointer
     * @return - A pointer to the data
     */
    FORCEINLINE const float* Data() const noexcept
    {
        return reinterpret_cast<const float*>(this);
    }

public:

    /**
     * @brief        - Returns a vector with the smallest of each component of two vectors
     * @param First  - First vector to compare with
     * @param Second - Second vector to compare with
     * @return       - A vector with the smallest components of First and Second
     */
    friend FORCEINLINE FVector2 Min(const FVector2& First, const FVector2& Second) noexcept
    {
        return FVector2(NMath::Min(First.x, Second.x), NMath::Min(First.y, Second.y));
    }

    /**
     * @brief        - Returns a vector with the largest of each component of two vectors
     * @param First  - First vector to compare with
     * @param Second - Second vector to compare with
     * @return       - A vector with the largest components of First and Second
     */
    friend FORCEINLINE FVector2 Max(const FVector2& First, const FVector2& Second) noexcept
    {
        return FVector2(NMath::Max(First.x, Second.x), NMath::Max(First.y, Second.y));
    }

    /**
     * @brief        - Returns the linear interpolation between two vectors
     * @param First  - First vector to interpolate
     * @param Second - Second vector to interpolate
     * @param Factor - Factor to interpolate with. Zero returns First, One returns seconds
     * @return       - A vector with the result of interpolation
     */
    friend FORCEINLINE FVector2 Lerp(const FVector2& First, const FVector2& Second, float Factor) noexcept
    {
        return FVector2((1.0f - Factor) * First.x + Factor * Second.x, (1.0f - Factor) * First.y + Factor * Second.y);
    }

    /**
     * @brief       - Returns a vector with all the components within the range of a min and max value
     * @param Min   - Vector with minimum values
     * @param Max   - Vector with maximum values
     * @param Value - Vector to clamp
     * @return      - A vector with the result of clamping
     */
    friend FORCEINLINE FVector2 Clamp(const FVector2& Min, const FVector2& Max, const FVector2& Value) noexcept
    {
        return FVector2(NMath::Min(NMath::Max(Value.x, Min.x), Max.x), NMath::Min(NMath::Max(Value.y, Min.y), Max.y));
    }

    /**
     * @brief       - Returns a vector with all the components within the range zero and one
     * @param Value - Value to saturate
     * @return      - A vector with the result of saturation
     */
    friend FORCEINLINE FVector2 Saturate(const FVector2& Value) noexcept
    {
        return FVector2(NMath::Min(NMath::Max(Value.x, 0.0f), 1.0f), NMath::Min(NMath::Max(Value.y, 0.0f), 1.0f));
    }

public:

    /**
     * @brief  - Return a vector with component-wise negation of this vector
     * @return - A negated vector
     */
    FORCEINLINE FVector2 operator-() const noexcept
    {
        return FVector2(-x, -y);
    }

    /**
     * @brief     - Returns the result of component-wise adding this and another vector
     * @param RHS - The vector to add
     * @return    - A vector with the result of addition
     */
    FORCEINLINE FVector2 operator+(const FVector2& RHS) const noexcept
    {
        return FVector2(x + RHS.x, y + RHS.y);
    }

    /**
     * @brief     - Returns this vector after component-wise adding this with another vector
     * @param RHS - The vector to add
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector2& operator+=(const FVector2& RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief     - Returns the result of adding a scalar to each component of this vector
     * @param RHS - The scalar to add
     * @return    - A vector with the result of addition
     */
    FORCEINLINE FVector2 operator+(float RHS) const noexcept
    {
        return FVector2(x + RHS, y + RHS);
    }

    /**
     * @brief     - Returns this vector after adding a scalar to each component of this vector
     * @param RHS - The scalar to add
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector2& operator+=(float RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief     - Returns the result of component-wise subtraction between this and another vector
     * @param RHS - The vector to subtract
     * @return    - A vector with the result of subtraction
     */
    FORCEINLINE FVector2 operator-(const FVector2& RHS) const noexcept
    {
        return FVector2(x - RHS.x, y - RHS.y);
    }

    /**
     * @brief     - Returns this vector after component-wise subtraction between this and another vector
     * @param RHS - The vector to subtract
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector2& operator-=(const FVector2& RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief     - Returns the result of subtracting each component of this vector with a scalar
     * @param RHS - The scalar to subtract
     * @return    - A vector with the result of the subtraction
     */
    FORCEINLINE FVector2 operator-(float RHS) const noexcept
    {
        return FVector2(x - RHS, y - RHS);
    }

    /**
     * @brief     - Returns this vector after subtracting each component of this vector with a scalar
     * @param RHS - The scalar to subtract
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector2& operator-=(float RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief     - Returns the result of component-wise multiplication with this and another vector
     * @param RHS - The vector to multiply with
     * @return    - A vector with the result of the multiplication
     */
    FORCEINLINE FVector2 operator*(const FVector2& RHS) const noexcept
    {
        return FVector2(x * RHS.x, y * RHS.y);
    }

    /**
     * @brief     - Returns this vector after component-wise multiplication with this and another vector
     * @param RHS - The vector to multiply with
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector2& operator*=(const FVector2& RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief     - Returns the result of multiplying each component of this vector with a scalar
     * @param RHS - The scalar to multiply with
     * @return    - A vector with the result of the multiplication
     */
    FORCEINLINE FVector2 operator*(float RHS) const noexcept
    {
        return FVector2(x * RHS, y * RHS);
    }

    /**
     * @brief     - Returns the result of multiplying each component of a vector with a scalar
     * @param LHS - The scalar to multiply with
     * @param RHS - The vector to multiply with
     * @return    - A vector with the result of the multiplication
     */
    friend FORCEINLINE FVector2 operator*(float LHS, const FVector2& RHS) noexcept
    {
        return FVector2(LHS * RHS.x, LHS * RHS.y);
    }

    /**
     * @brief     - Returns this vector after multiplying each component of this vector with a scalar
     * @param RHS - The scalar to multiply with
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector2 operator*=(float RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief     - Returns the result of component-wise division with this and another vector
     * @param RHS - The vector to divide with
     * @return    - A vector with the result of the division
     */
    FORCEINLINE FVector2 operator/(const FVector2& RHS) const noexcept
    {
        return FVector2(x / RHS.x, y / RHS.y);
    }

    /**
     * @brief     - Returns this vector after component-wise division with this and another vector
     * @param RHS - The vector to divide with
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector2& operator/=(const FVector2& RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief     - Returns the result of dividing each component of this vector and a scalar
     * @param RHS - The scalar to divide with
     * @return    - A vector with the result of the division
     */
    FORCEINLINE FVector2 operator/(float RHS) const noexcept
    {
        return FVector2(x / RHS, y / RHS);
    }


    /**
     * @brief     - Returns this vector after dividing each component of this vector and a scalar
     * @param RHS - The scalar to divide with
     * @return    - A reference to this vector
     */
    FORCEINLINE FVector2& operator/=(float RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief       - Returns the result after comparing this and another vector
     * @param Other - The vector to compare with
     * @return      - True if equal, false if not
     */
    FORCEINLINE bool operator==(const FVector2& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief       - Returns the negated result after comparing this and another vector
     * @param Other - The vector to compare with
     * @return      - False if equal, true if not
     */
    FORCEINLINE bool operator!=(const FVector2& Other) const noexcept
    {
        return !IsEqual(Other);
    }

    /**
     * @brief       - Returns the component specified
     * @param Index - The component index
     * @return      - The component
     */
    FORCEINLINE float& operator[](int32 Index) noexcept
    {
        CHECK(Index < 2);
        return reinterpret_cast<float*>(this)[Index];
    }

    /**
     * @brief       - Returns the component specified
     * @param Index - The component index
     * @return      - The component
     */
    FORCEINLINE float operator[](int32 Index) const noexcept
    {
        CHECK(Index < 2);
        return reinterpret_cast<const float*>(this)[Index];
    }

public:

     /** @brief - The x-coordinate */
    float x;

    /** @brief - The y-coordinate */
    float y;
};

MARK_AS_REALLOCATABLE(FVector2);

namespace NMath
{
    template<>
    FORCEINLINE FVector2 ToDegrees(FVector2 Radians)
    {
        return FVector2(ToDegrees(Radians.x), ToDegrees(Radians.y));
    }

    template<>
    FORCEINLINE FVector2 ToRadians(FVector2 Degrees)
    {
        return FVector2(ToRadians(Degrees.x), ToRadians(Degrees.y));
    }
}