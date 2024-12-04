#pragma once
#include "Core/Math/Vector2.h"
#include "Core/Math/VectorMath/VectorMath.h"

/** @brief 2D Matrix class with float components. */
class VECTOR_ALIGN FMatrix2
{
public:

    /** @brief Default constructor (Initialize components to zero) */
    FORCEINLINE FMatrix2() noexcept 
        : M{ { 0.0f, 0.0f }, { 0.0f, 0.0f } }
    {
    }

    /**
     * @brief Constructor initializing all values on the diagonal with a single value. The other values are set to zero.
     * @param Diagonal Value to set on the diagonal
     */
    FORCEINLINE explicit FMatrix2(float Diagonal) noexcept
        : M{ { Diagonal, 0.0f }, { 0.0f, Diagonal } }
    {
    }

    /**
     * @brief Constructor initializing all values with vectors specifying each row
     * @param Row0 Vector to set the first row to
     * @param Row1 Vector to set the second row to
     */
    FORCEINLINE explicit FMatrix2(const FVector2& Row0, const FVector2& Row1) noexcept
        : M{ { Row0.X, Row0.Y }, { Row1.X, Row1.Y } }
    {
    }
    
    /**
     * @brief Constructor initializing all values with corresponding values
     * @param M00 Value to set on row 0 and column 0
     * @param M01 Value to set on row 0 and column 1
     * @param M10 Value to set on row 1 and column 0
     * @param M11 Value to set on row 1 and column 1
     */
    FORCEINLINE explicit FMatrix2(float M00, float M01, float M10, float M11) noexcept
        : M{ { M00, M01 }, { M10, M11 } }
    {
    }

    /**
     * @brief Returns the transposed version of this matrix
     * @return Transposed matrix
     */
    FORCEINLINE FMatrix2 GetTranspose() const noexcept
    {
        FMatrix2 Transpose;

    #if !USE_VECTOR_MATH
        Transpose.M[0][0] = M[0][0];
        Transpose.M[0][1] = M[1][0];
        Transpose.M[1][0] = M[0][1];
        Transpose.M[1][1] = M[1][1];
    #else
        FFloat128 M_128      = FVectorMath::VectorLoad(&M[0][0]);
        FFloat128 Result_128 = FVectorMath::VectorShuffle<0, 2, 1, 3>(M_128);
        FVectorMath::VectorStore(Result_128, &Transpose.M[0][0]);
    #endif

        return Transpose;
    }

    /**
     * @brief Returns the inverted version of this matrix
     * @return Inverse matrix
     */
    FORCEINLINE FMatrix2 GetInverse() const noexcept
    {
        FMatrix2 Inverse;

        const float Determinant = GetDeterminant();
        CHECK(Determinant != 0.0f);

    #if !USE_VECTOR_MATH
        const float RcpDeterminant = 1.0f / Determinant;

        Inverse.M[0][0] =  M[1][1] * RcpDeterminant;
        Inverse.M[1][0] = -M[1][0] * RcpDeterminant;
        Inverse.M[0][1] = -M[0][1] * RcpDeterminant;
        Inverse.M[1][1] =  M[0][0] * RcpDeterminant;
    #else
        FFloat128 M_128   = FVectorMath::VectorLoad(&M[0][0]);
        FFloat128 VectorA = FVectorMath::VectorShuffle<3, 2, 1, 0>(M_128);

        constexpr int32 KeepMask   = 0;
        constexpr int32 NegateMask = (1 << 31);

        FInt128   Mask_128       = FVectorMath::VectorSetInt(KeepMask, NegateMask, NegateMask, KeepMask);
        FFloat128 VectorB        = FVectorMath::VectorOr(VectorA, FVectorMath::VectorIntToFloat(Mask_128));
        FFloat128 RcpDeterminant = FVectorMath::VectorRecip(FVectorMath::VectorSet1(Determinant));
        FFloat128 Result_128     = FVectorMath::VectorMul(VectorB, RcpDeterminant);
        FVectorMath::VectorStore(Result_128, &Inverse.M[0][0]);
    #endif

        return Inverse;
    }

    /**
     * @brief Returns the adjugate of this matrix
     * @return Adjugate matrix
     */
    FORCEINLINE FMatrix2 GetAdjoint() const noexcept
    {
        FMatrix2 Adjugate;

    #if !USE_VECTOR_MATH
        Adjugate.M[0][0] =  M[1][1];
        Adjugate.M[1][0] = -M[1][0];
        Adjugate.M[0][1] = -M[0][1];
        Adjugate.M[1][1] =  M[0][0];
    #else
        FFloat128 M_128   = FVectorMath::VectorLoad(&M[0][0]);
        FFloat128 VectorA = FVectorMath::VectorShuffle<3, 2, 1, 0>(M_128);

        constexpr int32 KeepMask   = 0;
        constexpr int32 NegateMask = (1 << 31);

        FInt128   Mask_128   = FVectorMath::VectorSetInt(KeepMask, NegateMask, NegateMask, KeepMask);
        FFloat128 Result_128 = FVectorMath::VectorOr(VectorA, FVectorMath::VectorIntToFloat(Mask_128));
        FVectorMath::VectorStore(Result_128, &Adjugate.M[0][0]);
    #endif

        return Adjugate;
    }

    /**
     * @brief Returns the determinant of this matrix
     * @return The determinant
     */
    FORCEINLINE float GetDeterminant() const noexcept
    {
        return (M[0][0] * M[1][1]) - (M[0][1] * M[1][0]);
    }

    /**
     * @brief Checks whether this matrix has any value that equals NaN
     * @return True if any value equals NaN, false if not
     */
    FORCEINLINE bool ContainsNaN() const noexcept
    {
        for (int32 Row = 0; Row < 2; ++Row)
        {
            for (int32 Col = 0; Col < 2; ++Col)
            {
                if (FMath::IsNaN(M[Row][Col]))
                {
                    return true;
                }
            }
        }

        return false;
    }

    /**
     * @brief Checks whether this matrix has any value that equals infinity
     * @return True if any value equals infinity, false if not
     */
    FORCEINLINE bool ContainsInfinity() const noexcept
    {
        for (int32 Row = 0; Row < 2; ++Row)
        {
            for (int32 Col = 0; Col < 2; ++Col)
            {
                if (FMath::IsInfinity(M[Row][Col]))
                {
                    return true;
                }
            }
        }

        return false;
    }

    /**
     * @brief Compares, within a threshold Epsilon, this matrix with another matrix
     * @param Other Matrix to compare against
     * @param Epsilon Threshold for comparison
     * @return True if equal within Epsilon, false otherwise
     */
    FORCEINLINE bool IsEqual(const FMatrix2& Other, float Epsilon = FMath::kIsEqualEpsilon) const noexcept
    {
    #if !USE_VECTOR_MATH
        Epsilon = FMath::Abs(Epsilon);
        for (int32 Row = 0; Row < 2; ++Row)
        {
            for (int32 Col = 0; Col < 2; ++Col)
            {
                float Diff = M[Row][Col] - Other.M[Row][Col];
                if (FMath::Abs(Diff) > Epsilon)
                {
                    return false;
                }
            }
        }

        return true;
    #else
        FFloat128 Epsilon_128 = FVectorMath::VectorSet1(Epsilon);
        FFloat128 VectorA     = FVectorMath::VectorAbs(Epsilon_128);
        FFloat128 VectorB     = FVectorMath::VectorSub(&M[0][0], &Other.M[0][0]);
        FFloat128 Result_128  = FVectorMath::VectorAbs(VectorB);
        return FVectorMath::VectorAllLessThan(Result_128, Epsilon_128);
    #endif
    }

    /** 
     * @brief Sets this matrix to an identity matrix 
     */
    FORCEINLINE void SetIdentity() noexcept
    {
    #if !USE_VECTOR_MATH
        M[0][0] = 1.0f;
        M[0][1] = 0.0f;
        M[1][0] = 0.0f;
        M[1][1] = 1.0f;
    #else
        FFloat128 Identity_128 = FVectorMath::VectorSet(1.0f, 0.0f, 0.0f, 1.0f);
        FVectorMath::VectorStore(Identity_128, &M[0][0]);
    #endif
    }

    /**
     * @brief Returns a row of this matrix
     * @param Row The row to retrieve (0 or 1)
     * @return A vector containing the specified row
     */
    FORCEINLINE FVector2 GetRow(int32 Row) const noexcept
    {
        CHECK(Row < 2);
        return FVector2(M[Row][0], M[Row][1]);
    }

    /**
     * @brief Returns a column of this matrix
     * @param Column The column to retrieve (0 or 1)
     * @return A vector containing the specified column
     */
    FORCEINLINE FVector2 GetColumn(int32 Column) const noexcept
    {
        CHECK(Column < 2);
        return FVector2(M[0][Column], M[1][Column]);
    }

public:

    /**
     * @brief Transforms a 2-D vector
     * @param Vector The vector to transform
     * @return A vector containing the transformation
     */
    FORCEINLINE FVector2 operator*(const FVector2& Vector) const noexcept
    {
        FVector2 Result;

    #if !USE_VECTOR_MATH
        Result.x = (Vector.x * M[0][0]) + (Vector.y * M[1][0]);
        Result.y = (Vector.x * M[0][1]) + (Vector.y * M[1][1]);
    #else
        FFloat128 X_128      = FVectorMath::VectorSet1(Vector.X);
        FFloat128 Y_128      = FVectorMath::VectorSet1(Vector.Y);
        FFloat128 VectorA    = FVectorMath::VectorShuffle0011<0, 0, 0, 0>(X_128, Y_128);
        FFloat128 VectorB    = FVectorMath::VectorMul(&M[0][0], VectorA);
        FFloat128 VectorC    = FVectorMath::VectorShuffle<2, 3, 2, 3>(VectorB);
        FFloat128 Result_128 = FVectorMath::VectorAdd(VectorC, VectorB);

        Result.X = FVectorMath::VectorGetX(Result_128);
        Result.Y = FVectorMath::VectorGetY(Result_128);
    #endif

        return Result;
    }

    /**
     * @brief Multiplies a matrix with another matrix
     * @param Other The other matrix
     * @return A matrix containing the result of the multiplication
     */
    FORCEINLINE FMatrix2 operator*(const FMatrix2& Other) const noexcept
    {
        FMatrix2 Result;

    #if !USE_VECTOR_MATH
        Result.M[0][0] = (M[0][0] * Other.M[0][0]) + (M[0][1] * Other.M[1][0]);
        Result.M[0][1] = (M[0][1] * Other.M[1][1]) + (M[0][0] * Other.M[0][1]);
        Result.M[1][0] = (M[1][0] * Other.M[0][0]) + (M[1][1] * Other.M[1][0]);
        Result.M[1][1] = (M[1][1] * Other.M[1][1]) + (M[1][0] * Other.M[0][1]);
    #else
        FFloat128 M_128      = FVectorMath::VectorLoad(&M[0][0]);
        FFloat128 Other_128  = FVectorMath::VectorLoad(&Other.M[0][0]);
        FFloat128 Result_128 = FVectorMath::MatrixMul2x2(M_128, Other_128);
        FVectorMath::VectorStore(Result_128, &Result.M[0][0]);
    #endif

        return Result;
    }

    /**
     * @brief Multiplies this matrix with another matrix
     * @param Other The other matrix
     * @return A reference to this matrix after multiplication
     */
    FORCEINLINE FMatrix2& operator*=(const FMatrix2& Other) noexcept
    {
    #if !USE_VECTOR_MATH
        M[0][0] = (M[0][0] * Other.M[0][0]) + (M[0][1] * Other.M[1][0]);
        M[0][1] = (M[0][1] * Other.M[1][1]) + (M[0][0] * Other.M[0][1]);
        M[1][0] = (M[1][0] * Other.M[0][0]) + (M[1][1] * Other.M[1][0]);
        M[1][1] = (M[1][1] * Other.M[1][1]) + (M[1][0] * Other.M[0][1]);
    #else
        FFloat128 M_128      = FVectorMath::VectorLoad(&M[0][0]);
        FFloat128 Other_128  = FVectorMath::VectorLoad(&Other.M[0][0]);
        FFloat128 Result_128 = FVectorMath::MatrixMul2x2(M_128, Other_128);
        FVectorMath::VectorStore(Result_128, &M[0][0]);
    #endif

        return *this;
    }

    /**
     * @brief Multiplies a matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A matrix containing the result of the multiplication
     */
    FORCEINLINE FMatrix2 operator*(float Scalar) const noexcept
    {
        FMatrix2 Result;

    #if !USE_VECTOR_MATH
        Result.M[0][0] = M[0][0] * Scalar;
        Result.M[0][1] = M[0][1] * Scalar;
        Result.M[1][0] = M[1][0] * Scalar;
        Result.M[1][1] = M[1][1] * Scalar;
    #else
        FFloat128 VectorA = FVectorMath::VectorSet1(Scalar);
        FFloat128 VectorB = FVectorMath::VectorMul(&M[0][0], VectorA);
        FVectorMath::VectorStore(VectorB, &Result.M[0][0]);
    #endif

        return Result;
    }

    /**
     * @brief Multiplies this matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A reference to this matrix after multiplication
     */
    FORCEINLINE FMatrix2& operator*=(float Scalar) noexcept
    {
    #if !USE_VECTOR_MATH
        M[0][0] = M[0][0] * Scalar;
        M[0][1] = M[0][1] * Scalar;
        M[1][0] = M[1][0] * Scalar;
        M[1][1] = M[1][1] * Scalar;
    #else
        FFloat128 VectorA = FVectorMath::VectorSet1(Scalar);
        FFloat128 VectorB = FVectorMath::VectorMul(&M[0][0], VectorA);
        FVectorMath::VectorStore(VectorB, &M[0][0]);
    #endif

        return *this;
    }

    /**
     * @brief Adds a matrix component-wise with another matrix
     * @param Other The other matrix
     * @return A matrix containing the result of the addition
     */
    FORCEINLINE FMatrix2 operator+(const FMatrix2& Other) const noexcept
    {
        FMatrix2 Result;

    #if !USE_VECTOR_MATH
        Result.M[0][0] = M[0][0] + Other.M[0][0];
        Result.M[0][1] = M[0][1] + Other.M[0][1];
        Result.M[1][0] = M[1][0] + Other.M[1][0];
        Result.M[1][1] = M[1][1] + Other.M[1][1];
    #else
        FFloat128 M_128 = FVectorMath::VectorAdd(&M[0][0], &Other.M[0][0]);
        FVectorMath::VectorStore(M_128, &Result.M[0][0]);
    #endif

        return Result;
    }

    /**
     * @brief Adds this matrix component-wise with another matrix
     * @param Other The other matrix
     * @return A reference to this matrix after addition
     */
    FORCEINLINE FMatrix2& operator+=(const FMatrix2& Other) noexcept
    {
    #if !USE_VECTOR_MATH
        Result.M[0][0] = M[0][0] + Other.M[0][0];
        Result.M[0][1] = M[0][1] + Other.M[0][1];
        Result.M[1][0] = M[1][0] + Other.M[1][0];
        Result.M[1][1] = M[1][1] + Other.M[1][1];
    #else
        FFloat128 M_128 = FVectorMath::VectorAdd(&M[0][0], &Other.M[0][0]);
        FVectorMath::VectorStore(M_128, &M[0][0]);
    #endif

        return *this;
    }

    /**
     * @brief Adds a matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A matrix containing the result of the addition
     */
    FORCEINLINE FMatrix2 operator+(float Scalar) const noexcept
    {
        FMatrix2 Result;

    #if !USE_VECTOR_MATH
        Result.M[0][0] = M[0][0] + Scalar;
        Result.M[0][1] = M[0][1] + Scalar;
        Result.M[1][0] = M[1][0] + Scalar;
        Result.M[1][1] = M[1][1] + Scalar;
    #else
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128  = FVectorMath::VectorAdd(&M[0][0], Scalars_128);
        FVectorMath::VectorStore(Result_128, &Result.M[0][0]);
    #endif

        return Result;
    }

    /**
     * @brief Adds this matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A reference to this matrix after addition
     */
    FORCEINLINE FMatrix2& operator+=(float Scalar) noexcept
    {
    #if !USE_VECTOR_MATH
        M[0][0] = M[0][0] + Scalar;
        M[0][1] = M[0][1] + Scalar;
        M[1][0] = M[1][0] + Scalar;
        M[1][1] = M[1][1] + Scalar;
    #else
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128  = FVectorMath::VectorAdd(&M[0][0], Scalars_128);
        FVectorMath::VectorStore(Result_128, &M[0][0]);
    #endif

        return *this;
    }

    /**
     * @brief Subtracts a matrix component-wise with another matrix
     * @param Other The other matrix
     * @return A matrix containing the result of the subtraction
     */
    FORCEINLINE FMatrix2 operator-(const FMatrix2& Other) const noexcept
    {
        FMatrix2 Result;

    #if !USE_VECTOR_MATH
        Result.M[0][0] = M[0][0] - Other.M[0][0];
        Result.M[0][1] = M[0][1] - Other.M[0][1];
        Result.M[1][0] = M[1][0] - Other.M[1][0];
        Result.M[1][1] = M[1][1] - Other.M[1][1];
    #else
        FFloat128 Result_128 = FVectorMath::VectorSub(&M[0][0], &Other.M[0][0]);
        FVectorMath::VectorStore(Result_128, &Result.M[0][0]);
    #endif

        return Result;
    }

    /**
     * @brief Subtracts this matrix component-wise with another matrix
     * @param Other The other matrix
     * @return A reference to this matrix after subtraction
     */
    FORCEINLINE FMatrix2& operator-=(const FMatrix2& Other) noexcept
    {
    #if !USE_VECTOR_MATH
        M[0][0] = M[0][0] - Other.M[0][0];
        M[0][1] = M[0][1] - Other.M[0][1];
        M[1][0] = M[1][0] - Other.M[1][0];
        M[1][1] = M[1][1] - Other.M[1][1];
    #else
        FFloat128 Result_128 = FVectorMath::VectorSub(&M[0][0], &Other.M[0][0]);
        FVectorMath::VectorStore(Result_128, &M[0][0]);
    #endif

        return *this;
    }

    /**
     * @brief Subtracts a matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A matrix containing the result of the subtraction
     */
    FORCEINLINE FMatrix2 operator-(float Scalar) const noexcept
    {
        FMatrix2 Result;

    #if !USE_VECTOR_MATH
        Result.M[0][0] = M[0][0] - Scalar;
        Result.M[0][1] = M[0][1] - Scalar;
        Result.M[1][0] = M[1][0] - Scalar;
        Result.M[1][1] = M[1][1] - Scalar;
    #else
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128  = FVectorMath::VectorSub(&M[0][0], Scalars_128);
        FVectorMath::VectorStore(Result_128, &Result.M[0][0]);
    #endif

        return Result;
    }

    /**
     * @brief Subtracts this matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A reference to this matrix after subtraction
     */
    FORCEINLINE FMatrix2& operator-=(float Scalar) noexcept
    {
    #if !USE_VECTOR_MATH
        M[0][0] = M[0][0] - Scalar;
        M[0][1] = M[0][1] - Scalar;
        M[1][0] = M[1][0] - Scalar;
        M[1][1] = M[1][1] - Scalar;
    #else
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);
        FFloat128 Result_128  = FVectorMath::VectorSub(&M[0][0], Scalars_128);
        FVectorMath::VectorStore(Result_128, &M[0][0]);
    #endif

        return *this;
    }

public:

    /**
     * @brief Creates and returns an identity matrix
     * @return An identity matrix
     */
    static FORCEINLINE FMatrix2 Identity() noexcept
    {
        return FMatrix2(1.0f);
    }

    /**
     * @brief Creates and returns a uniform scale matrix
     * @param Scale Uniform scale that represents this matrix
     * @return A scale matrix
     */
    static FORCEINLINE FMatrix2 Scale(float Scale) noexcept
    {
        return FMatrix2(Scale);
    }

    /**
     * @brief Creates and returns a scale matrix for each axis
     * @param x Scale for the x-axis
     * @param y Scale for the y-axis
     * @return A scale matrix
     */
    static FORCEINLINE FMatrix2 Scale(float x, float y) noexcept
    {
        return FMatrix2(x, 0.0f, 0.0f, y);
    }

    /**
     * @brief Creates and returns a scale matrix for each axis
     * @param VectorWithScale A vector containing the scale for each axis in the x- and y-components
     * @return A scale matrix
     */
    static FORCEINLINE FMatrix2 Scale(const FVector2& VectorWithScale) noexcept
    {
        return Scale(VectorWithScale.X, VectorWithScale.Y);
    }

    /**
     * @brief Creates and returns a rotation matrix around the z-axis
     * @param Rotation Rotation in radians
     * @return A rotation matrix
     */
    static FORCEINLINE FMatrix2 Rotation(float Rotation) noexcept
    {
        const float SinZ = FMath::Sin(Rotation);
        const float CosZ = FMath::Cos(Rotation);

        return FMatrix2(CosZ, SinZ, -SinZ, CosZ);
    }

public:

    /** @brief Each element of the matrix stored as a 2-D array. M[row][column], where row and column are 0-based indices. */
    float M[2][2];
};

MARK_AS_REALLOCATABLE(FMatrix2);
