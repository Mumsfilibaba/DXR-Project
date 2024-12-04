#pragma once
#include "Core/Math/Matrix4.h"

/** @brief 3x4 Matrix class with float components. Represents a 3D affine transformation matrix. */
class VECTOR_ALIGN FMatrix3x4
{
public:

    /** @brief Default constructor (Initializes all components to zero) */
    FORCEINLINE FMatrix3x4() noexcept
        : M{ {0.0f, 0.0f, 0.0f, 0.0f},
             {0.0f, 0.0f, 0.0f, 0.0f},
             {0.0f, 0.0f, 0.0f, 0.0f} }
    {
    }

    /**
     * @brief Constructor initializing all values with corresponding values
     * @param M00 Value to set on row 0 and column 0
     * @param M01 Value to set on row 0 and column 1
     * @param M02 Value to set on row 0 and column 2
     * @param M03 Value to set on row 0 and column 3
     * @param M10 Value to set on row 1 and column 0
     * @param M11 Value to set on row 1 and column 1
     * @param M12 Value to set on row 1 and column 2
     * @param M13 Value to set on row 1 and column 3
     * @param M20 Value to set on row 2 and column 0
     * @param M21 Value to set on row 2 and column 1
     * @param M22 Value to set on row 2 and column 2
     * @param M23 Value to set on row 2 and column 3
     */
    FORCEINLINE explicit FMatrix3x4(
        float M00, float M01, float M02, float M03,
        float M10, float M11, float M12, float M13,
        float M20, float M21, float M22, float M23) noexcept
        : M{ {M00, M01, M02, M03},
             {M10, M11, M12, M13},
             {M20, M21, M22, M23} }
    {
    }

    /**
     * @brief Constructor initializing all components from a 4x4 matrix
     * @param InMatrix A 4x4 matrix to initialize this matrix from
     */
    FORCEINLINE explicit FMatrix3x4(const FMatrix4& InMatrix) noexcept
    {
        FMemory::Memcpy(M[0], InMatrix.M[0], sizeof(M));
    }

    /**
     * @brief Checks whether this matrix has any value that equals NaN
     * @return True if any value equals NaN, false otherwise
     */
    FORCEINLINE bool ContainsNaN() const noexcept
    {
        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 4; ++Col)
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
     * @return True if any value equals infinity, false otherwise
     */
    FORCEINLINE bool ContainsInfinity() const noexcept
    {
        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 4; ++Col)
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
    FORCEINLINE bool IsEqual(const FMatrix3x4& Other, float Epsilon = FMath::kIsEqualEpsilon) const noexcept
    {
        Epsilon = FMath::Abs(Epsilon);

        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 4; ++Col)
            {
                float Diff = M[Row][Col] - Other.M[Row][Col];
                if (FMath::Abs(Diff) > Epsilon)
                {
                    return false;
                }
            }
        }

        return true;
    }

public:

    /**
     * @brief Compares this matrix with another matrix for equality
     * @param Other The matrix to compare with
     * @return True if matrices are equal within a default epsilon, false otherwise
     */
    FORCEINLINE bool operator==(const FMatrix3x4& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief Compares this matrix with another matrix for inequality
     * @param Other The matrix to compare with
     * @return True if matrices are not equal within a default epsilon, false otherwise
     */
    FORCEINLINE bool operator!=(const FMatrix3x4& Other) const noexcept
    {
        return !IsEqual(Other);
    }

public:

    /** @brief Each element of the matrix stored as a 2-D array. M[row][column], where row and column are 0-based indices. */
    float M[3][4];
};

MARK_AS_REALLOCATABLE(FMatrix3x4);
