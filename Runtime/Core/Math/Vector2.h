#pragma once
#include "Core/Math/Math.h"

class Vector2
{
public:

    /**
     * @brief Default constructor (initializes components to zero).
     */
    FORCEINLINE Vector2() noexcept
        : X(0.0f)
        , Y(0.0f)
    {
    }

    /**
     * @brief Constructor initializing all components with specific values.
     * @param InX The X-coordinate.
     * @param InY The Y-coordinate.
     */
    FORCEINLINE explicit Vector2(float InX, float InY) noexcept
        : X(InX)
        , Y(InY)
    {
    }

    /**
     * @brief Constructor initializing all components with a single value.
     * @param Scalar Value to set all components to.
     */
    FORCEINLINE explicit Vector2(float Scalar) noexcept
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
            const float RcpLength = 1.0f / Math::Sqrt(LengthSqrd);
            X *= RcpLength;
            Y *= RcpLength;
        }
    }

    /**
     * @brief Returns a normalized version of this vector.
     * @return A normalized copy of this vector.
     */
    FORCEINLINE Vector2 GetNormalized() const noexcept
    {
        Vector2 Result(*this);
        Result.Normalize();
        return Result;
    }

    /**
     * @brief Returns a vector that is perpendicular to this vector.
     * @return The perpendicular vector.
     */
    FORCEINLINE Vector2 GetPerpendicular() const noexcept
    {
        return Vector2(-Y, X);
    }

    /**
     * @brief Returns a rotated version of this vector by a specified angle.
     * @param AngleRadians The angle to rotate by, in radians.
     * @return The rotated vector.
     */
    FORCEINLINE Vector2 GetRotated(float AngleRadians) const noexcept
    {
        float CosAngle = Math::Cos(AngleRadians);
        float SinAngle = Math::Sin(AngleRadians);

        return Vector2(X * CosAngle - Y * SinAngle, X * SinAngle + Y * CosAngle);
    }

    /**
     * @brief Calculates the distance to another vector.
     * @param Other The other vector.
     * @return The distance between this vector and the other vector.
     */
    FORCEINLINE float GetDistanceTo(const Vector2& Other) const noexcept
    {
        return (*this - Other).GetLength();
    }

    /**
     * @brief Calculates the squared distance to another vector.
     * @param Other The other vector.
     * @return The squared distance between this vector and the other vector.
     */
    FORCEINLINE float GetDistanceSquaredTo(const Vector2& Other) const noexcept
    {
        return (*this - Other).GetLengthSquared();
    }

    /**
     * @brief Calculates the angle between this vector and another vector.
     * @param Other The other vector.
     * @return The angle in radians between the two vectors.
     */
    FORCEINLINE float GetAngleBetween(const Vector2& Other) const noexcept
    {
        float Dot     = DotProduct(Other);
        float Lengths = GetLength() * Other.GetLength();

        // Prevent division by zero
        if (Lengths == 0.0f)
        {
            return 0.0f;
        }

        float CosTheta = Math::Clamp(Dot / Lengths, -1.0f, 1.0f);
        return Math::Acos(CosTheta);
    }

    /**
     * @brief Reflects this vector around a normal vector.
     * @param Normal The normal vector to reflect around (should be normalized).
     * @return The reflected vector.
     */
    FORCEINLINE Vector2 GetReflected(const Vector2& Normal) const noexcept
    {
        return *this - 2.0f * DotProduct(Normal) * Normal;
    }

    /**
     * @brief Compares this vector with another vector within a specified threshold.
     * @param Other Vector to compare against.
     * @param Threshold The threshold for comparison.
     * @return True if vectors are approximately equal, false otherwise.
     */
    inline bool IsEqual(const Vector2& Other, float Threshold = Math::Constants::CmpThreshold) const noexcept
    {
        Threshold = Math::Abs(Threshold);

        for (int32 Index = 0; Index < 2; ++Index)
        {
            float Diff = XY[Index] - Other.XY[Index];
            if (Math::Abs(Diff) > Threshold)
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
        const float LengthDiff = Math::Abs(1.0f - GetLengthSquared());
        return (LengthDiff < Math::Constants::CmpThreshold);
    }

    /**
     * @brief Checks whether this vector contains any NaN components.
     * @return True if any component equals NaN, false otherwise.
     */
    FORCEINLINE bool ContainsNaN() const noexcept
    {
        for (int32 Index = 0; Index < 2; ++Index)
        {
            if (Math::IsNaN(XY[Index]))
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
            if (Math::IsInfinity(XY[Index]))
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
        return Math::Sqrt(LengthSqrd);
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
    FORCEINLINE float DotProduct(const Vector2& Other) const noexcept
    {
        return (X * Other.X) + (Y * Other.Y);
    }

    /**
     * @brief Projects this vector onto another vector.
     * @param Other The vector to project onto.
     * @return The projected vector.
     */
    FORCEINLINE Vector2 ProjectOn(const Vector2& Other) const noexcept
    {
        float AdotB = DotProduct(Other);
        float BdotB = Other.DotProduct(Other);

        // Prevent division by zero
        if (BdotB == 0.0f)
        {
            return Vector2(0.0f, 0.0f);
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
    static FORCEINLINE Vector2 Min(const Vector2& ValueA, const Vector2& ValueB) noexcept
    {
        return Vector2(Math::Min(ValueA.X, ValueB.X), Math::Min(ValueA.Y, ValueB.Y));
    }

    /**
     * @brief Returns a vector with the largest components of two vectors.
     * @param ValueA First vector to compare.
     * @param ValueB Second vector to compare.
     * @return A vector with the largest components.
     */
    static FORCEINLINE Vector2 Max(const Vector2& ValueA, const Vector2& ValueB) noexcept
    {
        return Vector2(Math::Max(ValueA.X, ValueB.X), Math::Max(ValueA.Y, ValueB.Y));
    }

    /**
     * @brief Performs linear interpolation between two vectors.
     * @param ValueA First vector.
     * @param ValueB Second vector.
     * @param Factor Interpolation factor (0 returns ValueA, 1 returns ValueB).
     * @return The interpolated vector.
     */
    static FORCEINLINE Vector2 Lerp(const Vector2& ValueA, const Vector2& ValueB, float Factor) noexcept
    {
        return Vector2((1.0f - Factor) * ValueA.X + Factor * ValueB.X, (1.0f - Factor) * ValueA.Y + Factor * ValueB.Y);
    }

    /**
     * @brief Clamps the components of a vector within specified ranges.
     * @param Value The vector to clamp.
     * @param Min Vector containing the minimum values.
     * @param Max Vector containing the maximum values.
     * @return The clamped vector.
     */
    static FORCEINLINE Vector2 Clamp(const Vector2& Value, const Vector2& Min, const Vector2& Max) noexcept
    {
        return Vector2(Math::Clamp(Value.X, Min.X, Max.X), Math::Clamp(Value.Y, Min.Y, Max.Y));
    }

    /**
     * @brief Saturates the components of a vector to the range [0, 1].
     * @param Value The vector to saturate.
     * @return The saturated vector.
     */
    static FORCEINLINE Vector2 Saturate(const Vector2& Value) noexcept
    {
        return Vector2(Math::Saturate(Value.X), Math::Saturate(Value.Y));
    }

    /**
     * @brief Converts vector components from radians to degrees.
     * @param Radians Vector in radians.
     * @return Vector with components in degrees.
     */
    static FORCEINLINE Vector2 RadiansToDegrees(const Vector2& Radians) noexcept
    {
        return Vector2(Math::RadiansToDegrees(Radians.X), Math::RadiansToDegrees(Radians.Y));
    }

    /**
     * @brief Converts vector components from degrees to radians.
     * @param Degrees Vector in degrees.
     * @return Vector with components in radians.
     */
    static FORCEINLINE Vector2 DegreesToRadians(const Vector2& Degrees) noexcept
    {
        return Vector2(Math::DegreesToRadians(Degrees.X), Math::DegreesToRadians(Degrees.Y));
    }

public:

    /**
     * @brief Returns a vector with negated components.
     * @return The negated vector.
     */
    FORCEINLINE Vector2 operator-() const noexcept
    {
        return Vector2(-X, -Y);
    }

    /**
     * @brief Adds two vectors component-wise.
     * @param RHS The vector to add.
     * @return The result of the addition.
     */
    FORCEINLINE Vector2 operator+(const Vector2& RHS) const noexcept
    {
        return Vector2(X + RHS.X, Y + RHS.Y);
    }

    /**
     * @brief Adds another vector to this vector component-wise.
     * @param RHS The vector to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE Vector2& operator+=(const Vector2& RHS) noexcept
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
    FORCEINLINE Vector2 operator+(float RHS) const noexcept
    {
        return Vector2(X + RHS, Y + RHS);
    }

    /**
     * @brief Adds a scalar to each component of this vector.
     * @param RHS The scalar to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE Vector2& operator+=(float RHS) noexcept
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
    FORCEINLINE Vector2 operator-(const Vector2& RHS) const noexcept
    {
        return Vector2(X - RHS.X, Y - RHS.Y);
    }

    /**
     * @brief Subtracts another vector from this vector component-wise.
     * @param RHS The vector to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE Vector2& operator-=(const Vector2& RHS) noexcept
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
    FORCEINLINE Vector2 operator-(float RHS) const noexcept
    {
        return Vector2(X - RHS, Y - RHS);
    }

    /**
     * @brief Subtracts a scalar from each component of this vector.
     * @param RHS The scalar to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE Vector2& operator-=(float RHS) noexcept
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
    FORCEINLINE Vector2 operator*(const Vector2& RHS) const noexcept
    {
        return Vector2(X * RHS.X, Y * RHS.Y);
    }

    /**
     * @brief Multiplies this vector with another vector component-wise.
     * @param RHS The vector to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE Vector2& operator*=(const Vector2& RHS) noexcept
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
    FORCEINLINE Vector2 operator*(float RHS) const noexcept
    {
        return Vector2(X * RHS, Y * RHS);
    }

    /**
     * @brief Multiplies each component of this vector by a scalar.
     * @param RHS The scalar to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE Vector2& operator*=(float RHS) noexcept
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
    friend FORCEINLINE Vector2 operator*(float LHS, const Vector2& RHS) noexcept
    {
        return Vector2(LHS * RHS.X, LHS * RHS.Y);
    }

    /**
     * @brief Divides this vector by another vector component-wise.
     * @param RHS The vector to divide by.
     * @return The result of the division.
     */
    FORCEINLINE Vector2 operator/(const Vector2& RHS) const noexcept
    {
        return Vector2(X / RHS.X, Y / RHS.Y);
    }

    /**
     * @brief Divides this vector by another vector component-wise.
     * @param RHS The vector to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE Vector2& operator/=(const Vector2& RHS) noexcept
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
    FORCEINLINE Vector2 operator/(float RHS) const noexcept
    {
        return Vector2(X / RHS, Y / RHS);
    }

    /**
     * @brief Divides each component of this vector by a scalar.
     * @param RHS The scalar to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE Vector2& operator/=(float RHS) noexcept
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
    FORCEINLINE bool operator==(const Vector2& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief Checks if this vector is not equal to another vector.
     * @param Other The vector to compare with.
     * @return True if not equal, false otherwise.
     */
    FORCEINLINE bool operator!=(const Vector2& Other) const noexcept
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

MARK_AS_REALLOCATABLE(Vector2);
