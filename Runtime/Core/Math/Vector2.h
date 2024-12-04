#pragma once
#include "Core/Math/MathCommon.h"

class FVector2
{
public:

    /** 
     * @brief Default constructor (Initialize components to zero) 
     */
    FORCEINLINE FVector2() noexcept
        : X(0.0f)
        , Y(0.0f)
    {
    }

    /**
     * @brief Constructor initializing all components with a corresponding value.
     * @param InX - The X-coordinate
     * @param InY - The Y-coordinate
     */
    FORCEINLINE explicit FVector2(float InX, float InY) noexcept
        : X(InX)
        , Y(InY)
    {
    }

    /**
     * @brief Constructor initializing all components with a single value.
     * @param Scalar - Value to set all components to
     */
    FORCEINLINE explicit FVector2(float Scalar) noexcept
        : X(Scalar)
        , Y(Scalar)
    {
    }

     /** @brief Normalized this vector */
    inline void Normalize() noexcept
    {
        const float LengthSqrd = GetLengthSquared();
        if (LengthSqrd != 0.0f)
        {
            const float RcpLength = 1.0f / FMath::Sqrt(LengthSqrd);
            X = X * RcpLength;
            Y = Y * RcpLength;
        }
    }

    /**
     * @brief Returns a normalized version of this vector
     * @return A copy of this vector normalized
     */
    FORCEINLINE FVector2 GetNormalized() const noexcept
    {
        FVector2 Result(*this);
        Result.Normalize();
        return Result;
    }

    /**
     * @brief Compares, within a threshold Epsilon, this vector with another vector
     * @param Other vector to compare against
     * @return True if equal, false if not
     */
    inline bool IsEqual(const FVector2& Other, float Epsilon = FMath::kIsEqualEpsilon) const noexcept
    {
        Epsilon = FMath::Abs(Epsilon);

        for (int32 Index = 0; Index < 2; ++Index)
        {
            float Diff = reinterpret_cast<const float*>(this)[Index] - reinterpret_cast<const float*>(&Other)[Index];
            if (FMath::Abs(Diff) > Epsilon)
            {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Checks weather this vector is a unit vector not
     * @return True if the length equals one, false if not
     */
    FORCEINLINE bool IsUnitVector() const noexcept
    {
        const float LengthSqrd = FMath::Abs(1.0f - GetLengthSquared());
        return (LengthSqrd < FMath::kIsEqualEpsilon);
    }

    /**
     * @brief Checks weather this vector has any component that equals NaN
     * @return True if the any component equals NaN, false if not
     */
    FORCEINLINE bool ContainsNaN() const noexcept
    {
        for (int32 Index = 0; Index < 2; ++Index)
        {
            if (FMath::IsNaN(XY[Index]))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Checks weather this vector has any component that equals infinity
     * @return True if the any component equals infinity, false if not
     */
    FORCEINLINE bool ContainsInfinity() const noexcept
    {
        for (int32 Index = 0; Index < 2; ++Index)
        {
            if (FMath::IsInfinity(XY[Index]))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Checks weather this vector has any value that equals infinity or NaN
     * @return False if the any value equals infinity or NaN, true if not
     */
    FORCEINLINE bool IsValid() const noexcept
    {
        return !ContainsNaN() && !ContainsInfinity();
    }

    /**
     * @brief Returns the length of this vector
     * @return The length of the vector
     */
    FORCEINLINE float GetLength() const noexcept
    {
        const float LengthSqrd = GetLengthSquared();
        return FMath::Sqrt(LengthSqrd);
    }

    /**
     * @brief Returns the length of this vector squared
     * @return The length of the vector squared
     */
    FORCEINLINE float GetLengthSquared() const noexcept
    {
        return DotProduct(*this);
    }

    /**
     * @brief Returns the dot product between this and another vector
     * @param Other The vector to perform dot product with
     * @return The dot product
     */
    FORCEINLINE float DotProduct(const FVector2& Other) const noexcept
    {
        return (X * Other.X) + (Y * Other.Y);
    }

    /**
     * @brief Returns the resulting vector after projecting this vector onto another.
     * @param Other The vector to project onto
     * @return The projected vector
     */
    inline FVector2 ProjectOn(const FVector2& Other) const noexcept
    {
        float AdotB = DotProduct(Other);
        float BdotB = Other.DotProduct(Other);
        return (AdotB / BdotB) * Other;
    }

public:

    /**
     * @brief Returns a vector with the smallest of each component of two vectors
     * @param ValueA First vector to compare with
     * @param ValueB Second vector to compare with
     * @return A vector with the smallest components of First and Second
     */
    FORCEINLINE static FVector2 Min(const FVector2& ValueA, const FVector2& ValueB) noexcept
    {
        return FVector2(FMath::Min(ValueA.X, ValueB.X), FMath::Min(ValueA.Y, ValueB.Y));
    }

    /**
     * @brief Returns a vector with the largest of each component of two vectors
     * @param ValueA First vector to compare with
     * @param ValueB Second vector to compare with
     * @return A vector with the largest components of First and Second
     */
    FORCEINLINE static FVector2 Max(const FVector2& ValueA, const FVector2& ValueB) noexcept
    {
        return FVector2(FMath::Max(ValueA.X, ValueB.X), FMath::Max(ValueA.Y, ValueB.Y));
    }

    /**
     * @brief Returns the linear interpolation between two vectors
     * @param First First vector to interpolate
     * @param Second Second vector to interpolate
     * @param Factor Factor to interpolate with. Zero returns First, One returns seconds
     * @return A vector with the result of interpolation
     */
    FORCEINLINE static FVector2 Lerp(const FVector2& First, const FVector2& Second, float Factor) noexcept
    {
        return FVector2((1.0f - Factor) * First.X + Factor * Second.X, (1.0f - Factor) * First.Y + Factor * Second.Y);
    }

    /**
     * @brief Returns a vector with all the components within the range of a min and max value
     * @param Min Vector with minimum values
     * @param Max Vector with maximum values
     * @param Value Vector to clamp
     * @return A vector with the result of clamping
     */
    FORCEINLINE static FVector2 Clamp(const FVector2& Value, const FVector2& Min, const FVector2& Max) noexcept
    {
        return FVector2(FMath::Clamp(Value.X, Min.X, Max.X), FMath::Clamp(Value.Y, Min.Y, Max.Y));
    }

    /**
     * @brief Returns a vector with all the components within the range zero and one
     * @param Value Value to saturate
     * @return A vector with the result of saturation
     */
    FORCEINLINE static FVector2 Saturate(const FVector2& Value) noexcept
    {
        return FVector2(FMath::Saturate(Value.X), FMath::Saturate(Value.Y));
    }

    FORCEINLINE static FVector2 ToDegrees(FVector2 Radians)
    {
        return FVector2(FMath::ToDegrees(Radians.X), FMath::ToDegrees(Radians.Y));
    }

    FORCEINLINE static FVector2 ToRadians(FVector2 Degrees)
    {
        return FVector2(FMath::ToRadians(Degrees.X), FMath::ToRadians(Degrees.Y));
    }

public:

    /**
     * @brief Return a vector with component-wise negation of this vector
     * @return A negated vector
     */
    FORCEINLINE FVector2 operator-() const noexcept
    {
        return FVector2(-X, -Y);
    }

    /**
     * @brief Returns the result of component-wise adding this and another vector
     * @param RHS The vector to add
     * @return A vector with the result of addition
     */
    FORCEINLINE FVector2 operator+(const FVector2& RHS) const noexcept
    {
        return FVector2(X + RHS.X, Y + RHS.Y);
    }

    /**
     * @brief Returns this vector after component-wise adding this with another vector
     * @param RHS The vector to add
     * @return A reference to this vector
     */
    FORCEINLINE FVector2& operator+=(const FVector2& RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief Returns the result of adding a scalar to each component of this vector
     * @param RHS The scalar to add
     * @return A vector with the result of addition
     */
    FORCEINLINE FVector2 operator+(float RHS) const noexcept
    {
        return FVector2(X + RHS, Y + RHS);
    }

    /**
     * @brief Returns this vector after adding a scalar to each component of this vector
     * @param RHS The scalar to add
     * @return A reference to this vector
     */
    FORCEINLINE FVector2& operator+=(float RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief Returns the result of component-wise subtraction between this and another vector
     * @param RHS The vector to subtract
     * @return A vector with the result of subtraction
     */
    FORCEINLINE FVector2 operator-(const FVector2& RHS) const noexcept
    {
        return FVector2(X - RHS.X, Y - RHS.Y);
    }

    /**
     * @brief Returns this vector after component-wise subtraction between this and another vector
     * @param RHS The vector to subtract
     * @return A reference to this vector
     */
    FORCEINLINE FVector2& operator-=(const FVector2& RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief Returns the result of subtracting each component of this vector with a scalar
     * @param RHS The scalar to subtract
     * @return A vector with the result of the subtraction
     */
    FORCEINLINE FVector2 operator-(float RHS) const noexcept
    {
        return FVector2(X - RHS, Y - RHS);
    }

    /**
     * @brief Returns this vector after subtracting each component of this vector with a scalar
     * @param RHS The scalar to subtract
     * @return A reference to this vector
     */
    FORCEINLINE FVector2& operator-=(float RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief Returns the result of component-wise multiplication with this and another vector
     * @param RHS The vector to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE FVector2 operator*(const FVector2& RHS) const noexcept
    {
        return FVector2(X * RHS.X, Y * RHS.Y);
    }

    /**
     * @brief Returns this vector after component-wise multiplication with this and another vector
     * @param RHS The vector to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE FVector2& operator*=(const FVector2& RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief Returns the result of multiplying each component of this vector with a scalar
     * @param RHS The scalar to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE FVector2 operator*(float RHS) const noexcept
    {
        return FVector2(X * RHS, Y * RHS);
    }

    /**
     * @brief Returns the result of multiplying each component of a vector with a scalar
     * @param LHS The scalar to multiply with
     * @param RHS The vector to multiply with
     * @return A vector with the result of the multiplication
     */
    friend FORCEINLINE FVector2 operator*(float LHS, const FVector2& RHS) noexcept
    {
        return FVector2(LHS * RHS.X, LHS * RHS.Y);
    }

    /**
     * @brief Returns this vector after multiplying each component of this vector with a scalar
     * @param RHS The scalar to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE FVector2 operator*=(float RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief Returns the result of component-wise division with this and another vector
     * @param RHS The vector to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE FVector2 operator/(const FVector2& RHS) const noexcept
    {
        return FVector2(X / RHS.X, Y / RHS.Y);
    }

    /**
     * @brief Returns this vector after component-wise division with this and another vector
     * @param RHS The vector to divide with
     * @return A reference to this vector
     */
    FORCEINLINE FVector2& operator/=(const FVector2& RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief Returns the result of dividing each component of this vector and a scalar
     * @param RHS The scalar to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE FVector2 operator/(float RHS) const noexcept
    {
        return FVector2(X / RHS, Y / RHS);
    }


    /**
     * @brief Returns this vector after dividing each component of this vector and a scalar
     * @param RHS The scalar to divide with
     * @return A reference to this vector
     */
    FORCEINLINE FVector2& operator/=(float RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief Returns the result after comparing this and another vector
     * @param Other The vector to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==(const FVector2& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief Returns the negated result after comparing this and another vector
     * @param Other The vector to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=(const FVector2& Other) const noexcept
    {
        return !IsEqual(Other);
    }

    /**
     * @brief Returns the component specified
     * @param Index The component index
     * @return The component
     */
    FORCEINLINE float& operator[](int32 Index) noexcept
    {
        CHECK(Index < 2);
        return XY[Index];
    }

    /**
     * @brief Returns the component specified
     * @param Index The component index
     * @return The component
     */
    FORCEINLINE float operator[](int32 Index) const noexcept
    {
        CHECK(Index < 2);
        return XY[Index];
    }

public:

    union
    {
        struct 
        {
            /** @brief The X-coordinate */
            float X;

            /** @brief The Y-coordinate */
            float Y;
        };

        float XY[2];
    };
    
};

MARK_AS_REALLOCATABLE(FVector2);
