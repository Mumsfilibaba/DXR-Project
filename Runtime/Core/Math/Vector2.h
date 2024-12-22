#pragma once
#include "Core/Math/MathCommon.h"

class FVector2
{
public:

    /**
     * @brief Default constructor (initializes components to zero).
     */
    FORCEINLINE FVector2() noexcept
        : X(0.0f)
        , Y(0.0f)
    {
    }

    /**
     * @brief Constructor initializing all components with specific values.
     * @param InX The X-coordinate.
     * @param InY The Y-coordinate.
     */
    FORCEINLINE explicit FVector2(float InX, float InY) noexcept
        : X(InX)
        , Y(InY)
    {
    }

    /**
     * @brief Constructor initializing all components with a single value.
     * @param Scalar Value to set all components to.
     */
    FORCEINLINE explicit FVector2(float Scalar) noexcept
        : X(Scalar)
        , Y(Scalar)
    {
    }

    /**
     * @brief Normalizes this vector.
     */
    inline void Normalize() noexcept
    {
        const float LengthSqrd = GetLengthSquared();
        if (LengthSqrd != 0.0f)
        {
            const float RcpLength = 1.0f / FMath::Sqrt(LengthSqrd);
            X *= RcpLength;
            Y *= RcpLength;
        }
    }

    /**
     * @brief Returns a normalized version of this vector.
     * @return A normalized copy of this vector.
     */
    FORCEINLINE FVector2 GetNormalized() const noexcept
    {
        FVector2 Result(*this);
        Result.Normalize();
        return Result;
    }

    /**
     * @brief Returns a vector that is perpendicular to this vector.
     * @return The perpendicular vector.
     */
    FORCEINLINE FVector2 GetPerpendicular() const noexcept
    {
        return FVector2(-Y, X);
    }

    /**
     * @brief Returns a rotated version of this vector by a specified angle.
     * @param AngleRadians The angle to rotate by, in radians.
     * @return The rotated vector.
     */
    FORCEINLINE FVector2 GetRotated(float AngleRadians) const noexcept
    {
        float CosAngle = FMath::Cos(AngleRadians);
        float SinAngle = FMath::Sin(AngleRadians);

        return FVector2(X * CosAngle - Y * SinAngle, X * SinAngle + Y * CosAngle);
    }

    /**
     * @brief Calculates the distance to another vector.
     * @param Other The other vector.
     * @return The distance between this vector and the other vector.
     */
    FORCEINLINE float GetDistanceTo(const FVector2& Other) const noexcept
    {
        return (*this - Other).GetLength();
    }

    /**
     * @brief Calculates the squared distance to another vector.
     * @param Other The other vector.
     * @return The squared distance between this vector and the other vector.
     */
    FORCEINLINE float GetDistanceSquaredTo(const FVector2& Other) const noexcept
    {
        return (*this - Other).GetLengthSquared();
    }

    /**
     * @brief Calculates the angle between this vector and another vector.
     * @param Other The other vector.
     * @return The angle in radians between the two vectors.
     */
    FORCEINLINE float GetAngleBetween(const FVector2& Other) const noexcept
    {
        float Dot     = DotProduct(Other);
        float Lengths = GetLength() * Other.GetLength();

        // Prevent division by zero
        if (Lengths == 0.0f)
        {
            return 0.0f;
        }

        float CosTheta = FMath::Clamp(Dot / Lengths, -1.0f, 1.0f);
        return FMath::Acos(CosTheta);
    }

    /**
     * @brief Reflects this vector around a normal vector.
     * @param Normal The normal vector to reflect around (should be normalized).
     * @return The reflected vector.
     */
    FORCEINLINE FVector2 GetReflected(const FVector2& Normal) const noexcept
    {
        return *this - 2.0f * DotProduct(Normal) * Normal;
    }

    /**
     * @brief Compares this vector with another vector within a specified threshold.
     * @param Other Vector to compare against.
     * @param Epsilon The threshold for comparison.
     * @return True if vectors are approximately equal, false otherwise.
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
     * @brief Checks whether this vector is a unit vector.
     * @return True if the length equals one, false otherwise.
     */
    FORCEINLINE bool IsUnitVector() const noexcept
    {
        const float LengthDiff = FMath::Abs(1.0f - GetLengthSquared());
        return (LengthDiff < FMath::kIsEqualEpsilon);
    }

    /**
     * @brief Checks whether this vector contains any NaN components.
     * @return True if any component equals NaN, false otherwise.
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
     * @brief Checks whether this vector contains any infinite components.
     * @return True if any component equals infinity, false otherwise.
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
     * @brief Returns the length of this vector.
     * @return The length of the vector.
     */
    FORCEINLINE float GetLength() const noexcept
    {
        const float LengthSqrd = GetLengthSquared();
        return FMath::Sqrt(LengthSqrd);
    }

    /**
     * @brief Returns the squared length of this vector.
     * @return The squared length of the vector.
     */
    FORCEINLINE float GetLengthSquared() const noexcept
    {
        return DotProduct(*this);
    }

    /**
     * @brief Calculates the dot product between this vector and another vector.
     * @param Other The vector to perform the dot product with.
     * @return The dot product.
     */
    FORCEINLINE float DotProduct(const FVector2& Other) const noexcept
    {
        return (X * Other.X) + (Y * Other.Y);
    }

    /**
     * @brief Projects this vector onto another vector.
     * @param Other The vector to project onto.
     * @return The projected vector.
     */
    FORCEINLINE FVector2 ProjectOn(const FVector2& Other) const noexcept
    {
        float AdotB = DotProduct(Other);
        float BdotB = Other.DotProduct(Other);

        // Prevent division by zero
        if (BdotB == 0.0f)
        {
            return FVector2(0.0f, 0.0f);
        }

        return (AdotB / BdotB) * Other;
    }

public:

    /**
     * @brief Returns a vector with the smallest components of two vectors.
     * @param ValueA First vector to compare.
     * @param ValueB Second vector to compare.
     * @return A vector with the smallest components.
     */
    static FORCEINLINE FVector2 Min(const FVector2& ValueA, const FVector2& ValueB) noexcept
    {
        return FVector2(FMath::Min(ValueA.X, ValueB.X), FMath::Min(ValueA.Y, ValueB.Y));
    }

    /**
     * @brief Returns a vector with the largest components of two vectors.
     * @param ValueA First vector to compare.
     * @param ValueB Second vector to compare.
     * @return A vector with the largest components.
     */
    static FORCEINLINE FVector2 Max(const FVector2& ValueA, const FVector2& ValueB) noexcept
    {
        return FVector2(FMath::Max(ValueA.X, ValueB.X), FMath::Max(ValueA.Y, ValueB.Y));
    }

    /**
     * @brief Performs linear interpolation between two vectors.
     * @param ValueA First vector.
     * @param ValueB Second vector.
     * @param Factor Interpolation factor (0 returns ValueA, 1 returns ValueB).
     * @return The interpolated vector.
     */
    static FORCEINLINE FVector2 Lerp(const FVector2& ValueA, const FVector2& ValueB, float Factor) noexcept
    {
        return FVector2((1.0f - Factor) * ValueA.X + Factor * ValueB.X, (1.0f - Factor) * ValueA.Y + Factor * ValueB.Y);
    }

    /**
     * @brief Clamps the components of a vector within specified ranges.
     * @param Value The vector to clamp.
     * @param Min Vector containing the minimum values.
     * @param Max Vector containing the maximum values.
     * @return The clamped vector.
     */
    static FORCEINLINE FVector2 Clamp(const FVector2& Value, const FVector2& Min, const FVector2& Max) noexcept
    {
        return FVector2(FMath::Clamp(Value.X, Min.X, Max.X), FMath::Clamp(Value.Y, Min.Y, Max.Y));
    }

    /**
     * @brief Saturates the components of a vector to the range [0, 1].
     * @param Value The vector to saturate.
     * @return The saturated vector.
     */
    static FORCEINLINE FVector2 Saturate(const FVector2& Value) noexcept
    {
        return FVector2(FMath::Saturate(Value.X), FMath::Saturate(Value.Y));
    }

    /**
     * @brief Converts vector components from radians to degrees.
     * @param Radians Vector in radians.
     * @return Vector with components in degrees.
     */
    static FORCEINLINE FVector2 ToDegrees(const FVector2& Radians) noexcept
    {
        return FVector2(FMath::ToDegrees(Radians.X), FMath::ToDegrees(Radians.Y));
    }

    /**
     * @brief Converts vector components from degrees to radians.
     * @param Degrees Vector in degrees.
     * @return Vector with components in radians.
     */
    static FORCEINLINE FVector2 ToRadians(const FVector2& Degrees) noexcept
    {
        return FVector2(FMath::ToRadians(Degrees.X), FMath::ToRadians(Degrees.Y));
    }

public:

    /**
     * @brief Returns a vector with negated components.
     * @return The negated vector.
     */
    FORCEINLINE FVector2 operator-() const noexcept
    {
        return FVector2(-X, -Y);
    }

    /**
     * @brief Adds two vectors component-wise.
     * @param RHS The vector to add.
     * @return The result of the addition.
     */
    FORCEINLINE FVector2 operator+(const FVector2& RHS) const noexcept
    {
        return FVector2(X + RHS.X, Y + RHS.Y);
    }

    /**
     * @brief Adds another vector to this vector component-wise.
     * @param RHS The vector to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE FVector2& operator+=(const FVector2& RHS) noexcept
    {
        X += RHS.X;
        Y += RHS.Y;
        return *this;
    }

    /**
     * @brief Adds a scalar to each component of this vector.
     * @param RHS The scalar to add.
     * @return The result of the addition.
     */
    FORCEINLINE FVector2 operator+(float RHS) const noexcept
    {
        return FVector2(X + RHS, Y + RHS);
    }

    /**
     * @brief Adds a scalar to each component of this vector.
     * @param RHS The scalar to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE FVector2& operator+=(float RHS) noexcept
    {
        X += RHS;
        Y += RHS;
        return *this;
    }

    /**
     * @brief Subtracts another vector from this vector component-wise.
     * @param RHS The vector to subtract.
     * @return The result of the subtraction.
     */
    FORCEINLINE FVector2 operator-(const FVector2& RHS) const noexcept
    {
        return FVector2(X - RHS.X, Y - RHS.Y);
    }

    /**
     * @brief Subtracts another vector from this vector component-wise.
     * @param RHS The vector to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE FVector2& operator-=(const FVector2& RHS) noexcept
    {
        X -= RHS.X;
        Y -= RHS.Y;
        return *this;
    }

    /**
     * @brief Subtracts a scalar from each component of this vector.
     * @param RHS The scalar to subtract.
     * @return The result of the subtraction.
     */
    FORCEINLINE FVector2 operator-(float RHS) const noexcept
    {
        return FVector2(X - RHS, Y - RHS);
    }

    /**
     * @brief Subtracts a scalar from each component of this vector.
     * @param RHS The scalar to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE FVector2& operator-=(float RHS) noexcept
    {
        X -= RHS;
        Y -= RHS;
        return *this;
    }

    /**
     * @brief Multiplies this vector with another vector component-wise.
     * @param RHS The vector to multiply with.
     * @return The result of the multiplication.
     */
    FORCEINLINE FVector2 operator*(const FVector2& RHS) const noexcept
    {
        return FVector2(X * RHS.X, Y * RHS.Y);
    }

    /**
     * @brief Multiplies this vector with another vector component-wise.
     * @param RHS The vector to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE FVector2& operator*=(const FVector2& RHS) noexcept
    {
        X *= RHS.X;
        Y *= RHS.Y;
        return *this;
    }

    /**
     * @brief Multiplies each component of this vector by a scalar.
     * @param RHS The scalar to multiply with.
     * @return The result of the multiplication.
     */
    FORCEINLINE FVector2 operator*(float RHS) const noexcept
    {
        return FVector2(X * RHS, Y * RHS);
    }

    /**
     * @brief Multiplies each component of this vector by a scalar.
     * @param RHS The scalar to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE FVector2& operator*=(float RHS) noexcept
    {
        X *= RHS;
        Y *= RHS;
        return *this;
    }

    /**
     * @brief Multiplies a scalar with a vector (friend function).
     * @param LHS The scalar to multiply.
     * @param RHS The vector to multiply with.
     * @return The result of the multiplication.
     */
    friend FORCEINLINE FVector2 operator*(float LHS, const FVector2& RHS) noexcept
    {
        return FVector2(LHS * RHS.X, LHS * RHS.Y);
    }

    /**
     * @brief Divides this vector by another vector component-wise.
     * @param RHS The vector to divide by.
     * @return The result of the division.
     */
    FORCEINLINE FVector2 operator/(const FVector2& RHS) const noexcept
    {
        return FVector2(X / RHS.X, Y / RHS.Y);
    }

    /**
     * @brief Divides this vector by another vector component-wise.
     * @param RHS The vector to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE FVector2& operator/=(const FVector2& RHS) noexcept
    {
        X /= RHS.X;
        Y /= RHS.Y;
        return *this;
    }

    /**
     * @brief Divides each component of this vector by a scalar.
     * @param RHS The scalar to divide by.
     * @return The result of the division.
     */
    FORCEINLINE FVector2 operator/(float RHS) const noexcept
    {
        return FVector2(X / RHS, Y / RHS);
    }

    /**
     * @brief Divides each component of this vector by a scalar.
     * @param RHS The scalar to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE FVector2& operator/=(float RHS) noexcept
    {
        X /= RHS;
        Y /= RHS;
        return *this;
    }

    /**
     * @brief Checks if this vector is equal to another vector.
     * @param Other The vector to compare with.
     * @return True if equal, false otherwise.
     */
    FORCEINLINE bool operator==(const FVector2& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief Checks if this vector is not equal to another vector.
     * @param Other The vector to compare with.
     * @return True if not equal, false otherwise.
     */
    FORCEINLINE bool operator!=(const FVector2& Other) const noexcept
    {
        return !IsEqual(Other);
    }

public:

    /**
     * @brief Accesses a component of the vector by index.
     * @param Index The component index (0 for X, 1 for Y).
     * @return Reference to the component.
     */
    FORCEINLINE float& operator[](int32 Index) noexcept
    {
        CHECK(Index >= 0 && Index < 2);
        return XY[Index];
    }

    /**
     * @brief Accesses a component of the vector by index.
     * @param Index The component index (0 for X, 1 for Y).
     * @return The component value.
     */
    FORCEINLINE float operator[](int32 Index) const noexcept
    {
        CHECK(Index >= 0 && Index < 2);
        return XY[Index];
    }

public:

    union
    {
        struct 
        {
            /** @brief The X-coordinate. */
            float X;

            /** @brief The Y-coordinate. */
            float Y;
        };

        /** @brief An array containing the X and Y components. */
        float XY[2];
    };
};

MARK_AS_REALLOCATABLE(FVector2);
