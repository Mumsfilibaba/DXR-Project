#pragma once
#include "MathCommon.h"

// TODO: Fill this out properly with functions etc

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// A 3x4 matrix

class CMatrix3x4
{
public:

    /* Default constructor (Initialize components to zero) */
    FORCEINLINE CMatrix3x4() noexcept
        : m00(0.0f), m01(0.0f), m02(0.0f), m03(0.0f)
        , m10(0.0f), m11(0.0f), m12(0.0f), m13(0.0f)
        , m20(0.0f), m21(0.0f), m22(0.0f), m23(0.0f)
    { }

    /**
     * @brief: Constructor initializing all values with corresponding value
     *
     * @param In00: Value to set on row 0 and column 0
     * @param In01: Value to set on row 0 and column 1
     * @param In02: Value to set on row 0 and column 2
     * @param In03: Value to set on row 0 and column 3
     * @param In10: Value to set on row 1 and column 0
     * @param In11: Value to set on row 1 and column 1
     * @param In12: Value to set on row 1 and column 2
     * @param In13: Value to set on row 1 and column 3
     * @param In20: Value to set on row 2 and column 0
     * @param In21: Value to set on row 2 and column 1
     * @param In22: Value to set on row 2 and column 2
     * @param In23: Value to set on row 2 and column 3
     */
    FORCEINLINE explicit CMatrix3x4( float In00, float In01, float In02, float In03
                                   , float In10, float In11, float In12, float In13
                                   , float In20, float In21, float In22, float In23) noexcept
        : m00(m00), m01(m01), m02(m02), m03(m03)
        , m10(m10), m11(m11), m12(m12), m13(m13)
        , m20(m20), m21(m21), m22(m22), m23(m23)
    { }

    /**
     * @brief: Compares, within a threshold Epsilon, this matrix with another matrix
     *
     * @param Other: matrix to compare against
     * @return True if equal, false if not
     */
    inline bool IsEqual(const CMatrix3x4& Other, float Epsilon = NMath::IS_EQUAL_EPISILON) const noexcept
    {
        Epsilon = NMath::Abs(Epsilon);

        for (int i = 0; i < 12; i++)
        {
            const float Diff = reinterpret_cast<const float*>(this)[i] - reinterpret_cast<const float*>(&Other)[i];
            if (NMath::Abs(Diff) > Epsilon)
            {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief: Returns the data of this matrix as a pointer
     *
     * @return A pointer to the data
     */
    FORCEINLINE float* GetData() noexcept { return reinterpret_cast<float*>(this); }

    /**
     * @brief: Returns the data of this matrix as a pointer
     *
     * @return A pointer to the data
     */
    FORCEINLINE const float* GetData() const noexcept { return reinterpret_cast<const float*>(this); }

    /**
     * @brief: Returns the result after comparing this and another matrix
     *
     * @param Other: The matrix to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==(const CMatrix3x4& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief: Returns the negated result after comparing this and another matrix
     *
     * @param Other: The matrix to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=(const CMatrix3x4& Other) const noexcept
    {
        return !IsEqual(Other);
    }

public:
    union
    {
        /* Each element of the matrix */
        struct
        {
            float m00, m01, m02, m03;
            float m10, m11, m12, m13;
            float m20, m21, m22, m23;
        };

        /* 2-D array of the matrix */
        float Elements[3][4];
    };
};
