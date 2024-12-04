#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/VectorMath/VectorMath.h"

class VECTOR_ALIGN FVector4
{
public:

    /**
     * @brief Default constructor (initializes components to zero).
     */
    FORCEINLINE FVector4() noexcept
        : X(0.0f)
        , Y(0.0f)
        , Z(0.0f)
        , W(0.0f)
    {
    }

    /**
     * @brief Constructor initializing all components with specific values.
     * @param InX The X-coordinate.
     * @param InY The Y-coordinate.
     * @param InZ The Z-coordinate.
     * @param InW The W-coordinate.
     */
    FORCEINLINE explicit FVector4(float InX, float InY, float InZ, float InW) noexcept
        : X(InX)
        , Y(InY)
        , Z(InZ)
        , W(InW)
    {
    }

    /**
     * @brief Constructor initializing all components with a single value.
     * @param Scalar Value to set all components to.
     */
    FORCEINLINE explicit FVector4(float Scalar) noexcept
        : X(Scalar)
        , Y(Scalar)
        , Z(Scalar)
        , W(Scalar)
    {
    }

    /**
     * @brief Constructor copying a 3D vector into the first components, setting W-component to zero.
     * @param XYZ Value to set first components to.
     */
    FORCEINLINE FVector4(const FVector3& XYZ) noexcept
        : X(XYZ.X)
        , Y(XYZ.Y)
        , Z(XYZ.Z)
        , W(0.0f)
    {
    }

    /**
     * @brief Constructor copying a 3D vector into the first components, setting W-component to a specific value.
     * @param XYZ Value to set first components to.
     * @param InW Value to set the W-component to.
     */
    FORCEINLINE explicit FVector4(const FVector3& XYZ, float InW) noexcept
        : X(XYZ.X)
        , Y(XYZ.Y)
        , Z(XYZ.Z)
        , W(InW)
    {
    }

    /**
     * @brief Normalizes this vector.
     */
    inline void Normalize() noexcept
    {
    #if !USE_VECTOR_MATH
        const float LengthSqrd = GetLengthSquared();
        if (LengthSqrd != 0.0f)
        {
            const float RcpLength = 1.0f / FMath::Sqrt(LengthSqrd);
            X *= RcpLength;
            Y *= RcpLength;
            Z *= RcpLength;
            W *= RcpLength;
        }
    #else
        FFloat128 XYZW_128     = FVectorMath::VectorLoad(XYZW);
        FFloat128 XYZWSqrd_128 = FVectorMath::VectorDot(XYZW_128, XYZW_128);

        const float LengthSqrd = FVectorMath::VectorGetX(XYZWSqrd_128);
        if (LengthSqrd != 0.0f)
        {
            FFloat128 RcpLength_128 = FVectorMath::VectorRecipSqrt(XYZWSqrd_128);
            FFloat128 Result_128    = FVectorMath::VectorMul(XYZWSqrd_128, RcpLength_128);
            FVectorMath::VectorStore(Result_128, XYZW);
        }
    #endif
    }

    /**
     * @brief Returns a normalized version of this vector.
     * @return A normalized copy of this vector.
     */
    FORCEINLINE FVector4 GetNormalized() const noexcept
    {
        FVector4 Result(*this);
        Result.Normalize();
        return Result;
    }

    /**
     * @brief Calculates the distance to another vector.
     * @param Other The other vector.
     * @return The distance between this vector and the other vector.
     */
    FORCEINLINE float GetDistanceTo(const FVector4& Other) const noexcept
    {
        return (*this - Other).GetLength();
    }

    /**
     * @brief Calculates the squared distance to another vector.
     * @param Other The other vector.
     * @return The squared distance between this vector and the other vector.
     */
    FORCEINLINE float GetDistanceSquaredTo(const FVector4& Other) const noexcept
    {
        return (*this - Other).GetLengthSquared();
    }

    /**
     * @brief Calculates the angle between this vector and another vector.
     * @param Other The other vector.
     * @return The angle in radians between the two vectors.
     */
    FORCEINLINE float GetAngleBetween(const FVector4& Other) const noexcept
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
    FORCEINLINE FVector4 GetReflected(const FVector4& Normal) const noexcept
    {
        return *this - 2.0f * DotProduct(Normal) * Normal;
    }

    /**
     * @brief Compares this vector with another vector within a specified threshold.
     * @param Other Vector to compare against.
     * @param Epsilon The threshold for comparison.
     * @return True if vectors are approximately equal, false otherwise.
     */
    inline bool IsEqual(const FVector4& Other, float Epsilon = FMath::kIsEqualEpsilon) const noexcept
    {
    #if !USE_VECTOR_MATH
        Epsilon = FMath::Abs(Epsilon);

        for (int32 Index = 0; Index < 4; ++Index)
        {
            float Diff = XYZW[Index] - Other.XYZW[Index];
            if (FMath::Abs(Diff) > Epsilon)
            {
                return false;
            }
        }

        return true;
    #else
        FFloat128 Epsilon_128 = FVectorMath::VectorSet1(Epsilon);
        Epsilon_128 = FVectorMath::VectorAbs(Epsilon_128);

        FFloat128 XYZW_128 = FVectorMath::VectorSub(XYZW, Other.XYZW);
        XYZW_128 = FVectorMath::VectorAbs(XYZW_128);

        return FVectorMath::VectorLessThan(XYZW_128, Epsilon_128);
    #endif
    }

    /**
     * @brief Checks whether this vector is a unit vector.
     * @return True if the length equals one, false otherwise.
     */
    FORCEINLINE bool IsUnitVector() const noexcept
    {
        const float LengthDiff = FMath::Abs(1.0f - GetLengthSquared());
        return LengthDiff < FMath::kIsEqualEpsilon;
    }

    /**
     * @brief Checks whether this vector contains any NaN components.
     * @return True if any component equals NaN, false otherwise.
     */
    FORCEINLINE bool ContainsNaN() const noexcept
    {
        for (int32 Index = 0; Index < 4; ++Index)
        {
            if (FMath::IsNaN(XYZW[Index]))
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
        for (int32 Index = 0; Index < 4; ++Index)
        {
            if (FMath::IsInfinity(XYZW[Index]))
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
    FORCEINLINE float DotProduct(const FVector4& Other) const noexcept
    {
    #if !USE_VECTOR_MATH
        return (X * Other.X) + (Y * Other.Y) + (Z * Other.Z) + (W * Other.W);
    #else
        FFloat128 XYZW_128   = FVectorMath::VectorLoad(XYZW);
        FFloat128 Other_128  = FVectorMath::VectorLoad(Other.XYZW);
        FFloat128 Result_128 = FVectorMath::VectorDot(XYZW_128, Other_128);
        return FVectorMath::VectorGetX(Result_128);
    #endif
    }

    /**
     * @brief Calculates the cross product between this vector and another vector.
     * @note This function does not take the W-component into account.
     * @param Other The vector to perform the cross product with.
     * @return The cross product vector.
     */
    FORCEINLINE FVector4 CrossProduct(const FVector4& Other) const noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4((Y * Other.Z) - (Z * Other.Y), (Z * Other.X) - (X * Other.Z), (X * Other.Y) - (Y * Other.X), 0.0f);
    #else
        FFloat128 XYZW_128   = FVectorMath::VectorLoad(XYZW);
        FFloat128 Other_128  = FVectorMath::VectorLoad(Other.XYZW);
        FFloat128 Cross_128  = FVectorMath::VectorCross(XYZW_128, Other_128);
        FInt128   Mask_128   = FVectorMath::VectorSetInt(~0, ~0, ~0, 0);
        FFloat128 Result_128 = FVectorMath::VectorAnd(Cross_128, FVectorMath::VectorIntToFloat(Mask_128));
        FVectorMath::VectorStore(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Projects this vector onto another vector.
     * @param Other The vector to project onto.
     * @return The projected vector.
     */
    FORCEINLINE FVector4 ProjectOn(const FVector4& Other) const noexcept
    {
        FVector4 Projected;

    #if !USE_VECTOR_MATH
        float AdotB = DotProduct(Other);
        float BdotB = Other.GetLengthSquared();

        if (BdotB == 0.0f)
        {
            Projected = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
        }
        else
        {
            Projected = (AdotB / BdotB) * Other;
        }
    #else
        FFloat128 XYZW_128   = FVectorMath::VectorLoad(XYZW);
        FFloat128 Other_128  = FVectorMath::VectorLoad(Other.XYZW);
        FFloat128 AdotB_128  = FVectorMath::VectorDot(XYZW_128, Other_128);
        FFloat128 VectorA    = FVectorMath::VectorShuffle<0, 0, 0, 0>(AdotB_128);
        FFloat128 BdotB_128  = FVectorMath::VectorDot(Other_128, Other_128);
        FFloat128 VectorB    = FVectorMath::VectorShuffle<0, 0, 0, 0>(BdotB_128);
        FFloat128 VectorC    = FVectorMath::VectorDiv(VectorA, VectorB);
        FFloat128 Result_128 = FVectorMath::VectorMul(VectorC, Other_128);
        FVectorMath::VectorStore(Result_128, Projected.XYZW);
    #endif

        return Projected;
    }

public:

    /**
     * @brief Returns a vector with the smallest components of two vectors.
     * @param ValueA First vector to compare.
     * @param ValueB Second vector to compare.
     * @return A vector with the smallest components.
     */
    static FORCEINLINE FVector4 Min(const FVector4& ValueA, const FVector4& ValueB) noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(FMath::Min(ValueA.X, ValueB.X), FMath::Min(ValueA.Y, ValueB.Y), FMath::Min(ValueA.Z, ValueB.Z), FMath::Min(ValueA.W, ValueB.W));
    #else
        FFloat128 VectorA    = FVectorMath::VectorLoad(ValueA.XYZW);
        FFloat128 VectorB    = FVectorMath::VectorLoad(ValueB.XYZW);
        FFloat128 Result_128 = FVectorMath::VectorMin(VectorA, VectorB);
        FVectorMath::VectorStore(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Returns a vector with the largest components of two vectors.
     * @param First First vector to compare.
     * @param Second Second vector to compare.
     * @return A vector with the largest components.
     */
    static FORCEINLINE FVector4 Max(const FVector4& First, const FVector4& Second) noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(FMath::Max(First.X, Second.X), FMath::Max(First.Y, Second.Y), FMath::Max(First.Z, Second.Z), FMath::Max(First.W, Second.W));
    #else
        FFloat128 VectorA    = FVectorMath::VectorLoad(First.XYZW);
        FFloat128 VectorB    = FVectorMath::VectorLoad(Second.XYZW);
        FFloat128 Result_128 = FVectorMath::VectorMax(VectorA, VectorB);
        FVectorMath::VectorStore(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Performs linear interpolation between two vectors.
     * @param ValueA First vector.
     * @param ValueB Second vector.
     * @param Factor Interpolation factor (0 returns ValueA, 1 returns ValueB).
     * @return The interpolated vector.
     */
    static FORCEINLINE FVector4 Lerp(const FVector4& ValueA, const FVector4& ValueB, float Factor) noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4((1.0f - Factor) * ValueA.X + Factor * ValueB.X, (1.0f - Factor) * ValueA.Y + Factor * ValueB.Y, (1.0f - Factor) * ValueA.Z + Factor * ValueB.Z, (1.0f - Factor) * ValueA.W + Factor * ValueB.W);
    #else
        FFloat128 VectorA            = FVectorMath::VectorLoad(ValueA.XYZW);
        FFloat128 VectorB            = FVectorMath::VectorLoad(ValueB.XYZW);
        FFloat128 Factor_128         = FVectorMath::VectorSet1(Factor);
        FFloat128 OneMinusFactor_128 = FVectorMath::VectorSub(FVectorMath::VectorOne(), Factor_128);
        FFloat128 VectorC            = FVectorMath::VectorMul(VectorA, OneMinusFactor_128);
        FFloat128 VectorD            = FVectorMath::VectorMul(VectorB, Factor_128);
        FFloat128 Result_128         = FVectorMath::VectorAdd(VectorC, VectorD);
        FVectorMath::VectorStore(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Clamps the components of a vector within specified ranges.
     * @param Value The vector to clamp.
     * @param Min Vector containing the minimum values.
     * @param Max Vector containing the maximum values.
     * @return The clamped vector.
     */
    static FORCEINLINE FVector4 Clamp(const FVector4& Value, const FVector4& Min, const FVector4& Max) noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(FMath::Clamp(Value.X, Min.X, Max.X), FMath::Clamp(Value.Y, Min.Y, Max.Y), FMath::Clamp(Value.Z, Min.Z, Max.Z), FMath::Clamp(Value.W, Min.W, Max.W));
    #else
        FFloat128 Value_128 = FVectorMath::VectorLoad(Value.XYZW);
        FFloat128 Min_128   = FVectorMath::VectorLoad(Min.XYZW);
        FFloat128 Max_128   = FVectorMath::VectorLoad(Max.XYZW);
        FFloat128 VectorA   = FVectorMath::VectorMax(Value_128, Min_128);
        FFloat128 VectorB   = FVectorMath::VectorMin(VectorA, Max_128);
        FVectorMath::VectorStore(VectorB, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Saturates the components of a vector to the range [0, 1].
     * @param Value The vector to saturate.
     * @return The saturated vector.
     */
    static FORCEINLINE FVector4 Saturate(const FVector4& Value) noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(FMath::Saturate(Value.X), FMath::Saturate(Value.Y), FMath::Saturate(Value.Z), FMath::Saturate(Value.W));
    #else
        FFloat128 Value_128 = FVectorMath::VectorLoad(Value.XYZW);
        FFloat128 Zeros_128 = FVectorMath::VectorZero();
        FFloat128 Ones_128  = FVectorMath::VectorOne();
        FFloat128 VectorA   = FVectorMath::VectorMax(Value_128, Zeros_128);
        FFloat128 VectorB   = FVectorMath::VectorMin(VectorA, Ones_128);
        FVectorMath::VectorStore(VectorB, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Converts vector components from radians to degrees.
     * @param Radians Vector in radians.
     * @return Vector with components in degrees.
     */
    static FORCEINLINE FVector4 ToDegrees(const FVector4& Radians) noexcept
    {
        return FVector4(FMath::ToDegrees(Radians.X), FMath::ToDegrees(Radians.Y), FMath::ToDegrees(Radians.Z), FMath::ToDegrees(Radians.W));
    }

    /**
     * @brief Converts vector components from degrees to radians.
     * @param Degrees Vector in degrees.
     * @return Vector with components in radians.
     */
    static FORCEINLINE FVector4 ToRadians(const FVector4& Degrees) noexcept
    {
        return FVector4(FMath::ToRadians(Degrees.X), FMath::ToRadians(Degrees.Y), FMath::ToRadians(Degrees.Z), FMath::ToRadians(Degrees.W));
    }

public:

    /**
     * @brief Returns a vector with negated components.
     * @return The negated vector.
     */
    FORCEINLINE FVector4 operator-() const noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(-X, -Y, -Z, -W);
    #else
        FFloat128 Zero_128   = FVectorMath::VectorZero();
        FFloat128 XYZW_128   = FVectorMath::VectorLoad(XYZW);
        FFloat128 Result_128 = FVectorMath::VectorSub(Zero_128, XYZW_128);
        FVectorMath::VectorStore(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Adds two vectors component-wise.
     * @param Other The vector to add.
     * @return The result of the addition.
     */
    FORCEINLINE FVector4 operator+(const FVector4& Other) const noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(X + Other.X, Y + Other.Y, Z + Other.Z, W + Other.W);
    #else
        FFloat128 XYZW_128   = FVectorMath::VectorLoad(XYZW);
        FFloat128 Other_128  = FVectorMath::VectorLoad(Other.XYZW);
        FFloat128 Result_128 = FVectorMath::VectorAdd(XYZW_128, Other_128);
        FVectorMath::VectorStore(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Adds another vector to this vector component-wise.
     * @param Other The vector to add.
     * @return Reference to this vector after addition.
     */
    FORCEINLINE FVector4& operator+=(const FVector4& Other) noexcept
    {
    #if !USE_VECTOR_MATH
        X += Other.X;
        Y += Other.Y;
        Z += Other.Z;
        W += Other.W;
    #else
        FFloat128 XYZW_128   = FVectorMath::VectorLoad(XYZW);
        FFloat128 Other_128  = FVectorMath::VectorLoad(Other.XYZW);
        FFloat128 Result_128 = FVectorMath::VectorAdd(XYZW_128, Other_128);
        FVectorMath::VectorStore(Result_128, XYZW);
    #endif

        return *this;
    }

    /**
     * @brief Adds a scalar to each component of this vector.
     * @param Scalar The scalar to add.
     * @return The result of the addition.
     */
    FORCEINLINE FVector4 operator+(float Scalar) const noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(X + Scalar, Y + Scalar, Z + Scalar, W + Scalar);
    #else
        FFloat128 XYZW_128    = FVectorMath::VectorLoad(XYZW);
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128  = FVectorMath::VectorAdd(XYZW_128, Scalars_128);
        FVectorMath::VectorStore(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Adds a scalar to each component of this vector.
     * @param Scalar The scalar to add.
     * @return Reference to this vector after addition.
     */
    FORCEINLINE FVector4& operator+=(float Scalar) noexcept
    {
    #if !USE_VECTOR_MATH
        X += Scalar;
        Y += Scalar;
        Z += Scalar;
        W += Scalar;
    #else
        FFloat128 XYZW_128    = FVectorMath::VectorLoad(XYZW);
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128  = FVectorMath::VectorAdd(XYZW_128, Scalars_128);
        FVectorMath::VectorStore(Result_128, XYZW);
    #endif

        return *this;
    }

    /**
     * @brief Subtracts another vector from this vector component-wise.
     * @param Other The vector to subtract.
     * @return The result of the subtraction.
     */
    FORCEINLINE FVector4 operator-(const FVector4& Other) const noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(X - Other.X, Y - Other.Y, Z - Other.Z, W - Other.W);
    #else
        FFloat128 XYZW_128   = FVectorMath::VectorLoad(XYZW);
        FFloat128 Other_128  = FVectorMath::VectorLoad(Other.XYZW);
        FFloat128 Result_128 = FVectorMath::VectorSub(XYZW_128, Other_128);
        FVectorMath::VectorStore(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Subtracts another vector from this vector component-wise.
     * @param Other The vector to subtract.
     * @return Reference to this vector after subtraction.
     */
    FORCEINLINE FVector4& operator-=(const FVector4& Other) noexcept
    {
    #if !USE_VECTOR_MATH
        X -= Other.X;
        Y -= Other.Y;
        Z -= Other.Z;
        W -= Other.W;
    #else
        FFloat128 XYZW_128   = FVectorMath::VectorLoad(XYZW);
        FFloat128 Other_128  = FVectorMath::VectorLoad(Other.XYZW);
        FFloat128 Result_128 = FVectorMath::VectorSub(XYZW_128, Other_128);
        FVectorMath::VectorStore(Result_128, XYZW);
    #endif

        return *this;
    }

    /**
     * @brief Subtracts a scalar from each component of this vector.
     * @param Scalar The scalar to subtract.
     * @return The result of the subtraction.
     */
    FORCEINLINE FVector4 operator-(float Scalar) const noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(X - Scalar, Y - Scalar, Z - Scalar, W - Scalar);
    #else
        FFloat128 XYZW_128    = FVectorMath::VectorLoad(XYZW);
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128  = FVectorMath::VectorSub(XYZW_128, Scalars_128);
        FVectorMath::VectorStore(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Subtracts a scalar from each component of this vector.
     * @param Scalar The scalar to subtract.
     * @return Reference to this vector after subtraction.
     */
    FORCEINLINE FVector4& operator-=(float Scalar) noexcept
    {
    #if !USE_VECTOR_MATH
        X -= Scalar;
        Y -= Scalar;
        Z -= Scalar;
        W -= Scalar;
    #else
        FFloat128 XYZW_128    = FVectorMath::VectorLoad(XYZW);
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128  = FVectorMath::VectorSub(XYZW_128, Scalars_128);
        FVectorMath::VectorStore(Result_128, XYZW);
    #endif

        return *this;
    }

    /**
     * @brief Multiplies this vector with another vector component-wise.
     * @param Other The vector to multiply with.
     * @return The result of the multiplication.
     */
    FORCEINLINE FVector4 operator*(const FVector4& Other) const noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(X * Other.X, Y * Other.Y, Z * Other.Z, W * Other.W);
    #else
        FFloat128 XYZW_128   = FVectorMath::VectorLoad(XYZW);
        FFloat128 Other_128  = FVectorMath::VectorLoad(Other.XYZW);
        FFloat128 Result_128 = FVectorMath::VectorMul(XYZW_128, Other_128);
        FVectorMath::VectorStore(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Multiplies this vector with another vector component-wise.
     * @param Other The vector to multiply with.
     * @return Reference to this vector after multiplication.
     */
    FORCEINLINE FVector4& operator*=(const FVector4& Other) noexcept
    {
    #if !USE_VECTOR_MATH
        X *= Other.X;
        Y *= Other.Y;
        Z *= Other.Z;
        W *= Other.W;
    #else
        FFloat128 XYZW_128   = FVectorMath::VectorLoad(XYZW);
        FFloat128 Other_128  = FVectorMath::VectorLoad(Other.XYZW);
        FFloat128 Result_128 = FVectorMath::VectorMul(XYZW_128, Other_128);
        FVectorMath::VectorStore(Result_128, XYZW);
    #endif

        return *this;
    }

    /**
     * @brief Multiplies each component of this vector by a scalar.
     * @param Scalar The scalar to multiply with.
     * @return The result of the multiplication.
     */
    FORCEINLINE FVector4 operator*(float Scalar) const noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(X * Scalar, Y * Scalar, Z * Scalar, W * Scalar);
    #else
        FFloat128 XYZW_128    = FVectorMath::VectorLoad(XYZW);
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128  = FVectorMath::VectorMul(XYZW_128, Scalars_128);
        FVectorMath::VectorStore(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Multiplies each component of this vector by a scalar.
     * @param Scalar The scalar to multiply with.
     * @return Reference to this vector after multiplication.
     */
    FORCEINLINE FVector4& operator*=(float Scalar) noexcept
    {
    #if !USE_VECTOR_MATH
        X *= Scalar;
        Y *= Scalar;
        Z *= Scalar;
        W *= Scalar;
    #else
        FFloat128 XYZW_128    = FVectorMath::VectorLoad(XYZW);
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128  = FVectorMath::VectorMul(XYZW_128, Scalars_128);
        FVectorMath::VectorStore(Result_128, XYZW);
    #endif

        return *this;
    }

    /**
     * @brief Multiplies a scalar with a vector (friend function).
     * @param Scalar The scalar to multiply.
     * @param Other The vector to multiply with.
     * @return The result of the multiplication.
     */
    friend FORCEINLINE FVector4 operator*(float Scalar, const FVector4& Other) noexcept
    {
        return Other * Scalar;
    }

    /**
     * @brief Divides this vector by another vector component-wise.
     * @param Other The vector to divide by.
     * @return The result of the division.
     */
    FORCEINLINE FVector4 operator/(const FVector4& Other) const noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(X / Other.X, Y / Other.Y, Z / Other.Z, W / Other.W);
    #else
        FFloat128 XYZW_128   = FVectorMath::VectorLoad(XYZW);
        FFloat128 Other_128  = FVectorMath::VectorLoad(Other.XYZW);
        FFloat128 Result_128 = FVectorMath::VectorDiv(XYZW_128, Other_128);
        FVectorMath::VectorStore(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Divides this vector by another vector component-wise.
     * @param Other The vector to divide by.
     * @return Reference to this vector after division.
     */
    FORCEINLINE FVector4& operator/=(const FVector4& Other) noexcept
    {
    #if !USE_VECTOR_MATH
        X /= Other.X;
        Y /= Other.Y;
        Z /= Other.Z;
        W /= Other.W;
    #else
        FFloat128 XYZW_128   = FVectorMath::VectorLoad(XYZW);
        FFloat128 Other_128  = FVectorMath::VectorLoad(Other.XYZW);
        FFloat128 Result_128 = FVectorMath::VectorDiv(XYZW_128, Other_128);
        FVectorMath::VectorStore(Result_128, XYZW);
    #endif

        return *this;
    }

    /**
     * @brief Divides each component of this vector by a scalar.
     * @param Scalar The scalar to divide by.
     * @return The result of the division.
     */
    FORCEINLINE FVector4 operator/(float Scalar) const noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result = FVector4(X / Scalar, Y / Scalar, Z / Scalar, W / Scalar);
    #else
        FFloat128 XYZW_128    = FVectorMath::VectorLoad(XYZW);
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128  = FVectorMath::VectorDiv(XYZW_128, Scalars_128);
        FVectorMath::VectorStore(Result_128, Result.XYZW);
    #endif

        return Result;
    }

    /**
     * @brief Divides each component of this vector by a scalar.
     * @param Scalar The scalar to divide by.
     * @return Reference to this vector after division.
     */
    FORCEINLINE FVector4& operator/=(float Scalar) noexcept
    {
    #if !USE_VECTOR_MATH
        X /= Scalar;
        Y /= Scalar;
        Z /= Scalar;
        W /= Scalar;
    #else
        FFloat128 XYZW_128    = FVectorMath::VectorLoad(XYZW);
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128  = FVectorMath::VectorDiv(XYZW_128, Scalars_128);
        FVectorMath::VectorStore(Result_128, XYZW);
    #endif

        return *this;
    }

    /**
     * @brief Checks if this vector is equal to another vector.
     * @param Other The vector to compare with.
     * @return True if equal, false otherwise.
     */
    FORCEINLINE bool operator==(const FVector4& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief Checks if this vector is not equal to another vector.
     * @param Other The vector to compare with.
     * @return True if not equal, false otherwise.
     */
    FORCEINLINE bool operator!=(const FVector4& Other) const noexcept
    {
        return !IsEqual(Other);
    }

public:

    /**
     * @brief Accesses a component of the vector by index.
     * @param Index The component index (0 for X, 1 for Y, 2 for Z, 3 for W).
     * @return Reference to the component.
     */
    FORCEINLINE float& operator[](int32 Index) noexcept
    {
        CHECK(Index >= 0 && Index < 4);
        return XYZW[Index];
    }

    /**
     * @brief Accesses a component of the vector by index.
     * @param Index The component index (0 for X, 1 for Y, 2 for Z, 3 for W).
     * @return The component value.
     */
    FORCEINLINE float operator[](int32 Index) const noexcept
    {
        CHECK(Index >= 0 && Index < 4);
        return XYZW[Index];
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

            /** @brief The W-coordinate. */
            float W;
        };

        /** @brief An array containing the X, Y, Z, and W components. */
        float XYZW[4];
    };
};

MARK_AS_REALLOCATABLE(FVector4);
