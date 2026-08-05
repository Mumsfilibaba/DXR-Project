#pragma once
#include "Core/Math/Math.h"

class Vector3
{
public:

    /** @brief Engine up-direction */
    static CORE_API const Vector3 Up;

    /** @brief Engine forward-direction */
    static CORE_API const Vector3 Forward;

    /** @brief Engine right-direction */
    static CORE_API const Vector3 Right;

public:

    /**
     * @brief Default constructor (initializes components to zero).
     */
    FORCEINLINE Vector3() noexcept
        : X(0.0f)
        , Y(0.0f)
        , Z(0.0f)
    {
    }

    /**
     * @brief Constructor initializing all components with specific values.
     * @param InX The X-coordinate.
     * @param InY The Y-coordinate.
     * @param InZ The Z-coordinate.
     */
    FORCEINLINE explicit Vector3(float InX, float InY, float InZ) noexcept
        : X(InX)
        , Y(InY)
        , Z(InZ)
    {
    }

    /**
     * @brief Constructor initializing all components with a single value.
     * @param Scalar Value to set all components to.
     */
    FORCEINLINE explicit Vector3(float Scalar) noexcept
        : X(Scalar)
        , Y(Scalar)
        , Z(Scalar)
    {
    }

    /**
     * @brief Normalizes this vector.
     * @return Reference to this vector after normalization.
     */
    inline Vector3& Normalize() noexcept
    {
        const float LengthSqrd = GetLengthSquared();
        if (LengthSqrd != 0.0f)
        {
            const float RcpLength = 1.0f / Math::Sqrt(LengthSqrd);

            X *= RcpLength;
            Y *= RcpLength;
            Z *= RcpLength;
        }

        return *this;
    }

    /**
     * @brief Returns a normalized version of this vector.
     * @return A normalized copy of this vector.
     */
    FORCEINLINE Vector3 GetNormalized() const noexcept
    {
        Vector3 Result(*this);
        Result.Normalize();
        return Result;
    }

    /**
     * @brief Compares this vector with another vector within a specified threshold.
     * @param Other Vector to compare against.
     * @param Threshold The threshold for comparison.
     * @return True if vectors are approximately equal, false otherwise.
     */
    inline bool IsEqual(const Vector3& Other, float Threshold = Math::Constants::CmpThreshold) const noexcept
    {
        Threshold = Math::Abs(Threshold);

        for (int32 Index = 0; Index < 3; ++Index)
        {
            float Diff = XYZ[Index] - Other.XYZ[Index];
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
        return LengthDiff < Math::Constants::CmpThreshold;
    }

    /**
     * @brief Checks whether this vector contains any NaN components.
     * @return True if any component equals NaN, false otherwise.
     */
    FORCEINLINE bool ContainsNaN() const noexcept
    {
        for (int32 Index = 0; Index < 3; ++Index)
        {
            if (Math::IsNaN(XYZ[Index]))
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
        for (int32 Index = 0; Index < 3; ++Index)
        {
            if (Math::IsInfinity(XYZ[Index]))
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
    FORCEINLINE float DotProduct(const Vector3& Other) const noexcept
    {
        return (X * Other.X) + (Y * Other.Y) + (Z * Other.Z);
    }

    /**
     * @brief Calculates the cross product between this vector and another vector.
     * @param Other The vector to perform the cross product with.
     * @return The cross product vector.
     */
    inline Vector3 CrossProduct(const Vector3& Other) const noexcept
    {
        return Vector3((Y * Other.Z) - (Z * Other.Y), (Z * Other.X) - (X * Other.Z), (X * Other.Y) - (Y * Other.X));
    }

    /**
     * @brief Projects this vector onto another vector.
     * @param Other The vector to project onto.
     * @return The projected vector.
     */
    inline Vector3 ProjectOn(const Vector3& Other) const noexcept
    {
        float AdotB = DotProduct(Other);
        float BdotB = Other.GetLengthSquared();

        // Prevent division by zero
        if (BdotB == 0.0f)
        {
            return Vector3(0.0f, 0.0f, 0.0f);
        }

        return (AdotB / BdotB) * Other;
    }

    /**
     * @brief Removes the component of this vector that lies along another vector, then normalizes the result.
     * @param Other The vector to make this vector orthogonal to.
     * @return A unit vector orthogonal to Other, or a zero vector when the two are parallel.
     */
    FORCEINLINE Vector3 GetOrthonormalTo(const Vector3& Other) const noexcept
    {
        return (*this - ProjectOn(Other)).GetNormalized();
    }

    /**
     * @brief Reflects this vector around a normal vector.
     * @param Normal The normal vector to reflect around (should be normalized).
     * @return The reflected vector.
     */
    inline Vector3 GetReflected(const Vector3& Normal) const noexcept
    {
        return *this - 2.0f * DotProduct(Normal) * Normal;
    }

    /**
     * @brief Calculates the distance to another vector.
     * @param Other The other vector.
     * @return The distance between this vector and the other vector.
     */
    FORCEINLINE float GetDistanceTo(const Vector3& Other) const noexcept
    {
        return (*this - Other).GetLength();
    }

    /**
     * @brief Calculates the squared distance to another vector.
     * @param Other The other vector.
     * @return The squared distance between this vector and the other vector.
     */
    FORCEINLINE float GetDistanceSquaredTo(const Vector3& Other) const noexcept
    {
        return (*this - Other).GetLengthSquared();
    }

    /**
     * @brief Calculates the angle between this vector and another vector.
     * @param Other The other vector.
     * @return The angle in radians between the two vectors.
     */
    FORCEINLINE float GetAngleBetween(const Vector3& Other) const noexcept
    {
        float AdotB   = DotProduct(Other);
        float Lengths = GetLength() * Other.GetLength();

        // Prevent division by zero
        if (Lengths == 0.0f)
        {
            return 0.0f;
        }

        float CosTheta = Math::Clamp(AdotB / Lengths, -1.0f, 1.0f);
        return Math::Acos(CosTheta);
    }

    /**
     * @brief Returns a rotated version of this vector around an axis by a specified angle.
     * @param Axis The axis to rotate around (should be normalized).
     * @param AngleRadians The angle to rotate by, in radians.
     * @return The rotated vector.
     */
    inline Vector3 GetRotated(const Vector3& Axis, float AngleRadians) const noexcept
    {
        // Using Rodrigues' rotation formula
        Vector3 NormalizedAxis = Axis.GetNormalized();
        return (*this) * Math::Cos(AngleRadians) 
            + NormalizedAxis.CrossProduct(*this) * Math::Sin(AngleRadians)
            + NormalizedAxis * NormalizedAxis.DotProduct(*this) * (1 - Math::Cos(AngleRadians));
    }

public:

    /**
     * @brief Returns a vector with the smallest components of two vectors.
     * @param First First vector to compare.
     * @param Second Second vector to compare.
     * @return A vector with the smallest components.
     */
    static FORCEINLINE Vector3 Min(const Vector3& First, const Vector3& Second) noexcept
    {
        return Vector3(Math::Min(First.X, Second.X), Math::Min(First.Y, Second.Y), Math::Min(First.Z, Second.Z));
    }

    /**
     * @brief Returns a vector with the largest components of two vectors.
     * @param First First vector to compare.
     * @param Second Second vector to compare.
     * @return A vector with the largest components.
     */
    static FORCEINLINE Vector3 Max(const Vector3& First, const Vector3& Second) noexcept
    {
        return Vector3(Math::Max(First.X, Second.X), Math::Max(First.Y, Second.Y), Math::Max(First.Z, Second.Z));
    }

    /**
     * @brief Performs linear interpolation between two vectors.
     * @param First First vector.
     * @param Second Second vector.
     * @param Factor Interpolation factor (0 returns First, 1 returns Second).
     * @return The interpolated vector.
     */
    static FORCEINLINE Vector3 Lerp(const Vector3& First, const Vector3& Second, float Factor) noexcept
    {
        return Vector3((1.0f - Factor) * First.X + Factor * Second.X, (1.0f - Factor) * First.Y + Factor * Second.Y, (1.0f - Factor) * First.Z + Factor * Second.Z);
    }

    /**
     * @brief Clamps the components of a vector within specified ranges.
     * @param Value The vector to clamp.
     * @param Min Vector containing the minimum values.
     * @param Max Vector containing the maximum values.
     * @return The clamped vector.
     */
    static FORCEINLINE Vector3 Clamp(const Vector3& Value, const Vector3& Min, const Vector3& Max) noexcept
    {
        return Vector3(Math::Clamp(Value.X, Min.X, Max.X), Math::Clamp(Value.Y, Min.Y, Max.Y), Math::Clamp(Value.Z, Min.Z, Max.Z));
    }

    /**
     * @brief Saturates the components of a vector to the range [0, 1].
     * @param Value The vector to saturate.
     * @return The saturated vector.
     */
    static FORCEINLINE Vector3 Saturate(const Vector3& Value) noexcept
    {
        return Vector3(Math::Saturate(Value.X), Math::Saturate(Value.Y), Math::Saturate(Value.Z));
    }

    /**
     * @brief Converts vector components from radians to degrees.
     * @param Radians Vector in radians.
     * @return Vector with components in degrees.
     */
    static FORCEINLINE Vector3 RadiansToDegrees(const Vector3& Radians) noexcept
    {
        return Vector3(Math::RadiansToDegrees(Radians.X), Math::RadiansToDegrees(Radians.Y), Math::RadiansToDegrees(Radians.Z));
    }

    /**
     * @brief Converts vector components from degrees to radians.
     * @param Degrees Vector in degrees.
     * @return Vector with components in radians.
     */
    static FORCEINLINE Vector3 DegreesToRadians(const Vector3& Degrees) noexcept
    {
        return Vector3(Math::DegreesToRadians(Degrees.X), Math::DegreesToRadians(Degrees.Y), Math::DegreesToRadians(Degrees.Z));
    }

public:

    /**
     * @brief Returns a vector with negated components.
     * @return The negated vector.
     */
    FORCEINLINE Vector3 operator-() const noexcept
    {
        return Vector3(-X, -Y, -Z);
    }

    /**
     * @brief Adds two vectors component-wise.
     * @param RHS The vector to add.
     * @return The result of the addition.
     */
    FORCEINLINE Vector3 operator+(const Vector3& RHS) const noexcept
    {
        return Vector3(X + RHS.X, Y + RHS.Y, Z + RHS.Z);
    }

    /**
     * @brief Adds another vector to this vector component-wise.
     * @param RHS The vector to add.
     * @return Reference to this vector after addition.
     */
    FORCEINLINE Vector3& operator+=(const Vector3& RHS) noexcept
    {
        X += RHS.X;
        Y += RHS.Y;
        Z += RHS.Z;
        return *this;
    }

    /**
     * @brief Adds a scalar to each component of this vector.
     * @param RHS The scalar to add.
     * @return The result of the addition.
     */
    FORCEINLINE Vector3 operator+(float RHS) const noexcept
    {
        return Vector3(X + RHS, Y + RHS, Z + RHS);
    }

    /**
     * @brief Adds a scalar to each component of this vector.
     * @param RHS The scalar to add.
     * @return Reference to this vector after addition.
     */
    FORCEINLINE Vector3& operator+=(float RHS) noexcept
    {
        X += RHS;
        Y += RHS;
        Z += RHS;
        return *this;
    }

    /**
     * @brief Subtracts another vector from this vector component-wise.
     * @param RHS The vector to subtract.
     * @return The result of the subtraction.
     */
    FORCEINLINE Vector3 operator-(const Vector3& RHS) const noexcept
    {
        return Vector3(X - RHS.X, Y - RHS.Y, Z - RHS.Z);
    }

    /**
     * @brief Subtracts another vector from this vector component-wise.
     * @param RHS The vector to subtract.
     * @return Reference to this vector after subtraction.
     */
    FORCEINLINE Vector3& operator-=(const Vector3& RHS) noexcept
    {
        X -= RHS.X;
        Y -= RHS.Y;
        Z -= RHS.Z;
        return *this;
    }

    /**
     * @brief Subtracts a scalar from each component of this vector.
     * @param RHS The scalar to subtract.
     * @return The result of the subtraction.
     */
    FORCEINLINE Vector3 operator-(float RHS) const noexcept
    {
        return Vector3(X - RHS, Y - RHS, Z - RHS);
    }

    /**
     * @brief Subtracts a scalar from each component of this vector.
     * @param RHS The scalar to subtract.
     * @return Reference to this vector after subtraction.
     */
    FORCEINLINE Vector3& operator-=(float RHS) noexcept
    {
        X -= RHS;
        Y -= RHS;
        Z -= RHS;
        return *this;
    }

    /**
     * @brief Multiplies this vector with another vector component-wise.
     * @param RHS The vector to multiply with.
     * @return The result of the multiplication.
     */
    FORCEINLINE Vector3 operator*(const Vector3& RHS) const noexcept
    {
        return Vector3(X * RHS.X, Y * RHS.Y, Z * RHS.Z);
    }

    /**
     * @brief Multiplies this vector with another vector component-wise.
     * @param RHS The vector to multiply with.
     * @return Reference to this vector after multiplication.
     */
    FORCEINLINE Vector3& operator*=(const Vector3& RHS) noexcept
    {
        X *= RHS.X;
        Y *= RHS.Y;
        Z *= RHS.Z;
        return *this;
    }

    /**
     * @brief Multiplies each component of this vector by a scalar.
     * @param RHS The scalar to multiply with.
     * @return The result of the multiplication.
     */
    FORCEINLINE Vector3 operator*(float RHS) const noexcept
    {
        return Vector3(X * RHS, Y * RHS, Z * RHS);
    }

    /**
     * @brief Multiplies each component of this vector by a scalar.
     * @param RHS The scalar to multiply with.
     * @return Reference to this vector after multiplication.
     */
    FORCEINLINE Vector3& operator*=(float RHS) noexcept
    {
        X *= RHS;
        Y *= RHS;
        Z *= RHS;
        return *this;
    }

    /**
     * @brief Multiplies a scalar with a vector (friend function).
     * @param LHS The scalar to multiply.
     * @param RHS The vector to multiply with.
     * @return The result of the multiplication.
     */
    friend FORCEINLINE Vector3 operator*(float LHS, const Vector3& RHS) noexcept
    {
        return Vector3(LHS * RHS.X, LHS * RHS.Y, LHS * RHS.Z);
    }

    /**
     * @brief Divides this vector by another vector component-wise.
     * @param RHS The vector to divide by.
     * @return The result of the division.
     */
    FORCEINLINE Vector3 operator/(const Vector3& RHS) const noexcept
    {
        return Vector3(X / RHS.X, Y / RHS.Y, Z / RHS.Z);
    }

    /**
     * @brief Divides this vector by another vector component-wise.
     * @param RHS The vector to divide by.
     * @return Reference to this vector after division.
     */
    FORCEINLINE Vector3& operator/=(const Vector3& RHS) noexcept
    {
        X /= RHS.X;
        Y /= RHS.Y;
        Z /= RHS.Z;
        return *this;
    }

    /**
     * @brief Divides each component of this vector by a scalar.
     * @param RHS The scalar to divide by.
     * @return The result of the division.
     */
    FORCEINLINE Vector3 operator/(float RHS) const noexcept
    {
        return Vector3(X / RHS, Y / RHS, Z / RHS);
    }

    /**
     * @brief Divides each component of this vector by a scalar.
     * @param RHS The scalar to divide by.
     * @return Reference to this vector after division.
     */
    FORCEINLINE Vector3& operator/=(float RHS) noexcept
    {
        X /= RHS;
        Y /= RHS;
        Z /= RHS;
        return *this;
    }

    /**
     * @brief Checks if this vector is equal to another vector.
     * @param Other The vector to compare with.
     * @return True if equal, false otherwise.
     */
    FORCEINLINE bool operator==(const Vector3& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief Checks if this vector is not equal to another vector.
     * @param Other The vector to compare with.
     * @return True if not equal, false otherwise.
     */
    FORCEINLINE bool operator!=(const Vector3& Other) const noexcept
    {
        return !IsEqual(Other);
    }

public:

    /**
     * @brief Accesses a component of the vector by index.
     * @param Index The component index (0 for X, 1 for Y, 2 for Z).
     * @return Reference to the component.
     */
    FORCEINLINE float& operator[](int32 Index) noexcept
    {
        CHECK(Index >= 0 && Index < 3);
        return XYZ[Index];
    }

    /**
     * @brief Accesses a component of the vector by index.
     * @param Index The component index (0 for X, 1 for Y, 2 for Z).
     * @return The component value.
     */
    FORCEINLINE float operator[](int32 Index) const noexcept
    {
        CHECK(Index >= 0 && Index < 3);
        return XYZ[Index];
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

            /** @brief The Z-coordinate. */
            float Z;
        };

        /** @brief An array containing the X, Y, and Z components. */
        float XYZ[3];
    };
};

MARK_AS_REALLOCATABLE(Vector3);
