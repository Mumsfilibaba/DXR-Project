#pragma once
#include "Core/Math/MathCommon.h"
#include "Core/Math/VectorMath/VectorMath.h"

class FInt16Vector3
{
public:

    /** @brief Default constructor initializes components to zero. */
    FORCEINLINE FInt16Vector3() noexcept
        : X(0)
        , Y(0)
        , Z(0)
    {
    }

    /**
     * @brief Constructs the vector with specified X, Y, and Z components.
     * @param InX The X-coordinate.
     * @param InY The Y-coordinate.
     * @param InZ The Z-coordinate.
     */
    FORCEINLINE FInt16Vector3(int16 InX, int16 InY, int16 InZ) noexcept
        : X(InX)
        , Y(InY)
        , Z(InZ)
    {
    }

    /**
     * @brief Constructs the vector by setting X, Y, and Z to the same scalar value.
     * @param Scalar The scalar value to set all components.
     */
    FORCEINLINE explicit FInt16Vector3(int16 Scalar) noexcept
        : X(Scalar)
        , Y(Scalar)
        , Z(Scalar)
    {
    }

public:

    /**
     * @brief Returns a negated vector.
     * @return A new vector with each component negated.
     */
    FORCEINLINE FInt16Vector3 operator-() const noexcept
    {
        return FInt16Vector3(-X, -Y, -Z);
    }

    /**
     * @brief Adds another vector component-wise.
     * @param Other The vector to add.
     * @return A new vector representing the sum.
     */
    FORCEINLINE FInt16Vector3 operator+(const FInt16Vector3& Other) const noexcept
    {
        return FInt16Vector3(X + Other.X, Y + Other.Y, Z + Other.Z);
    }

    /**
     * @brief Adds another vector component-wise to this vector.
     * @param Other The vector to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE FInt16Vector3& operator+=(const FInt16Vector3& Other) noexcept
    {
        X += Other.X;
        Y += Other.Y;
        Z += Other.Z;
        return *this;
    }

    /**
     * @brief Adds a scalar to each component.
     * @param Scalar The scalar value to add.
     * @return A new vector with each component increased by the scalar.
     */
    FORCEINLINE FInt16Vector3 operator+(int16 Scalar) const noexcept
    {
        return FInt16Vector3(X + Scalar, Y + Scalar, Z + Scalar);
    }

    /**
     * @brief Adds a scalar to each component of this vector.
     * @param Scalar The scalar value to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE FInt16Vector3& operator+=(int16 Scalar) noexcept
    {
        X += Scalar;
        Y += Scalar;
        Z += Scalar;
        return *this;
    }

    /**
     * @brief Subtracts another vector component-wise.
     * @param Other The vector to subtract.
     * @return A new vector representing the difference.
     */
    FORCEINLINE FInt16Vector3 operator-(const FInt16Vector3& Other) const noexcept
    {
        return FInt16Vector3(X - Other.X, Y - Other.Y, Z - Other.Z);
    }

    /**
     * @brief Subtracts another vector component-wise from this vector.
     * @param Other The vector to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE FInt16Vector3& operator-=(const FInt16Vector3& Other) noexcept
    {
        X -= Other.X;
        Y -= Other.Y;
        Z -= Other.Z;
        return *this;
    }

    /**
     * @brief Subtracts a scalar from each component.
     * @param Scalar The scalar value to subtract.
     * @return A new vector with each component decreased by the scalar.
     */
    FORCEINLINE FInt16Vector3 operator-(int16 Scalar) const noexcept
    {
        return FInt16Vector3(X - Scalar, Y - Scalar, Z - Scalar);
    }

    /**
     * @brief Subtracts a scalar from each component of this vector.
     * @param Scalar The scalar value to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE FInt16Vector3& operator-=(int16 Scalar) noexcept
    {
        X -= Scalar;
        Y -= Scalar;
        Z -= Scalar;
        return *this;
    }

    /**
     * @brief Multiplies another vector component-wise.
     * @param Other The vector to multiply with.
     * @return A new vector representing the product.
     */
    FORCEINLINE FInt16Vector3 operator*(const FInt16Vector3& Other) const noexcept
    {
        return FInt16Vector3(X * Other.X, Y * Other.Y, Z * Other.Z);
    }

    /**
     * @brief Multiplies another vector component-wise with this vector.
     * @param Other The vector to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE FInt16Vector3& operator*=(const FInt16Vector3& Other) noexcept
    {
        X *= Other.X;
        Y *= Other.Y;
        Z *= Other.Z;
        return *this;
    }

    /**
     * @brief Multiplies each component by a scalar.
     * @param Scalar The scalar value to multiply with.
     * @return A new vector with each component multiplied by the scalar.
     */
    FORCEINLINE FInt16Vector3 operator*(int16 Scalar) const noexcept
    {
        return FInt16Vector3(X * Scalar, Y * Scalar, Z * Scalar);
    }

    /**
     * @brief Multiplies each component of this vector by a scalar.
     * @param Scalar The scalar value to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE FInt16Vector3& operator*=(int16 Scalar) noexcept
    {
        X *= Scalar;
        Y *= Scalar;
        Z *= Scalar;
        return *this;
    }

    /**
     * @brief Divides another vector component-wise.
     * @param Other The vector to divide by.
     * @return A new vector representing the quotient.
     */
    FORCEINLINE FInt16Vector3 operator/(const FInt16Vector3& Other) const noexcept
    {
        CHECK(Other.X != 0 && Other.Y != 0 && Other.Z != 0);
        return FInt16Vector3(X / Other.X, Y / Other.Y, Z / Other.Z);
    }

    /**
     * @brief Divides another vector component-wise with this vector.
     * @param Other The vector to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE FInt16Vector3& operator/=(const FInt16Vector3& Other) noexcept
    {
        CHECK(Other.X != 0 && Other.Y != 0 && Other.Z != 0);
        X /= Other.X;
        Y /= Other.Y;
        Z /= Other.Z;
        return *this;
    }

    /**
     * @brief Divides each component by a scalar.
     * @param Scalar The scalar value to divide by.
     * @return A new vector with each component divided by the scalar.
     */
    FORCEINLINE FInt16Vector3 operator/(int16 Scalar) const noexcept
    {
        CHECK(Scalar != 0);
        return FInt16Vector3(X / Scalar, Y / Scalar, Z / Scalar);
    }

    /**
     * @brief Divides each component of this vector by a scalar.
     * @param Scalar The scalar value to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE FInt16Vector3& operator/=(int16 Scalar) noexcept
    {
        CHECK(Scalar != 0);
        X /= Scalar;
        Y /= Scalar;
        Z /= Scalar;
        return *this;
    }

    /**
     * @brief Compares this vector with another for equality.
     * @param Other The vector to compare against.
     * @return True if all components are equal; otherwise, false.
     */
    FORCEINLINE bool operator==(const FInt16Vector3& Other) const noexcept
    {
        return (X == Other.X) && (Y == Other.Y) && (Z == Other.Z);
    }

    /**
     * @brief Compares this vector with another for inequality.
     * @param Other The vector to compare against.
     * @return True if any component is not equal; otherwise, false.
     */
    FORCEINLINE bool operator!=(const FInt16Vector3& Other) const noexcept
    {
        return !(*this == Other);
    }

public:

    /**
     * @brief Returns the component-wise minimum of two vectors.
     * @param LHS First vector.
     * @param RHS Second vector.
     * @return A new vector containing the minimum of each component.
     */
    static FORCEINLINE FInt16Vector3 Min(const FInt16Vector3& LHS, const FInt16Vector3& RHS) noexcept
    {
        return FInt16Vector3(FMath::Min(LHS.X, RHS.X), FMath::Min(LHS.Y, RHS.Y), FMath::Min(LHS.Z, RHS.Z));
    }

    /**
     * @brief Returns the component-wise maximum of two vectors.
     * @param LHS First vector.
     * @param RHS Second vector.
     * @return A new vector containing the maximum of each component.
     */
    static FORCEINLINE FInt16Vector3 Max(const FInt16Vector3& LHS, const FInt16Vector3& RHS) noexcept
    {
        return FInt16Vector3(FMath::Max(LHS.X, RHS.X), FMath::Max(LHS.Y, RHS.Y), FMath::Max(LHS.Z, RHS.Z));
    }

    /**
     * @brief Clamps each component of a vector between the corresponding components of min and max vectors.
     * @param Value The vector to clamp.
     * @param Min The minimum bounds vector.
     * @param Max The maximum bounds vector.
     * @return A new vector with each component clamped.
     */
    static FORCEINLINE FInt16Vector3 Clamp(const FInt16Vector3& Value, const FInt16Vector3& Min, const FInt16Vector3& Max) noexcept
    {
        return FInt16Vector3(FMath::Clamp(Value.X, Min.X, Max.X), FMath::Clamp(Value.Y, Min.Y, Max.Y), FMath::Clamp(Value.Z, Min.Z, Max.Z));
    }

public:

    /**
     * @brief Adds a scalar to each component of the vector (scalar on left-hand side).
     * @param Scalar The scalar value to add.
     * @param Vector The vector to add the scalar to.
     * @return A new vector with each component increased by the scalar.
     */
    FORCEINLINE friend FInt16Vector3 operator+(int16 Scalar, const FInt16Vector3& Vector) noexcept
    {
        return Vector + Scalar;
    }

    /**
     * @brief Subtracts a vector from a scalar (scalar on left-hand side).
     * @param Scalar The scalar value.
     * @param Vector The vector to subtract from the scalar.
     * @return A new vector with each component being Scalar minus the original component.
     */
    FORCEINLINE friend FInt16Vector3 operator-(int16 Scalar, const FInt16Vector3& Vector) noexcept
    {
        return FInt16Vector3(Scalar - Vector.X, Scalar - Vector.Y, Scalar - Vector.Z);
    }

    /**
     * @brief Multiplies each component of the vector by a scalar (scalar on left-hand side).
     * @param Scalar The scalar value to multiply with.
     * @param Vector The vector to multiply.
     * @return A new vector with each component multiplied by the scalar.
     */
    FORCEINLINE friend FInt16Vector3 operator*(int16 Scalar, const FInt16Vector3& Vector) noexcept
    {
        return Vector * Scalar;
    }

    /**
     * @brief Divides a scalar by each component of the vector (scalar on left-hand side).
     * @param Scalar The scalar value to divide.
     * @param Vector The vector whose components divide the scalar.
     * @return A new vector with each component being Scalar divided by the original component.
     */
    FORCEINLINE friend FInt16Vector3 operator/(int16 Scalar, const FInt16Vector3& Vector) noexcept
    {
        CHECK(Vector.X != 0 && Vector.Y != 0 && Vector.Z != 0);
        return FInt16Vector3(Scalar / Vector.X, Scalar / Vector.Y, Scalar / Vector.Z);
    }

public:

    /**
     * @brief Accesses the component at the specified index.
     * @param Index The component index (0 for X, 1 for Y, 2 for Z).
     * @return Reference to the component.
     */
    FORCEINLINE int16& operator[](int32 Index) noexcept
    {
        CHECK(Index >= 0 && Index < 3);
        return XYZ[Index];
    }

    /**
     * @brief Accesses the component at the specified index (const version).
     * @param Index The component index (0 for X, 1 for Y, 2 for Z).
     * @return The component value.
     */
    FORCEINLINE int16 operator[](int32 Index) const noexcept
    {
        CHECK(Index >= 0 && Index < 3);
        return XYZ[Index];
    }

public:

    union
    {
        struct
        {
            /** @brief The X-coordinate */
            int16 X;

            /** @brief The Y-coordinate */
            int16 Y;

            /** @brief The Z-coordinate */
            int16 Z;
        };

        int16 XYZ[3];
    };
};

MARK_AS_REALLOCATABLE(FInt16Vector3);

class FIntVector3
{
public:

    /** @brief Default constructor initializes components to zero. */
    FORCEINLINE FIntVector3() noexcept
        : X(0)
        , Y(0)
        , Z(0)
    {
    }

    /**
     * @brief Constructs the vector with specified X, Y, and Z components.
     * @param InX The X-coordinate.
     * @param InY The Y-coordinate.
     * @param InZ The Z-coordinate.
     */
    FORCEINLINE FIntVector3(int32 InX, int32 InY, int32 InZ) noexcept
        : X(InX)
        , Y(InY)
        , Z(InZ)
    {
    }

    /**
     * @brief Constructs the vector by setting X, Y, and Z to the same scalar value.
     * @param Scalar The scalar value to set all components.
     */
    FORCEINLINE explicit FIntVector3(int32 Scalar) noexcept
        : X(Scalar)
        , Y(Scalar)
        , Z(Scalar)
    {
    }

public:

    /**
     * @brief Returns a negated vector.
     * @return A new vector with each component negated.
     */
    FORCEINLINE FIntVector3 operator-() const noexcept
    {
        return FIntVector3(-X, -Y, -Z);
    }

    /**
     * @brief Adds another vector component-wise.
     * @param Other The vector to add.
     * @return A new vector representing the sum.
     */
    FORCEINLINE FIntVector3 operator+(const FIntVector3& Other) const noexcept
    {
        return FIntVector3(X + Other.X, Y + Other.Y, Z + Other.Z);
    }

    /**
     * @brief Adds another vector component-wise to this vector.
     * @param Other The vector to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE FIntVector3& operator+=(const FIntVector3& Other) noexcept
    {
        X += Other.X;
        Y += Other.Y;
        Z += Other.Z;
        return *this;
    }

    /**
     * @brief Adds a scalar to each component.
     * @param Scalar The scalar value to add.
     * @return A new vector with each component increased by the scalar.
     */
    FORCEINLINE FIntVector3 operator+(int32 Scalar) const noexcept
    {
        return FIntVector3(X + Scalar, Y + Scalar, Z + Scalar);
    }

    /**
     * @brief Adds a scalar to each component of this vector.
     * @param Scalar The scalar value to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE FIntVector3& operator+=(int32 Scalar) noexcept
    {
        X += Scalar;
        Y += Scalar;
        Z += Scalar;
        return *this;
    }

    /**
     * @brief Subtracts another vector component-wise.
     * @param Other The vector to subtract.
     * @return A new vector representing the difference.
     */
    FORCEINLINE FIntVector3 operator-(const FIntVector3& Other) const noexcept
    {
        return FIntVector3(X - Other.X, Y - Other.Y, Z - Other.Z);
    }

    /**
     * @brief Subtracts another vector component-wise from this vector.
     * @param Other The vector to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE FIntVector3& operator-=(const FIntVector3& Other) noexcept
    {
        X -= Other.X;
        Y -= Other.Y;
        Z -= Other.Z;
        return *this;
    }

    /**
     * @brief Subtracts a scalar from each component.
     * @param Scalar The scalar value to subtract.
     * @return A new vector with each component decreased by the scalar.
     */
    FORCEINLINE FIntVector3 operator-(int32 Scalar) const noexcept
    {
        return FIntVector3(X - Scalar, Y - Scalar, Z - Scalar);
    }

    /**
     * @brief Subtracts a scalar from each component of this vector.
     * @param Scalar The scalar value to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE FIntVector3& operator-=(int32 Scalar) noexcept
    {
        X -= Scalar;
        Y -= Scalar;
        Z -= Scalar;
        return *this;
    }

    /**
     * @brief Multiplies another vector component-wise.
     * @param Other The vector to multiply with.
     * @return A new vector representing the product.
     */
    FORCEINLINE FIntVector3 operator*(const FIntVector3& Other) const noexcept
    {
        return FIntVector3(X * Other.X, Y * Other.Y, Z * Other.Z);
    }

    /**
     * @brief Multiplies another vector component-wise with this vector.
     * @param Other The vector to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE FIntVector3& operator*=(const FIntVector3& Other) noexcept
    {
        X *= Other.X;
        Y *= Other.Y;
        Z *= Other.Z;
        return *this;
    }

    /**
     * @brief Multiplies each component by a scalar.
     * @param Scalar The scalar value to multiply with.
     * @return A new vector with each component multiplied by the scalar.
     */
    FORCEINLINE FIntVector3 operator*(int32 Scalar) const noexcept
    {
        return FIntVector3(X * Scalar, Y * Scalar, Z * Scalar);
    }

    /**
     * @brief Multiplies each component of this vector by a scalar.
     * @param Scalar The scalar value to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE FIntVector3& operator*=(int32 Scalar) noexcept
    {
        X *= Scalar;
        Y *= Scalar;
        Z *= Scalar;
        return *this;
    }

    /**
     * @brief Divides another vector component-wise.
     * @param Other The vector to divide by.
     * @return A new vector representing the quotient.
     */
    FORCEINLINE FIntVector3 operator/(const FIntVector3& Other) const noexcept
    {
        CHECK(Other.X != 0 && Other.Y != 0 && Other.Z != 0);
        return FIntVector3(X / Other.X, Y / Other.Y, Z / Other.Z);
    }

    /**
     * @brief Divides another vector component-wise with this vector.
     * @param Other The vector to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE FIntVector3& operator/=(const FIntVector3& Other) noexcept
    {
        CHECK(Other.X != 0 && Other.Y != 0 && Other.Z != 0);
        X /= Other.X;
        Y /= Other.Y;
        Z /= Other.Z;
        return *this;
    }

    /**
     * @brief Divides each component by a scalar.
     * @param Scalar The scalar value to divide by.
     * @return A new vector with each component divided by the scalar.
     */
    FORCEINLINE FIntVector3 operator/(int32 Scalar) const noexcept
    {
        CHECK(Scalar != 0);
        return FIntVector3(X / Scalar, Y / Scalar, Z / Scalar);
    }

    /**
     * @brief Divides each component of this vector by a scalar.
     * @param Scalar The scalar value to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE FIntVector3& operator/=(int32 Scalar) noexcept
    {
        CHECK(Scalar != 0);
        X /= Scalar;
        Y /= Scalar;
        Z /= Scalar;
        return *this;
    }

    /**
     * @brief Compares this vector with another for equality.
     * @param Other The vector to compare against.
     * @return True if all components are equal; otherwise, false.
     */
    FORCEINLINE bool operator==(const FIntVector3& Other) const noexcept
    {
        return (X == Other.X) && (Y == Other.Y) && (Z == Other.Z);
    }

    /**
     * @brief Compares this vector with another for inequality.
     * @param Other The vector to compare against.
     * @return True if any component is not equal; otherwise, false.
     */
    FORCEINLINE bool operator!=(const FIntVector3& Other) const noexcept
    {
        return !(*this == Other);
    }

public:

    /**
     * @brief Returns the component-wise minimum of two vectors.
     * @param LHS First vector.
     * @param RHS Second vector.
     * @return A new vector containing the minimum of each component.
     */
    static FORCEINLINE FIntVector3 Min(const FIntVector3& LHS, const FIntVector3& RHS) noexcept
    {
        return FIntVector3(FMath::Min(LHS.X, RHS.X), FMath::Min(LHS.Y, RHS.Y), FMath::Min(LHS.Z, RHS.Z));
    }

    /**
     * @brief Returns the component-wise maximum of two vectors.
     * @param LHS First vector.
     * @param RHS Second vector.
     * @return A new vector containing the maximum of each component.
     */
    static FORCEINLINE FIntVector3 Max(const FIntVector3& LHS, const FIntVector3& RHS) noexcept
    {
        return FIntVector3(FMath::Max(LHS.X, RHS.X), FMath::Max(LHS.Y, RHS.Y), FMath::Max(LHS.Z, RHS.Z));
    }

    /**
     * @brief Clamps each component of a vector between the corresponding components of min and max vectors.
     * @param Value The vector to clamp.
     * @param Min The minimum bounds vector.
     * @param Max The maximum bounds vector.
     * @return A new vector with each component clamped.
     */
    static FORCEINLINE FIntVector3 Clamp(const FIntVector3& Value, const FIntVector3& Min, const FIntVector3& Max) noexcept
    {
        return FIntVector3(FMath::Clamp(Value.X, Min.X, Max.X), FMath::Clamp(Value.Y, Min.Y, Max.Y), FMath::Clamp(Value.Z, Min.Z, Max.Z));
    }

public:

    /**
     * @brief Adds a scalar to each component of the vector (scalar on left-hand side).
     * @param Scalar The scalar value to add.
     * @param Vector The vector to add the scalar to.
     * @return A new vector with each component increased by the scalar.
     */
    FORCEINLINE friend FIntVector3 operator+(int32 Scalar, const FIntVector3& Vector) noexcept
    {
        return Vector + Scalar;
    }

    /**
     * @brief Subtracts a vector from a scalar (scalar on left-hand side).
     * @param Scalar The scalar value.
     * @param Vector The vector to subtract from the scalar.
     * @return A new vector with each component being Scalar minus the original component.
     */
    FORCEINLINE friend FIntVector3 operator-(int32 Scalar, const FIntVector3& Vector) noexcept
    {
        return FIntVector3(Scalar - Vector.X, Scalar - Vector.Y, Scalar - Vector.Z);
    }

    /**
     * @brief Multiplies each component of the vector by a scalar (scalar on left-hand side).
     * @param Scalar The scalar value to multiply with.
     * @param Vector The vector to multiply.
     * @return A new vector with each component multiplied by the scalar.
     */
    FORCEINLINE friend FIntVector3 operator*(int32 Scalar, const FIntVector3& Vector) noexcept
    {
        return Vector * Scalar;
    }

    /**
     * @brief Divides a scalar by each component of the vector (scalar on left-hand side).
     * @param Scalar The scalar value to divide.
     * @param Vector The vector whose components divide the scalar.
     * @return A new vector with each component being Scalar divided by the original component.
     */
    FORCEINLINE friend FIntVector3 operator/(int32 Scalar, const FIntVector3& Vector) noexcept
    {
        CHECK(Vector.X != 0 && Vector.Y != 0 && Vector.Z != 0);
        return FIntVector3(Scalar / Vector.X, Scalar / Vector.Y, Scalar / Vector.Z);
    }

public:

    /**
     * @brief Accesses the component at the specified index.
     * @param Index The component index (0 for X, 1 for Y, 2 for Z).
     * @return Reference to the component.
     */
    FORCEINLINE int32& operator[](int32 Index) noexcept
    {
        CHECK(Index >= 0 && Index < 3);
        return XYZ[Index];
    }

    /**
     * @brief Accesses the component at the specified index (const version).
     * @param Index The component index (0 for X, 1 for Y, 2 for Z).
     * @return The component value.
     */
    FORCEINLINE int32 operator[](int32 Index) const noexcept
    {
        CHECK(Index >= 0 && Index < 3);
        return XYZ[Index];
    }

public:

    union
    {
        struct
        {
            /** @brief The X-coordinate */
            int32 X;

            /** @brief The Y-coordinate */
            int32 Y;

            /** @brief The Z-coordinate */
            int32 Z;
        };

        int32 XYZ[3];
    };
};

MARK_AS_REALLOCATABLE(FIntVector3);
