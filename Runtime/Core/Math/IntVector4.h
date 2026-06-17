#pragma once
#include "Core/Math/Math.h"
#include "Core/Math/VectorMath/VectorMath.h"

class Int16Vector4
{
public:

    /** @brief Default constructor initializes all components to zero. */
    FORCEINLINE Int16Vector4() noexcept
        : X(0)
        , Y(0)
        , Z(0)
        , W(0)
    {
    }

    /**
     * @brief Constructs the vector with specified X, Y, Z, and W components.
     * @param InX The X-coordinate.
     * @param InY The Y-coordinate.
     * @param InZ The Z-coordinate.
     * @param InW The W-coordinate.
     */
    FORCEINLINE Int16Vector4(int16 InX, int16 InY, int16 InZ, int16 InW) noexcept
        : X(InX)
        , Y(InY)
        , Z(InZ)
        , W(InW)
    {
    }

    /**
     * @brief Constructs the vector by setting X, Y, Z, and W to the same scalar value.
     * @param Scalar The scalar value to set all components.
     */
    FORCEINLINE explicit Int16Vector4(int16 Scalar) noexcept
        : X(Scalar)
        , Y(Scalar)
        , Z(Scalar)
        , W(Scalar)
    {
    }

public:

    /**
     * @brief Returns a negated vector.
     * @return A new vector with each component negated.
     */
    FORCEINLINE Int16Vector4 operator-() const noexcept
    {
        return Int16Vector4(-X, -Y, -Z, -W);
    }

    /**
     * @brief Adds another vector component-wise.
     * @param Other The vector to add.
     * @return A new vector representing the sum.
     */
    FORCEINLINE Int16Vector4 operator+(const Int16Vector4& Other) const noexcept
    {
        return Int16Vector4(X + Other.X, Y + Other.Y, Z + Other.Z, W + Other.W);
    }

    /**
     * @brief Adds another vector component-wise to this vector.
     * @param Other The vector to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE Int16Vector4& operator+=(const Int16Vector4& Other) noexcept
    {
        X += Other.X;
        Y += Other.Y;
        Z += Other.Z;
        W += Other.W;
        return *this;
    }

    /**
     * @brief Adds a scalar to each component.
     * @param Scalar The scalar value to add.
     * @return A new vector with each component increased by the scalar.
     */
    FORCEINLINE Int16Vector4 operator+(int16 Scalar) const noexcept
    {
        return Int16Vector4(X + Scalar, Y + Scalar, Z + Scalar, W + Scalar);
    }

    /**
     * @brief Adds a scalar to each component of this vector.
     * @param Scalar The scalar value to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE Int16Vector4& operator+=(int16 Scalar) noexcept
    {
        X += Scalar;
        Y += Scalar;
        Z += Scalar;
        W += Scalar;
        return *this;
    }

    /**
     * @brief Subtracts another vector component-wise.
     * @param Other The vector to subtract.
     * @return A new vector representing the difference.
     */
    FORCEINLINE Int16Vector4 operator-(const Int16Vector4& Other) const noexcept
    {
        return Int16Vector4(X - Other.X, Y - Other.Y, Z - Other.Z, W - Other.W);
    }

    /**
     * @brief Subtracts another vector component-wise from this vector.
     * @param Other The vector to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE Int16Vector4& operator-=(const Int16Vector4& Other) noexcept
    {
        X -= Other.X;
        Y -= Other.Y;
        Z -= Other.Z;
        W -= Other.W;
        return *this;
    }

    /**
     * @brief Subtracts a scalar from each component.
     * @param Scalar The scalar value to subtract.
     * @return A new vector with each component decreased by the scalar.
     */
    FORCEINLINE Int16Vector4 operator-(int16 Scalar) const noexcept
    {
        return Int16Vector4(X - Scalar, Y - Scalar, Z - Scalar, W - Scalar);
    }

    /**
     * @brief Subtracts a scalar from each component of this vector.
     * @param Scalar The scalar value to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE Int16Vector4& operator-=(int16 Scalar) noexcept
    {
        X -= Scalar;
        Y -= Scalar;
        Z -= Scalar;
        W -= Scalar;
        return *this;
    }

    /**
     * @brief Multiplies another vector component-wise.
     * @param Other The vector to multiply with.
     * @return A new vector representing the product.
     */
    FORCEINLINE Int16Vector4 operator*(const Int16Vector4& Other) const noexcept
    {
        return Int16Vector4(X * Other.X, Y * Other.Y, Z * Other.Z, W * Other.W);
    }

    /**
     * @brief Multiplies another vector component-wise with this vector.
     * @param Other The vector to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE Int16Vector4& operator*=(const Int16Vector4& Other) noexcept
    {
        X *= Other.X;
        Y *= Other.Y;
        Z *= Other.Z;
        W *= Other.W;
        return *this;
    }

    /**
     * @brief Multiplies each component by a scalar.
     * @param Scalar The scalar value to multiply with.
     * @return A new vector with each component multiplied by the scalar.
     */
    FORCEINLINE Int16Vector4 operator*(int16 Scalar) const noexcept
    {
        return Int16Vector4(X * Scalar, Y * Scalar, Z * Scalar, W * Scalar);
    }

    /**
     * @brief Multiplies each component of this vector by a scalar.
     * @param Scalar The scalar value to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE Int16Vector4& operator*=(int16 Scalar) noexcept
    {
        X *= Scalar;
        Y *= Scalar;
        Z *= Scalar;
        W *= Scalar;
        return *this;
    }

    /**
     * @brief Divides another vector component-wise.
     * @param Other The vector to divide by.
     * @return A new vector representing the quotient.
     */
    FORCEINLINE Int16Vector4 operator/(const Int16Vector4& Other) const noexcept
    {
        CHECK(Other.X != 0 && Other.Y != 0 && Other.Z != 0 && Other.W);
        return Int16Vector4(X / Other.X, Y / Other.Y, Z / Other.Z, W / Other.W);
    }

    /**
     * @brief Divides another vector component-wise with this vector.
     * @param Other The vector to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE Int16Vector4& operator/=(const Int16Vector4& Other) noexcept
    {
        CHECK(Other.X != 0 && Other.Y != 0 && Other.Z != 0 && Other.W);
        X /= Other.X;
        Y /= Other.Y;
        Z /= Other.Z;
        W /= Other.W;
        return *this;
    }

    /**
     * @brief Divides each component by a scalar.
     * @param Scalar The scalar value to divide by.
     * @return A new vector with each component divided by the scalar.
     */
    FORCEINLINE Int16Vector4 operator/(int16 Scalar) const noexcept
    {
        CHECK(Scalar != 0);
        return Int16Vector4(X / Scalar, Y / Scalar, Z / Scalar, W / Scalar);
    }

    /**
     * @brief Divides each component of this vector by a scalar.
     * @param Scalar The scalar value to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE Int16Vector4& operator/=(int16 Scalar) noexcept
    {
        CHECK(Scalar != 0);
        X /= Scalar;
        Y /= Scalar;
        Z /= Scalar;
        W /= Scalar;
        return *this;
    }

    /**
     * @brief Compares this vector with another for equality.
     * @param Other The vector to compare against.
     * @return True if all components are equal; otherwise, false.
     */
    FORCEINLINE bool operator==(const Int16Vector4& Other) const noexcept
    {
        return (X == Other.X) && (Y == Other.Y) && (Z == Other.Z) && (W == Other.W);
    }

    /**
     * @brief Compares this vector with another for inequality.
     * @param Other The vector to compare against.
     * @return True if any component is not equal; otherwise, false.
     */
    FORCEINLINE bool operator!=(const Int16Vector4& Other) const noexcept
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
    static FORCEINLINE Int16Vector4 Min(const Int16Vector4& LHS, const Int16Vector4& RHS) noexcept
    {
        return Int16Vector4(Math::Min(LHS.X, RHS.X), Math::Min(LHS.Y, RHS.Y), Math::Min(LHS.Z, RHS.Z), Math::Min(LHS.W, RHS.W));
    }

    /**
     * @brief Returns the component-wise maximum of two vectors.
     * @param LHS First vector.
     * @param RHS Second vector.
     * @return A new vector containing the maximum of each component.
     */
    static FORCEINLINE Int16Vector4 Max(const Int16Vector4& LHS, const Int16Vector4& RHS) noexcept
    {
        return Int16Vector4(Math::Max(LHS.X, RHS.X), Math::Max(LHS.Y, RHS.Y), Math::Max(LHS.Z, RHS.Z), Math::Max(LHS.W, RHS.W));
    }

    /**
     * @brief Clamps each component of a vector between the corresponding components of min and max vectors.
     * @param Value The vector to clamp.
     * @param Min The minimum bounds vector.
     * @param Max The maximum bounds vector.
     * @return A new vector with each component clamped.
     */
    static FORCEINLINE Int16Vector4 Clamp(const Int16Vector4& Value, const Int16Vector4& Min, const Int16Vector4& Max) noexcept
    {
        return Int16Vector4(Math::Clamp(Value.X, Min.X, Max.X), Math::Clamp(Value.Y, Min.Y, Max.Y), Math::Clamp(Value.Z, Min.Z, Max.Z), Math::Clamp(Value.W, Min.W, Max.W));
    }

public:

    /**
     * @brief Adds a scalar to each component of the vector (scalar on left-hand side).
     * @param Scalar The scalar value to add.
     * @param Vector The vector to add the scalar to.
     * @return A new vector with each component increased by the scalar.
     */
    FORCEINLINE friend Int16Vector4 operator+(int16 Scalar, const Int16Vector4& Vector) noexcept
    {
        return Vector + Scalar;
    }

    /**
     * @brief Subtracts a vector from a scalar (scalar on left-hand side).
     * @param Scalar The scalar value.
     * @param Vector The vector to subtract from the scalar.
     * @return A new vector with each component being Scalar minus the original component.
     */
    FORCEINLINE friend Int16Vector4 operator-(int16 Scalar, const Int16Vector4& Vector) noexcept
    {
        return Int16Vector4(Scalar - Vector.X, Scalar - Vector.Y, Scalar - Vector.Z, Scalar - Vector.W);
    }

    /**
     * @brief Multiplies each component of the vector by a scalar (scalar on left-hand side).
     * @param Scalar The scalar value to multiply with.
     * @param Vector The vector to multiply.
     * @return A new vector with each component multiplied by the scalar.
     */
    FORCEINLINE friend Int16Vector4 operator*(int16 Scalar, const Int16Vector4& Vector) noexcept
    {
        return Vector * Scalar;
    }

    /**
     * @brief Divides a scalar by each component of the vector (scalar on left-hand side).
     * @param Scalar The scalar value to divide.
     * @param Vector The vector whose components divide the scalar.
     * @return A new vector with each component being Scalar divided by the original component.
     */
    FORCEINLINE friend Int16Vector4 operator/(int16 Scalar, const Int16Vector4& Vector) noexcept
    {
        CHECK(Vector.X != 0 && Vector.Y != 0 && Vector.Z != 0 && Vector.W != 0);
        return Int16Vector4(Scalar / Vector.X, Scalar / Vector.Y, Scalar / Vector.Z, Scalar / Vector.W);
    }

public:

    /**
     * @brief Accesses the component at the specified index.
     * @param Index The component index (0 for X, 1 for Y, 2 for Z, 3 for W).
     * @return Reference to the component.
     */
    FORCEINLINE int16& operator[](int32 Index) noexcept
    {
        CHECK(Index >= 0 && Index < 4);
        return XYZW[Index];
    }

    /**
     * @brief Accesses the component at the specified index (const version).
     * @param Index The component index (0 for X, 1 for Y, 2 for Z, 3 for W).
     * @return The component value.
     */
    FORCEINLINE int16 operator[](int32 Index) const noexcept
    {
        CHECK(Index >= 0 && Index < 4);
        return XYZW[Index];
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

            /** @brief The W-coordinate */
            int16 W;
        };

        /** @brief Array access to components (0 = X, 1 = Y, 2 = Z, 3 = W) */
        int16 XYZW[4];
    };
};

MARK_AS_REALLOCATABLE(Int16Vector4);

class IntVector4
{
public:

    /** @brief Default constructor initializes all components to zero. */
    FORCEINLINE IntVector4() noexcept
        : X(0)
        , Y(0)
        , Z(0)
        , W(0)
    {
    }

    /**
     * @brief Constructs the vector with specified X, Y, Z, and W components.
     * @param InX The X-coordinate.
     * @param InY The Y-coordinate.
     * @param InZ The Z-coordinate.
     * @param InW The W-coordinate.
     */
    FORCEINLINE IntVector4(int32 InX, int32 InY, int32 InZ, int32 InW) noexcept
        : X(InX)
        , Y(InY)
        , Z(InZ)
        , W(InW)
    {
    }

    /**
     * @brief Constructs the vector by setting X, Y, Z, and W to the same scalar value.
     * @param Scalar The scalar value to set all components.
     */
    FORCEINLINE explicit IntVector4(int32 Scalar) noexcept
        : X(Scalar)
        , Y(Scalar)
        , Z(Scalar)
        , W(Scalar)
    {
    }

public:

    /**
     * @brief Returns a negated vector.
     * @return A new vector with each component negated.
     */
    FORCEINLINE IntVector4 operator-() const noexcept
    {
        IntVector4 Result;

    #if !USE_INT_VECTOR_MATH
        Result = IntVector4(-X, -Y, -Z, -W);
    #else
        FInt128 Zero_128   = FVectorMath::VectorZeroInt();
        FInt128 XYZW_128   = FVectorMath::VectorLoadInt(XYZW);
        FInt128 Result_128 = FVectorMath::VectorSubInt(Zero_128, XYZW_128);
        FVectorMath::VectorStoreInt(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Adds another vector component-wise.
     * @param Other The vector to add.
     * @return A new vector representing the sum.
     */
    FORCEINLINE IntVector4 operator+(const IntVector4& Other) const noexcept
    {
        IntVector4 Result;

    #if !USE_INT_VECTOR_MATH
        Result = IntVector4(X + Other.X, Y + Other.Y, Z + Other.Z, W + Other.W);
    #else
        FInt128 XYZW_128   = FVectorMath::VectorLoadInt(XYZW);
        FInt128 Other_128  = FVectorMath::VectorLoadInt(Other.XYZW);
        FInt128 Result_128 = FVectorMath::VectorAddInt(XYZW_128, Other_128);
        FVectorMath::VectorStoreInt(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Adds another vector component-wise to this vector.
     * @param Other The vector to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE IntVector4& operator+=(const IntVector4& Other) noexcept
    {
    #if !USE_INT_VECTOR_MATH
        X += Other.X;
        Y += Other.Y;
        Z += Other.Z;
        W += Other.W;
    #else
        FInt128 XYZW_128   = FVectorMath::VectorLoadInt(XYZW);
        FInt128 Other_128  = FVectorMath::VectorLoadInt(Other.XYZW);
        FInt128 Result_128 = FVectorMath::VectorAddInt(XYZW_128, Other_128);
        FVectorMath::VectorStoreInt(Result_128, XYZW);
    #endif

        return *this;
    }

    /**
     * @brief Adds a scalar to each component.
     * @param Scalar The scalar value to add.
     * @return A new vector with each component increased by the scalar.
     */
    FORCEINLINE IntVector4 operator+(int32 Scalar) const noexcept
    {
        IntVector4 Result;

    #if !USE_INT_VECTOR_MATH
        Result = IntVector4(X + Scalar, Y + Scalar, Z + Scalar, W + Scalar);
    #else
        FInt128 XYZW_128    = FVectorMath::VectorLoadInt(XYZW);
        FInt128 Scalars_128 = FVectorMath::VectorSetInt1(Scalar);
        FInt128 Result_128  = FVectorMath::VectorAddInt(XYZW_128, Scalars_128);
        FVectorMath::VectorStoreInt(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Adds a scalar to each component of this vector.
     * @param Scalar The scalar value to add.
     * @return A reference to this vector after addition.
     */
    FORCEINLINE IntVector4& operator+=(int32 Scalar) noexcept
    {
    #if !USE_INT_VECTOR_MATH
        X += Scalar;
        Y += Scalar;
        Z += Scalar;
        W += Scalar;
    #else
        FInt128 XYZW_128    = FVectorMath::VectorLoadInt(XYZW);
        FInt128 Scalars_128 = FVectorMath::VectorSetInt1(Scalar);
        FInt128 Result_128  = FVectorMath::VectorAddInt(XYZW_128, Scalars_128);
        FVectorMath::VectorStoreInt(Result_128, XYZW);
    #endif

        return *this;
    }

    /**
     * @brief Subtracts another vector component-wise.
     * @param Other The vector to subtract.
     * @return A new vector representing the difference.
     */
    FORCEINLINE IntVector4 operator-(const IntVector4& Other) const noexcept
    {
        IntVector4 Result;

    #if !USE_INT_VECTOR_MATH
        Result = IntVector4(X - Other.X, Y - Other.Y, Z - Other.Z, W - Other.W);
    #else
        FInt128 XYZW_128   = FVectorMath::VectorLoadInt(XYZW);
        FInt128 Other_128  = FVectorMath::VectorLoadInt(Other.XYZW);
        FInt128 Result_128 = FVectorMath::VectorSubInt(XYZW_128, Other_128);
        FVectorMath::VectorStoreInt(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Subtracts another vector component-wise from this vector.
     * @param Other The vector to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE IntVector4& operator-=(const IntVector4& Other) noexcept
    {
    #if !USE_INT_VECTOR_MATH
        X -= Other.X;
        Y -= Other.Y;
        Z -= Other.Z;
        W -= Other.W;
    #else
        FInt128 XYZW_128   = FVectorMath::VectorLoadInt(XYZW);
        FInt128 Other_128  = FVectorMath::VectorLoadInt(Other.XYZW);
        FInt128 Result_128 = FVectorMath::VectorSubInt(XYZW_128, Other_128);
        FVectorMath::VectorStoreInt(Result_128, XYZW);
    #endif

        return *this;
    }

    /**
     * @brief Subtracts a scalar from each component.
     * @param Scalar The scalar value to subtract.
     * @return A new vector with each component decreased by the scalar.
     */
    FORCEINLINE IntVector4 operator-(int32 Scalar) const noexcept
    {
        IntVector4 Result;

    #if !USE_INT_VECTOR_MATH
        Result = IntVector4(X - Scalar, Y - Scalar, Z - Scalar, W - Scalar);
    #else
        FInt128 XYZW_128    = FVectorMath::VectorLoadInt(XYZW);
        FInt128 Scalars_128 = FVectorMath::VectorSetInt1(Scalar);
        FInt128 Result_128  = FVectorMath::VectorSubInt(XYZW_128, Scalars_128);
        FVectorMath::VectorStoreInt(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Subtracts a scalar from each component of this vector.
     * @param Scalar The scalar value to subtract.
     * @return A reference to this vector after subtraction.
     */
    FORCEINLINE IntVector4& operator-=(int32 Scalar) noexcept
    {
    #if !USE_INT_VECTOR_MATH
        X -= Scalar;
        Y -= Scalar;
        Z -= Scalar;
        W -= Scalar;
    #else
        FInt128 XYZW_128    = FVectorMath::VectorLoadInt(XYZW);
        FInt128 Scalars_128 = FVectorMath::VectorSetInt1(Scalar);
        FInt128 Result_128  = FVectorMath::VectorSubInt(XYZW_128, Scalars_128);
        FVectorMath::VectorStoreInt(Result_128, XYZW);
    #endif

        return *this;
    }

    /**
     * @brief Multiplies another vector component-wise.
     * @param Other The vector to multiply with.
     * @return A new vector representing the product.
     */
    FORCEINLINE IntVector4 operator*(const IntVector4& Other) const noexcept
    {
        IntVector4 Result;

    #if !USE_INT_VECTOR_MATH
        Result = IntVector4(X * Other.X, Y * Other.Y, Z * Other.Z, W * Other.W);
    #else
        FInt128 XYZW_128   = FVectorMath::VectorLoadInt(XYZW);
        FInt128 Other_128  = FVectorMath::VectorLoadInt(Other.XYZW);
        FInt128 Result_128 = FVectorMath::VectorMulInt(XYZW_128, Other_128);
        FVectorMath::VectorStoreInt(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Multiplies another vector component-wise with this vector.
     * @param Other The vector to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE IntVector4& operator*=(const IntVector4& Other) noexcept
    {
    #if !USE_INT_VECTOR_MATH
        X *= Other.X;
        Y *= Other.Y;
        Z *= Other.Z;
        W *= Other.W;
    #else
        FInt128 XYZW_128   = FVectorMath::VectorLoadInt(XYZW);
        FInt128 Other_128  = FVectorMath::VectorLoadInt(Other.XYZW);
        FInt128 Result_128 = FVectorMath::VectorMulInt(XYZW_128, Other_128);
        FVectorMath::VectorStoreInt(Result_128, XYZW);
    #endif

        return *this;
    }

    /**
     * @brief Multiplies each component by a scalar.
     * @param Scalar The scalar value to multiply with.
     * @return A new vector with each component multiplied by the scalar.
     */
    FORCEINLINE IntVector4 operator*(int32 Scalar) const noexcept
    {
        IntVector4 Result;

    #if !USE_INT_VECTOR_MATH
        Result = IntVector4(X * Scalar, Y * Scalar, Z * Scalar, W * Scalar);
    #else
        FInt128 XYZW_128    = FVectorMath::VectorLoadInt(XYZW);
        FInt128 Scalars_128 = FVectorMath::VectorSetInt1(Scalar);
        FInt128 Result_128  = FVectorMath::VectorMulInt(XYZW_128, Scalars_128);
        FVectorMath::VectorStoreInt(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Multiplies each component of this vector by a scalar.
     * @param Scalar The scalar value to multiply with.
     * @return A reference to this vector after multiplication.
     */
    FORCEINLINE IntVector4& operator*=(int32 Scalar) noexcept
    {
    #if !USE_INT_VECTOR_MATH
        X *= Scalar;
        Y *= Scalar;
        Z *= Scalar;
        W *= Scalar;
    #else
        FInt128 XYZW_128    = FVectorMath::VectorLoadInt(XYZW);
        FInt128 Scalars_128 = FVectorMath::VectorSetInt1(Scalar);
        FInt128 Result_128  = FVectorMath::VectorMulInt(XYZW_128, Scalars_128);
        FVectorMath::VectorStoreInt(Result_128, XYZW);
    #endif

        return *this;
    }

    /**
     * @brief Divides another vector component-wise.
     * @param Other The vector to divide by.
     * @return A new vector representing the quotient.
     */
    FORCEINLINE IntVector4 operator/(const IntVector4& Other) const noexcept
    {
        CHECK(Other.X != 0 && Other.Y != 0 && Other.Z != 0 && Other.W != 0);
        return IntVector4(X / Other.X, Y / Other.Y, Z / Other.Z, W / Other.W);
    }

    /**
     * @brief Divides another vector component-wise with this vector.
     * @param Other The vector to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE IntVector4& operator/=(const IntVector4& Other) noexcept
    {
        CHECK(Other.X != 0 && Other.Y != 0 && Other.Z != 0 && Other.W != 0);
        X /= Other.X;
        Y /= Other.Y;
        Z /= Other.Z;
        W /= Other.W;
        return *this;
    }

    /**
     * @brief Divides each component by a scalar.
     * @param Scalar The scalar value to divide by.
     * @return A new vector with each component divided by the scalar.
     */
    FORCEINLINE IntVector4 operator/(int32 Scalar) const noexcept
    {
        CHECK(Scalar != 0);
        return IntVector4(X / Scalar, Y / Scalar, Z / Scalar, W / Scalar);
    }

    /**
     * @brief Divides each component of this vector by a scalar.
     * @param Scalar The scalar value to divide by.
     * @return A reference to this vector after division.
     */
    FORCEINLINE IntVector4& operator/=(int32 Scalar) noexcept
    {
        CHECK(Scalar != 0);
        X /= Scalar;
        Y /= Scalar;
        Z /= Scalar;
        W /= Scalar;
        return *this;
    }

    /**
     * @brief Compares this vector with another for equality.
     * @param Other The vector to compare against.
     * @return True if all components are equal; otherwise, false.
     */
    FORCEINLINE bool operator==(const IntVector4& Other) const noexcept
    {
    #if !USE_INT_VECTOR_MATH
        return (X == Other.X) && (Y == Other.Y) && (Z == Other.Z) && (W == Other.W);
    #else
        FInt128 XYZW_128   = FVectorMath::VectorLoadInt(XYZW);
        FInt128 Other_128  = FVectorMath::VectorLoadInt(Other.XYZW);
        return FVectorMath::VectorEqualInt(XYZW_128, Other_128);
    #endif
    }

    /**
     * @brief Compares this vector with another for inequality.
     * @param Other The vector to compare against.
     * @return True if any component is not equal; otherwise, false.
     */
    FORCEINLINE bool operator!=(const IntVector4& Other) const noexcept
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
    static FORCEINLINE IntVector4 Min(const IntVector4& LHS, const IntVector4& RHS) noexcept
    {
        IntVector4 Result;

    #if !USE_INT_VECTOR_MATH
        Result = IntVector4(Math::Min(LHS.X, RHS.X), Math::Min(LHS.Y, RHS.Y), Math::Min(LHS.Z, RHS.Z), Math::Min(LHS.W, RHS.W));
    #else
        FInt128 LHS_128    = FVectorMath::VectorLoadInt(LHS.XYZW);
        FInt128 RHS_128    = FVectorMath::VectorLoadInt(RHS.XYZW);
        FInt128 Result_128 = FVectorMath::VectorMinInt(LHS_128, RHS_128);
        FVectorMath::VectorStoreInt(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Returns the component-wise maximum of two vectors.
     * @param LHS First vector.
     * @param RHS Second vector.
     * @return A new vector containing the maximum of each component.
     */
    static FORCEINLINE IntVector4 Max(const IntVector4& LHS, const IntVector4& RHS) noexcept
    {
        IntVector4 Result;

    #if !USE_INT_VECTOR_MATH
        Result = IntVector4(Math::Max(LHS.X, RHS.X), Math::Max(LHS.Y, RHS.Y), Math::Max(LHS.Z, RHS.Z), Math::Max(LHS.W, RHS.W));
    #else
        FInt128 LHS_128    = FVectorMath::VectorLoadInt(LHS.XYZW);
        FInt128 RHS_128    = FVectorMath::VectorLoadInt(RHS.XYZW);
        FInt128 Result_128 = FVectorMath::VectorMaxInt(LHS_128, RHS_128);
        FVectorMath::VectorStoreInt(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Clamps each component of a vector between the corresponding components of min and max vectors.
     * @param Value The vector to clamp.
     * @param Min The minimum bounds vector.
     * @param Max The maximum bounds vector.
     * @return A new vector with each component clamped.
     */
    static FORCEINLINE IntVector4 Clamp(const IntVector4& Value, const IntVector4& Min, const IntVector4& Max) noexcept
    {
        IntVector4 Result;

    #if !USE_INT_VECTOR_MATH
        Result = IntVector4(Math::Clamp(Value.X, Min.X, Max.X), Math::Clamp(Value.Y, Min.Y, Max.Y), Math::Clamp(Value.Z, Min.Z, Max.Z), Math::Clamp(Value.W, Min.W, Max.W));
    #else
        FInt128 Value_128  = FVectorMath::VectorLoadInt(Value.XYZW);
        FInt128 Min_128    = FVectorMath::VectorLoadInt(Min.XYZW);
        FInt128 Max_128    = FVectorMath::VectorLoadInt(Max.XYZW);
        FInt128 Result_128 = FVectorMath::VectorClampInt(Value_128, Min_128, Max_128);
        FVectorMath::VectorStoreInt(Result_128, Result.XYZW);
    #endif

        return Result;
    }

public:

    /**
     * @brief Adds a scalar to each component of the vector (scalar on left-hand side).
     * @param Scalar The scalar value to add.
     * @param Vector The vector to add the scalar to.
     * @return A new vector with each component increased by the scalar.
     */
    FORCEINLINE friend IntVector4 operator+(int32 Scalar, const IntVector4& Vector) noexcept
    {
        return Vector + Scalar;
    }

    /**
     * @brief Subtracts a vector from a scalar (scalar on left-hand side).
     * @param Scalar The scalar value.
     * @param Vector The vector to subtract from the scalar.
     * @return A new vector with each component being Scalar minus the original component.
     */
    FORCEINLINE friend IntVector4 operator-(int32 Scalar, const IntVector4& Vector) noexcept
    {
        IntVector4 Result;

    #if !USE_INT_VECTOR_MATH
        Result = IntVector4(Scalar - Vector.X, Scalar - Vector.Y, Scalar - Vector.Z, Scalar - Vector.W);
    #else
        FInt128 Vector_128  = FVectorMath::VectorLoadInt(Vector.XYZW);
        FInt128 Scalars_128 = FVectorMath::VectorSetInt1(Scalar);
        FInt128 Result_128  = FVectorMath::VectorSubInt(Scalars_128, Vector_128);
        FVectorMath::VectorStoreInt(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Multiplies each component of the vector by a scalar (scalar on left-hand side).
     * @param Scalar The scalar value to multiply with.
     * @param Vector The vector to multiply.
     * @return A new vector with each component multiplied by the scalar.
     */
    FORCEINLINE friend IntVector4 operator*(int32 Scalar, const IntVector4& Vector) noexcept
    {
        return Vector * Scalar;
    }

    /**
     * @brief Divides a scalar by each component of the vector (scalar on left-hand side).
     * @param Scalar The scalar value to divide.
     * @param Vector The vector whose components divide the scalar.
     * @return A new vector with each component being Scalar divided by the original component.
     */
    FORCEINLINE friend IntVector4 operator/(int32 Scalar, const IntVector4& Vector) noexcept
    {
        CHECK(Vector.X != 0 && Vector.Y != 0 && Vector.Z != 0 && Vector.W != 0);
        return IntVector4(Scalar / Vector.X, Scalar / Vector.Y, Scalar / Vector.Z, Scalar / Vector.W);
    }

public:

    /**
     * @brief Accesses the component at the specified index.
     * @param Index The component index (0 for X, 1 for Y, 2 for Z, 3 for W).
     * @return Reference to the component.
     */
    FORCEINLINE int32& operator[](int32 Index) noexcept
    {
        CHECK(Index >= 0 && Index < 4);
        return XYZW[Index];
    }

    /**
     * @brief Accesses the component at the specified index (const version).
     * @param Index The component index (0 for X, 1 for Y, 2 for Z, 3 for W).
     * @return The component value.
     */
    FORCEINLINE int32 operator[](int32 Index) const noexcept
    {
        CHECK(Index >= 0 && Index < 4);
        return XYZW[Index];
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

            /** @brief The W-coordinate */
            int32 W;
        };

        int32 XYZW[4];
    };
};

MARK_AS_REALLOCATABLE(IntVector4);
