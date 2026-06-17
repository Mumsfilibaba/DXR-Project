#pragma once
#include "Core/Math/Math.h"

class Int16Vector2
{
public:

    /** @brief Default constructor initializes components to zero. */
    FORCEINLINE Int16Vector2() noexcept
        : X(0)
        , Y(0)
    {
    }

    /**
     * @brief Constructs the vector with specified X and Y components.
     * @param InX The X-coordinate.
     * @param InY The Y-coordinate.
     */
    FORCEINLINE Int16Vector2(int16 InX, int16 InY) noexcept
        : X(InX)
        , Y(InY)
    {
    }

    /**
     * @brief Constructs the vector by setting both X and Y to the same scalar value.
     * @param Scalar The scalar value to set both components.
     */
    FORCEINLINE explicit Int16Vector2(int16 Scalar) noexcept
        : X(Scalar)
        , Y(Scalar)
    {
    }

public:

    /**
     * @brief Returns a negated vector.
     * @return A new vector with each component negated.
     */
    FORCEINLINE Int16Vector2 operator-() const noexcept
    {
        return Int16Vector2(-X, -Y);
    }

    /**
     * @brief Adds another vector component-wise.
     * @param Other The vector to add.
     * @return A new vector representing the sum.
     */
    FORCEINLINE Int16Vector2 operator+(const Int16Vector2& Other) const noexcept
    {
        return Int16Vector2(X + Other.X, Y + Other.Y);
    }

    /**
     * @brief Adds another vector component-wise to this vector.
     * @param Other The vector to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE Int16Vector2& operator+=(const Int16Vector2& Other) noexcept
    {
        X += Other.X;
        Y += Other.Y;
        return *this;
    }

    /**
     * @brief Adds a scalar to each component.
     * @param Scalar The scalar value to add.
     * @return A new vector with each component increased by the scalar.
     */
    FORCEINLINE Int16Vector2 operator+(int16 Scalar) const noexcept
    {
        return Int16Vector2(X + Scalar, Y + Scalar);
    }

    /**
     * @brief Adds a scalar to each component of this vector.
     * @param Scalar The scalar value to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE Int16Vector2& operator+=(int16 Scalar) noexcept
    {
        X += Scalar;
        Y += Scalar;
        return *this;
    }

    /**
     * @brief Subtracts another vector component-wise.
     * @param Other The vector to subtract.
     * @return A new vector representing the difference.
     */
    FORCEINLINE Int16Vector2 operator-(const Int16Vector2& Other) const noexcept
    {
        return Int16Vector2(X - Other.X, Y - Other.Y);
    }

    /**
     * @brief Subtracts another vector component-wise from this vector.
     * @param Other The vector to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE Int16Vector2& operator-=(const Int16Vector2& Other) noexcept
    {
        X -= Other.X;
        Y -= Other.Y;
        return *this;
    }

    /**
     * @brief Subtracts a scalar from each component.
     * @param Scalar The scalar value to subtract.
     * @return A new vector with each component decreased by the scalar.
     */
    FORCEINLINE Int16Vector2 operator-(int16 Scalar) const noexcept
    {
        return Int16Vector2(X - Scalar, Y - Scalar);
    }

    /**
     * @brief Subtracts a scalar from each component of this vector.
     * @param Scalar The scalar value to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE Int16Vector2& operator-=(int16 Scalar) noexcept
    {
        X -= Scalar;
        Y -= Scalar;
        return *this;
    }

    /**
     * @brief Multiplies another vector component-wise.
     * @param Other The vector to multiply with.
     * @return A new vector representing the product.
     */
    FORCEINLINE Int16Vector2 operator*(const Int16Vector2& Other) const noexcept
    {
        return Int16Vector2(X * Other.X, Y * Other.Y);
    }

    /**
     * @brief Multiplies another vector component-wise with this vector.
     * @param Other The vector to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE Int16Vector2& operator*=(const Int16Vector2& Other) noexcept
    {
        X *= Other.X;
        Y *= Other.Y;
        return *this;
    }

    /**
     * @brief Multiplies each component by a scalar.
     * @param Scalar The scalar value to multiply with.
     * @return A new vector with each component multiplied by the scalar.
     */
    FORCEINLINE Int16Vector2 operator*(int16 Scalar) const noexcept
    {
        return Int16Vector2(X * Scalar, Y * Scalar);
    }

    /**
     * @brief Multiplies each component of this vector by a scalar.
     * @param Scalar The scalar value to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE Int16Vector2& operator*=(int16 Scalar) noexcept
    {
        X *= Scalar;
        Y *= Scalar;
        return *this;
    }

    /**
     * @brief Divides another vector component-wise.
     * @param Other The vector to divide by.
     * @return A new vector representing the quotient.
     */
    FORCEINLINE Int16Vector2 operator/(const Int16Vector2& Other) const noexcept
    {
        CHECK(Other.X != 0 && Other.Y != 0);
        return Int16Vector2(X / Other.X, Y / Other.Y);
    }

    /**
     * @brief Divides another vector component-wise with this vector.
     * @param Other The vector to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE Int16Vector2& operator/=(const Int16Vector2& Other) noexcept
    {
        CHECK(Other.X != 0 && Other.Y != 0);
        X /= Other.X;
        Y /= Other.Y;
        return *this;
    }

    /**
     * @brief Divides each component by a scalar.
     * @param Scalar The scalar value to divide by.
     * @return A new vector with each component divided by the scalar.
     */
    FORCEINLINE Int16Vector2 operator/(int16 Scalar) const noexcept
    {
        CHECK(Scalar != 0);
        return Int16Vector2(X / Scalar, Y / Scalar);
    }

    /**
     * @brief Divides each component of this vector by a scalar.
     * @param Scalar The scalar value to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE Int16Vector2& operator/=(int16 Scalar) noexcept
    {
        CHECK(Scalar != 0);
        X /= Scalar;
        Y /= Scalar;
        return *this;
    }

    /**
     * @brief Compares this vector with another for equality.
     * @param Other The vector to compare against.
     * @return True if both components are equal; otherwise, false.
     */
    FORCEINLINE bool operator==(const Int16Vector2& Other) const noexcept
    {
        return (X == Other.X) && (Y == Other.Y);
    }

    /**
     * @brief Compares this vector with another for inequality.
     * @param Other The vector to compare against.
     * @return True if not equal; otherwise, false.
     */
    FORCEINLINE bool operator!=(const Int16Vector2& Other) const noexcept
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
    static FORCEINLINE Int16Vector2 Min(const Int16Vector2& LHS, const Int16Vector2& RHS) noexcept
    {
        return Int16Vector2(Math::Min(LHS.X, RHS.X), Math::Min(LHS.Y, RHS.Y));
    }

    /**
     * @brief Returns the component-wise maximum of two vectors.
     * @param LHS First vector.
     * @param RHS Second vector.
     * @return A new vector containing the maximum of each component.
     */
    static FORCEINLINE Int16Vector2 Max(const Int16Vector2& LHS, const Int16Vector2& RHS) noexcept
    {
        return Int16Vector2(Math::Max(LHS.X, RHS.X), Math::Max(LHS.Y, RHS.Y));
    }

    /**
     * @brief Clamps each component of a vector between the corresponding components of min and max vectors.
     * @param Value The vector to clamp.
     * @param Min The minimum bounds vector.
     * @param Max The maximum bounds vector.
     * @return A new vector with each component clamped.
     */
    static FORCEINLINE Int16Vector2 Clamp(const Int16Vector2& Value, const Int16Vector2& Min, const Int16Vector2& Max) noexcept
    {
        return Int16Vector2(Math::Clamp(Value.X, Min.X, Max.X), Math::Clamp(Value.Y, Min.Y, Max.Y));
    }

public:

    /**
     * @brief Adds a scalar to each component of the vector (scalar on left-hand side).
     * @param Scalar The scalar value to add.
     * @param Vector The vector to add the scalar to.
     * @return A new vector with each component increased by the scalar.
     */
    FORCEINLINE friend Int16Vector2 operator+(int16 Scalar, const Int16Vector2& Vector) noexcept
    {
        return Vector + Scalar;
    }

    /**
     * @brief Subtracts a vector from a scalar (scalar on left-hand side).
     * @param Scalar The scalar value.
     * @param Vector The vector to subtract from the scalar.
     * @return A new vector with each component being Scalar minus the original component.
     */
    FORCEINLINE friend Int16Vector2 operator-(int16 Scalar, const Int16Vector2& Vector) noexcept
    {
        return Int16Vector2(Scalar - Vector.X, Scalar - Vector.Y);
    }

    /**
     * @brief Multiplies each component of the vector by a scalar (scalar on left-hand side).
     * @param Scalar The scalar value to multiply with.
     * @param Vector The vector to multiply.
     * @return A new vector with each component multiplied by the scalar.
     */
    FORCEINLINE friend Int16Vector2 operator*(int16 Scalar, const Int16Vector2& Vector) noexcept
    {
        return Vector * Scalar;
    }

    /**
     * @brief Divides a scalar by each component of the vector (scalar on left-hand side).
     * @param Scalar The scalar value to divide.
     * @param Vector The vector whose components divide the scalar.
     * @return A new vector with each component being Scalar divided by the original component.
     */
    FORCEINLINE friend Int16Vector2 operator/(int16 Scalar, const Int16Vector2& Vector) noexcept
    {
        CHECK(Vector.X != 0 && Vector.Y != 0);
        return Int16Vector2(Scalar / Vector.X, Scalar / Vector.Y);
    }

public:

    /**
     * @brief Accesses the component at the specified index.
     * @param Index The component index (0 for X, 1 for Y).
     * @return Reference to the component.
     */
    FORCEINLINE int16& operator[](int32 Index) noexcept
    {
        CHECK(Index >= 0 && Index < 2);
        return XY[Index];
    }

    /**
     * @brief Accesses the component at the specified index (const version).
     * @param Index The component index (0 for X, 1 for Y).
     * @return The component value.
     */
    FORCEINLINE int16 operator[](int32 Index) const noexcept
    {
        CHECK(Index >= 0 && Index < 2);
        return XY[Index];
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
        };

        int16 XY[2];
    };
};

MARK_AS_REALLOCATABLE(Int16Vector2);

class IntVector2
{
public:

    /** @brief Default constructor initializes components to zero. */
    FORCEINLINE IntVector2() noexcept
        : X(0)
        , Y(0)
    {
    }

    /**
     * @brief Constructs the vector with specified X and Y components.
     * @param InX The X-coordinate.
     * @param InY The Y-coordinate.
     */
    FORCEINLINE IntVector2(int32 InX, int32 InY) noexcept
        : X(InX)
        , Y(InY)
    {
    }

    /**
     * @brief Constructs the vector by setting both X and Y to the same scalar value.
     * @param Scalar The scalar value to set both components.
     */
    FORCEINLINE explicit IntVector2(int32 Scalar) noexcept
        : X(Scalar)
        , Y(Scalar)
    {
    }

public:

    /**
     * @brief Returns a negated vector.
     * @return A new vector with each component negated.
     */
    FORCEINLINE IntVector2 operator-() const noexcept
    {
        return IntVector2(-X, -Y);
    }

    /**
     * @brief Adds another vector component-wise.
     * @param Other The vector to add.
     * @return A new vector representing the sum.
     */
    FORCEINLINE IntVector2 operator+(const IntVector2& Other) const noexcept
    {
        return IntVector2(X + Other.X, Y + Other.Y);
    }

    /**
     * @brief Adds another vector component-wise to this vector.
     * @param Other The vector to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE IntVector2& operator+=(const IntVector2& Other) noexcept
    {
        X += Other.X;
        Y += Other.Y;
        return *this;
    }

    /**
     * @brief Adds a scalar to each component.
     * @param Scalar The scalar value to add.
     * @return A new vector with each component increased by the scalar.
     */
    FORCEINLINE IntVector2 operator+(int32 Scalar) const noexcept
    {
        return IntVector2(X + Scalar, Y + Scalar);
    }

    /**
     * @brief Adds a scalar to each component of this vector.
     * @param Scalar The scalar value to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE IntVector2& operator+=(int32 Scalar) noexcept
    {
        X += Scalar;
        Y += Scalar;
        return *this;
    }

    /**
     * @brief Subtracts another vector component-wise.
     * @param Other The vector to subtract.
     * @return A new vector representing the difference.
     */
    FORCEINLINE IntVector2 operator-(const IntVector2& Other) const noexcept
    {
        return IntVector2(X - Other.X, Y - Other.Y);
    }

    /**
     * @brief Subtracts another vector component-wise from this vector.
     * @param Other The vector to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE IntVector2& operator-=(const IntVector2& Other) noexcept
    {
        X -= Other.X;
        Y -= Other.Y;
        return *this;
    }

    /**
     * @brief Subtracts a scalar from each component.
     * @param Scalar The scalar value to subtract.
     * @return A new vector with each component decreased by the scalar.
     */
    FORCEINLINE IntVector2 operator-(int32 Scalar) const noexcept
    {
        return IntVector2(X - Scalar, Y - Scalar);
    }

    /**
     * @brief Subtracts a scalar from each component of this vector.
     * @param Scalar The scalar value to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE IntVector2& operator-=(int32 Scalar) noexcept
    {
        X -= Scalar;
        Y -= Scalar;
        return *this;
    }

    /**
     * @brief Multiplies another vector component-wise.
     * @param Other The vector to multiply with.
     * @return A new vector representing the product.
     */
    FORCEINLINE IntVector2 operator*(const IntVector2& Other) const noexcept
    {
        return IntVector2(X * Other.X, Y * Other.Y);
    }

    /**
     * @brief Multiplies another vector component-wise with this vector.
     * @param Other The vector to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE IntVector2& operator*=(const IntVector2& Other) noexcept
    {
        X *= Other.X;
        Y *= Other.Y;
        return *this;
    }

    /**
     * @brief Multiplies each component by a scalar.
     * @param Scalar The scalar value to multiply with.
     * @return A new vector with each component multiplied by the scalar.
     */
    FORCEINLINE IntVector2 operator*(int32 Scalar) const noexcept
    {
        return IntVector2(X * Scalar, Y * Scalar);
    }

    /**
     * @brief Multiplies each component of this vector by a scalar.
     * @param Scalar The scalar value to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE IntVector2& operator*=(int32 Scalar) noexcept
    {
        X *= Scalar;
        Y *= Scalar;
        return *this;
    }

    /**
     * @brief Divides another vector component-wise.
     * @param Other The vector to divide by.
     * @return A new vector representing the quotient.
     */
    FORCEINLINE IntVector2 operator/(const IntVector2& Other) const noexcept
    {
        CHECK(Other.X != 0 && Other.Y != 0);
        return IntVector2(X / Other.X, Y / Other.Y);
    }

    /**
     * @brief Divides another vector component-wise with this vector.
     * @param Other The vector to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE IntVector2& operator/=(const IntVector2& Other) noexcept
    {
        CHECK(Other.X != 0 && Other.Y != 0);
        X /= Other.X;
        Y /= Other.Y;
        return *this;
    }

    /**
     * @brief Divides each component by a scalar.
     * @param Scalar The scalar value to divide by.
     * @return A new vector with each component divided by the scalar.
     */
    FORCEINLINE IntVector2 operator/(int32 Scalar) const noexcept
    {
        CHECK(Scalar != 0);
        return IntVector2(X / Scalar, Y / Scalar);
    }

    /**
     * @brief Divides each component of this vector by a scalar.
     * @param Scalar The scalar value to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE IntVector2& operator/=(int32 Scalar) noexcept
    {
        CHECK(Scalar != 0);
        X /= Scalar;
        Y /= Scalar;
        return *this;
    }

    /**
     * @brief Compares this vector with another for equality.
     * @param Other The vector to compare against.
     * @return True if both components are equal; otherwise, false.
     */
    FORCEINLINE bool operator==(const IntVector2& Other) const noexcept
    {
        return (X == Other.X) && (Y == Other.Y);
    }

    /**
     * @brief Compares this vector with another for inequality.
     * @param Other The vector to compare against.
     * @return True if not equal; otherwise, false.
     */
    FORCEINLINE bool operator!=(const IntVector2& Other) const noexcept
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
    static FORCEINLINE IntVector2 Min(const IntVector2& LHS, const IntVector2& RHS) noexcept
    {
        return IntVector2(Math::Min(LHS.X, RHS.X), Math::Min(LHS.Y, RHS.Y));
    }

    /**
     * @brief Returns the component-wise maximum of two vectors.
     * @param LHS First vector.
     * @param RHS Second vector.
     * @return A new vector containing the maximum of each component.
     */
    static FORCEINLINE IntVector2 Max(const IntVector2& LHS, const IntVector2& RHS) noexcept
    {
        return IntVector2(Math::Max(LHS.X, RHS.X), Math::Max(LHS.Y, RHS.Y));
    }

    /**
     * @brief Clamps each component of a vector between the corresponding components of min and max vectors.
     * @param Value The vector to clamp.
     * @param Min The minimum bounds vector.
     * @param Max The maximum bounds vector.
     * @return A new vector with each component clamped.
     */
    static FORCEINLINE IntVector2 Clamp(const IntVector2& Value, const IntVector2& Min, const IntVector2& Max) noexcept
    {
        return IntVector2(Math::Clamp(Value.X, Min.X, Max.X), Math::Clamp(Value.Y, Min.Y, Max.Y));
    }

public:

    /**
     * @brief Adds a scalar to each component of the vector (scalar on left-hand side).
     * @param Scalar The scalar value to add.
     * @param Vector The vector to add the scalar to.
     * @return A new vector with each component increased by the scalar.
     */
    FORCEINLINE friend IntVector2 operator+(int32 Scalar, const IntVector2& Vector) noexcept
    {
        return Vector + Scalar;
    }

    /**
     * @brief Subtracts a vector from a scalar (scalar on left-hand side).
     * @param Scalar The scalar value.
     * @param Vector The vector to subtract from the scalar.
     * @return A new vector with each component being Scalar minus the original component.
     */
    FORCEINLINE friend IntVector2 operator-(int32 Scalar, const IntVector2& Vector) noexcept
    {
        return IntVector2(Scalar - Vector.X, Scalar - Vector.Y);
    }

    /**
     * @brief Multiplies each component of the vector by a scalar (scalar on left-hand side).
     * @param Scalar The scalar value to multiply with.
     * @param Vector The vector to multiply.
     * @return A new vector with each component multiplied by the scalar.
     */
    FORCEINLINE friend IntVector2 operator*(int32 Scalar, const IntVector2& Vector) noexcept
    {
        return Vector * Scalar;
    }

    /**
     * @brief Divides a scalar by each component of the vector (scalar on left-hand side).
     * @param Scalar The scalar value to divide.
     * @param Vector The vector whose components divide the scalar.
     * @return A new vector with each component being Scalar divided by the original component.
     */
    FORCEINLINE friend IntVector2 operator/(int32 Scalar, const IntVector2& Vector) noexcept
    {
        CHECK(Vector.X != 0 && Vector.Y != 0);
        return IntVector2(Scalar / Vector.X, Scalar / Vector.Y);
    }

public:

    /**
     * @brief Accesses the component at the specified index.
     * @param Index The component index (0 for X, 1 for Y).
     * @return Reference to the component.
     */
    FORCEINLINE int32& operator[](int32 Index) noexcept
    {
        CHECK(Index >= 0 && Index < 2);
        return XY[Index];
    }

    /**
     * @brief Accesses the component at the specified index (const version).
     * @param Index The component index (0 for X, 1 for Y).
     * @return The component value.
     */
    FORCEINLINE int32 operator[](int32 Index) const noexcept
    {
        CHECK(Index >= 0 && Index < 2);
        return XY[Index];
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
        };

        int32 XY[2];
    };
};

MARK_AS_REALLOCATABLE(IntVector2);
