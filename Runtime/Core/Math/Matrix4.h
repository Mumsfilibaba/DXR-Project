#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Core/Math/Matrix3.h"
#include "Core/Math/MathCommon.h"

/** @brief 4x4 Matrix class with float components. Represents a 3D affine transformation matrix in homogeneous coordinates. */
class VECTOR_ALIGN FMatrix4
{
public:

    /** @brief Default constructor (Initializes all components to zero) */
    FORCEINLINE FMatrix4() noexcept
        : M{ {0.0f, 0.0f, 0.0f, 0.0f},
             {0.0f, 0.0f, 0.0f, 0.0f},
             {0.0f, 0.0f, 0.0f, 0.0f},
             {0.0f, 0.0f, 0.0f, 0.0f} }
    {
    }

    /**
     * @brief Constructor initializing all diagonal values with a single value. Other values are set to zero.
     * @param Diagonal Value to set on the diagonal
     */
    FORCEINLINE explicit FMatrix4(float Diagonal) noexcept
        : M{ { Diagonal, 0.0f,     0.0f,     0.0f },
             { 0.0f,     Diagonal, 0.0f,     0.0f },
             { 0.0f,     0.0f,     Diagonal, 0.0f },
             { 0.0f,     0.0f,     0.0f,     Diagonal } }
    {
    }

    /**
     * @brief Constructor initializing all values with vectors specifying each row
     * @param Row0 Vector to set the first row to
     * @param Row1 Vector to set the second row to
     * @param Row2 Vector to set the third row to
     * @param Row3 Vector to set the fourth row to
     */
    FORCEINLINE explicit FMatrix4(const FVector4& Row0, const FVector4& Row1, const FVector4& Row2, const FVector4& Row3) noexcept
        : M{ { Row0.X, Row0.Y, Row0.Z, Row0.W },
             { Row1.X, Row1.Y, Row1.Z, Row1.W },
             { Row2.X, Row2.Y, Row2.Z, Row2.W },
             { Row3.X, Row3.Y, Row3.Z, Row3.W } }
    {
    }

    /**
     * @brief Constructor initializing all values with corresponding values
     * @param M00 Value to set at row 0, column 0
     * @param M01 Value to set at row 0, column 1
     * @param M02 Value to set at row 0, column 2
     * @param M03 Value to set at row 0, column 3
     * @param M10 Value to set at row 1, column 0
     * @param M11 Value to set at row 1, column 1
     * @param M12 Value to set at row 1, column 2
     * @param M13 Value to set at row 1, column 3
     * @param M20 Value to set at row 2, column 0
     * @param M21 Value to set at row 2, column 1
     * @param M22 Value to set at row 2, column 2
     * @param M23 Value to set at row 2, column 3
     * @param M30 Value to set at row 3, column 0
     * @param M31 Value to set at row 3, column 1
     * @param M32 Value to set at row 3, column 2
     * @param M33 Value to set at row 3, column 3
     */
    FORCEINLINE explicit FMatrix4(
        float M00, float M01, float M02, float M03,
        float M10, float M11, float M12, float M13,
        float M20, float M21, float M22, float M23,
        float M30, float M31, float M32, float M33) noexcept
        : M{ { M00, M01, M02, M03 },
             { M10, M11, M12, M13 },
             { M20, M21, M22, M23 },
             { M30, M31, M32, M33 } }
    {
    }

    /**
     * @brief Constructor initializing all components with an array
     * @param Array Array with at least 16 elements
     */
    FORCEINLINE explicit FMatrix4(const float* Array) noexcept
    {
        CHECK(Array != nullptr);
        FMemory::Memcpy(M[0], Array, sizeof(M));
    }

    /**
     * @brief Transforms a 4-D vector using this matrix
     * @param Vector The vector to transform
     * @return The transformed vector
     */
    FORCEINLINE FVector4 Transform(const FVector4& Vector) const noexcept
    {
        FVector4 Result;

    #if !USE_VECTOR_MATH
        Result.x = (Vector.x * M[0][0]) + (Vector.y * M[1][0]) + (Vector.z * M[2][0]) + (Vector.w * M[3][0]);
        Result.y = (Vector.x * M[0][1]) + (Vector.y * M[1][1]) + (Vector.z * M[2][1]) + (Vector.w * M[3][1]);
        Result.z = (Vector.x * M[0][2]) + (Vector.y * M[1][2]) + (Vector.z * M[2][2]) + (Vector.w * M[3][2]);
        Result.w = (Vector.x * M[0][3]) + (Vector.y * M[1][3]) + (Vector.z * M[2][3]) + (Vector.w * M[3][3]);
    #else
        FFloat128 Vector_128 = FVectorMath::VectorLoad(reinterpret_cast<const float*>(&Vector));
        FFloat128 Result_128 = FVectorMath::VectorTransform(M[0], Vector_128);
        FVectorMath::VectorStore(Result_128, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
    }

    /**
     * @brief Transforms a 3-D vector using this matrix (as a direction, W=0)
     * @param Vector The vector to transform
     * @return The transformed vector
     */
    FORCEINLINE FVector3 Transform(const FVector3& Vector) const noexcept
    {
        FVector3 Result;

    #if !USE_VECTOR_MATH
        Result.x = (Vector.x * M[0][0]) + (Vector.y * M[1][0]) + (Vector.z * M[2][0]) + (1.0f * M[3][0]);
        Result.y = (Vector.x * M[0][1]) + (Vector.y * M[1][1]) + (Vector.z * M[2][1]) + (1.0f * M[3][1]);
        Result.z = (Vector.x * M[0][2]) + (Vector.y * M[1][2]) + (Vector.z * M[2][2]) + (1.0f * M[3][2]);
    #else
        FFloat128 Vector_128 = FVectorMath::VectorSet(Vector.X, Vector.Y, Vector.Z, 1.0f);
        FFloat128 Result_128 = FVectorMath::VectorTransform(M[0], Vector_128);
        Result = FVector3(FVectorMath::VectorGetX(Result_128), FVectorMath::VectorGetY(Result_128), FVectorMath::VectorGetZ(Result_128));
    #endif

        return Result;
    }

    /**
     * @brief Transforms a 3-D vector as position (W=1) and performs perspective division
     * @param Position The position vector to transform
     * @return The transformed vector with perspective division applied
     */
    FORCEINLINE FVector3 TransformCoord(const FVector3& Position) const noexcept
    {
        FVector3 Result;

    #if !USE_VECTOR_MATH
        float ComponentW = (Position.x * M[0][3]) + (Position.y * M[1][3]) + (Position.z * M[2][3]) + (1.0f * M[3][3]);
        ComponentW = 1.0f / ComponentW;

        Result.x = ((Position.x * M[0][0]) + (Position.y * M[1][0]) + (Position.z * M[2][0]) + (1.0f * M[3][0])) * ComponentW;
        Result.y = ((Position.x * M[0][1]) + (Position.y * M[1][1]) + (Position.z * M[2][1]) + (1.0f * M[3][1])) * ComponentW;
        Result.z = ((Position.x * M[0][2]) + (Position.y * M[1][2]) + (Position.z * M[2][2]) + (1.0f * M[3][2])) * ComponentW;
    #else
        FFloat128 Position_128 = FVectorMath::VectorSet(Position.X, Position.Y, Position.Z, 1.0f);
        FFloat128 VectorA      = FVectorMath::VectorTransform(M[0], Position_128);
        FFloat128 VectorB      = FVectorMath::VectorBroadcast<3>(VectorA);
        FFloat128 VectorC      = FVectorMath::VectorOne();
        FFloat128 VectorD      = FVectorMath::VectorDiv(VectorC, VectorB);
        FFloat128 Result_128   = FVectorMath::VectorMul(VectorD, VectorD);
        Result = FVector3(FVectorMath::VectorGetX(Result_128), FVectorMath::VectorGetY(Result_128), FVectorMath::VectorGetZ(Result_128));
    #endif

        return Result;
    }

    /**
     * @brief Transforms a 3-D vector as a normal (w=0)
     * @param Direction The direction vector to transform
     * @return The transformed normal vector
     */
    FORCEINLINE FVector3 TransformNormal(const FVector3& Direction) const noexcept
    {
        FVector3 Result;

    #if !USE_VECTOR_MATH
        Result.x = (Direction.x * M[0][0]) + (Direction.y * M[1][0]) + (Direction.z * M[2][0]);
        Result.y = (Direction.x * M[0][1]) + (Direction.y * M[1][1]) + (Direction.z * M[2][1]);
        Result.z = (Direction.x * M[0][2]) + (Direction.y * M[1][2]) + (Direction.z * M[2][2]);
    #else
        FFloat128 Direction_128 = FVectorMath::VectorSet(Direction.X, Direction.Y, Direction.Z, 0.0f);
        FFloat128 Result_128    = FVectorMath::VectorTransform(M[0], Direction_128);
        Result = FVector3(FVectorMath::VectorGetX(Result_128), FVectorMath::VectorGetY(Result_128), FVectorMath::VectorGetZ(Result_128));
    #endif

        return Result;
    }

    /**
     * @brief Returns the transposed version of this matrix
     * @return Transposed matrix
     */
    FORCEINLINE FMatrix4 GetTranspose() const noexcept
    {
        FMatrix4 Result;

    #if !USE_VECTOR_MATH
        Result.M[0][0] = M[0][0];
        Result.M[0][1] = M[1][0];
        Result.M[0][2] = M[2][0];
        Result.M[0][3] = M[3][0];

        Result.M[1][0] = M[0][1];
        Result.M[1][1] = M[1][1];
        Result.M[1][2] = M[2][1];
        Result.M[1][3] = M[3][1];

        Result.M[2][0] = M[0][2];
        Result.M[2][1] = M[1][2];
        Result.M[2][2] = M[2][2];
        Result.M[2][3] = M[3][2];

        Result.M[3][0] = M[0][3];
        Result.M[3][1] = M[1][3];
        Result.M[3][2] = M[2][3];
        Result.M[3][3] = M[3][3];
    #else
        FVectorMath::MatrixTranspose4x4(M[0], Result.M[0]);
    #endif

        return Result;
    }

    /**
     * @brief Returns the inverted version of this matrix
     * @return Inverse matrix
     */
    inline FMatrix4 GetInverse() const noexcept
    {
        FMatrix4 Inverse;

    #if !USE_VECTOR_MATH
        // Calculate the inverse of a 4x4 matrix manually
        float ScalarA = (M[2][2] * M[3][3]) - (M[2][3] * M[3][2]);
        float ScalarB = (M[2][1] * M[3][3]) - (M[2][3] * M[3][1]);
        float ScalarC = (M[2][1] * M[3][2]) - (M[2][2] * M[3][1]);
        float ScalarD = (M[2][0] * M[3][3]) - (M[2][3] * M[3][0]);
        float ScalarE = (M[2][0] * M[3][2]) - (M[2][2] * M[3][0]);
        float ScalarF = (M[2][0] * M[3][1]) - (M[2][1] * M[3][0]);

        // Compute the adjugate matrix
        Inverse.M[0][0] =   (M[1][1] * ScalarA) - (M[1][2] * ScalarB) + (M[1][3] * ScalarC);
        Inverse.M[0][1] = -((M[1][0] * ScalarA) - (M[1][2] * ScalarD) + (M[1][3] * ScalarE));
        Inverse.M[0][2] =   (M[1][0] * ScalarB) - (M[1][1] * ScalarD) + (M[1][3] * ScalarF);
        Inverse.M[0][3] = -((M[1][0] * ScalarC) - (M[1][1] * ScalarE) + (M[1][2] * ScalarF));

        Inverse.M[1][0] = -((M[0][1] * ScalarA) - (M[0][2] * ScalarB) + (M[0][3] * ScalarC));
        Inverse.M[1][1] =   (M[0][0] * ScalarA) - (M[0][2] * ScalarD) + (M[0][3] * ScalarE);
        Inverse.M[1][2] = -((M[0][0] * ScalarB) - (M[0][1] * ScalarD) + (M[0][3] * ScalarF));
        Inverse.M[1][3] =   (M[0][0] * ScalarC) - (M[0][1] * ScalarE) + (M[0][2] * ScalarF);

        float ScalarG = (M[1][2] * M[3][3]) - (M[1][3] * M[3][2]);
        float ScalarH = (M[1][1] * M[3][3]) - (M[1][3] * M[3][1]);
        float ScalarI = (M[1][1] * M[3][2]) - (M[1][2] * M[3][1]);
        float ScalarJ = (M[1][0] * M[3][3]) - (M[1][3] * M[3][0]);
        float ScalarK = (M[1][0] * M[3][2]) - (M[1][2] * M[3][0]);
        float ScalarL = (M[1][0] * M[3][1]) - (M[1][1] * M[3][0]);

        Inverse.M[2][0] =   (M[0][1] * ScalarA) - (M[0][2] * ScalarB) + (M[0][3] * ScalarC);
        Inverse.M[2][1] = -((M[0][0] * ScalarA) - (M[0][2] * ScalarD) + (M[0][3] * ScalarE));
        Inverse.M[2][2] =   (M[0][0] * ScalarB) - (M[0][1] * ScalarD) + (M[0][3] * ScalarF);
        Inverse.M[2][3] = -((M[0][0] * ScalarC) - (M[0][1] * ScalarE) + (M[0][2] * ScalarF));

        float Determinant    = (M[0][0] * Inverse.M[0][0]) + (M[0][1] * Inverse.M[1][0]) + (M[0][2] * Inverse.M[2][0]) + (M[0][3] * Inverse.M[3][0]);
        float RcpDeterminant = 1.0f / Determinant;

        for (int32 Row = 0; Row < 4; ++Row)
        {
            for (int32 Col = 0; Col < 4; ++Col)
            {
                Inverse.M[Row][Col] *= RcpDeterminant;
            }
        }
    #else
        FVectorMath::MatrixInvert4x4(M[0], Inverse.M[0]);
    #endif

        return Inverse;
    }

    /**
     * @brief Returns the adjugate of this matrix
     * @return Adjugate matrix
     */
    inline FMatrix4 GetAdjugate() const noexcept
    {
        FMatrix4 Adjugate;

    #if !USE_VECTOR_MATH
        // Calculate the adjugate matrix manually
        float ScalarA = (M[2][2] * M[3][3]) - (M[2][3] * M[3][2]);
        float ScalarB = (M[2][1] * M[3][3]) - (M[2][3] * M[3][1]);
        float ScalarC = (M[2][1] * M[3][2]) - (M[2][2] * M[3][1]);
        float ScalarD = (M[2][0] * M[3][3]) - (M[2][3] * M[3][0]);
        float ScalarE = (M[2][0] * M[3][2]) - (M[2][2] * M[3][0]);
        float ScalarF = (M[2][0] * M[3][1]) - (M[2][1] * M[3][0]);

        Adjugate.M[0][0] =   (M[1][1] * ScalarA) - (M[1][2] * ScalarB) + (M[1][3] * ScalarC);
        Adjugate.M[0][1] = -((M[1][0] * ScalarA) - (M[1][2] * ScalarD) + (M[1][3] * ScalarE));
        Adjugate.M[0][2] =   (M[1][0] * ScalarB) - (M[1][1] * ScalarD) + (M[1][3] * ScalarF);
        Adjugate.M[0][3] = -((M[1][0] * ScalarC) - (M[1][1] * ScalarE) + (M[1][2] * ScalarF));

        float ScalarG = (M[1][2] * M[3][3]) - (M[1][3] * M[3][2]);
        float ScalarH = (M[1][1] * M[3][3]) - (M[1][3] * M[3][1]);
        float ScalarI = (M[1][1] * M[3][2]) - (M[1][2] * M[3][1]);
        float ScalarJ = (M[1][0] * M[3][3]) - (M[1][3] * M[3][0]);
        float ScalarK = (M[1][0] * M[3][2]) - (M[1][2] * M[3][0]);
        float ScalarL = (M[1][0] * M[3][1]) - (M[1][1] * M[3][0]);

        Adjugate.M[2][0] =   (M[0][1] * ScalarA) - (M[0][2] * ScalarB) + (M[0][3] * ScalarC);
        Adjugate.M[2][1] = -((M[0][0] * ScalarA) - (M[0][2] * ScalarD) + (M[0][3] * ScalarE));
        Adjugate.M[2][2] =   (M[0][0] * ScalarB) - (M[0][1] * ScalarD) + (M[0][3] * ScalarF);
        Adjugate.M[2][3] = -((M[0][0] * ScalarC) - (M[0][1] * ScalarE) + (M[0][2] * ScalarF));

        float ScalarM = (M[1][2] * M[2][3]) - (M[1][3] * M[2][2]);
        float ScalarN = (M[1][1] * M[2][3]) - (M[1][3] * M[2][1]);
        float ScalarO = (M[1][1] * M[2][2]) - (M[1][2] * M[2][1]);
        float ScalarP = (M[1][0] * M[2][3]) - (M[1][3] * M[2][0]);
        float ScalarQ = (M[1][0] * M[2][2]) - (M[1][2] * M[2][0]);
        float ScalarR = (M[1][0] * M[2][1]) - (M[1][1] * M[2][0]);

        Adjugate.M[3][0] = -((M[0][1] * ScalarM) - (M[0][2] * ScalarN) + (M[0][3] * ScalarO));
        Adjugate.M[3][1] =   (M[0][0] * ScalarM) - (M[0][2] * ScalarP) + (M[0][3] * ScalarQ);
        Adjugate.M[3][2] = -((M[0][0] * ScalarN) - (M[0][1] * ScalarP) + (M[0][3] * ScalarR));
        Adjugate.M[3][3] =   (M[0][0] * ScalarO) - (M[0][1] * ScalarQ) + (M[0][2] * ScalarR);
    #else
        FVectorMath::MatrixAdjoint4x4(M[0], Adjugate.M[0]);
    #endif

        return Adjugate;
    }

    /**
     * @brief Returns the determinant of this matrix
     * @return The determinant
     */
    inline float GetDeterminant() const noexcept
    {
        float Determinant = 0.0f;

    #if !USE_VECTOR_MATH
        // Calculate the determinant manually
        float ScalarA = (M[2][2] * M[3][3]) - (M[2][3] * M[3][2]);
        float ScalarB = (M[2][1] * M[3][3]) - (M[2][3] * M[3][1]);
        float ScalarC = (M[2][1] * M[3][2]) - (M[2][2] * M[3][1]);
        float ScalarD = (M[2][0] * M[3][3]) - (M[2][3] * M[3][0]);
        float ScalarE = (M[2][0] * M[3][2]) - (M[2][2] * M[3][0]);
        float ScalarF = (M[2][0] * M[3][1]) - (M[2][1] * M[3][0]);

        Determinant  = M[0][0] * ((M[1][1] * ScalarA) - (M[1][2] * ScalarB) + (M[1][3] * ScalarC));
        Determinant -= M[0][1] * ((M[1][0] * ScalarA) - (M[1][2] * ScalarD) + (M[1][3] * ScalarE));
        Determinant += M[0][2] * ((M[1][0] * ScalarB) - (M[1][1] * ScalarD) + (M[1][3] * ScalarF));
        Determinant -= M[0][3] * ((M[1][0] * ScalarC) - (M[1][1] * ScalarE) + (M[1][2] * ScalarF));
    #else
        Determinant = FVectorMath::MatrixDeterminant4x4(M[0]);
    #endif

        return Determinant;
    }

    /**
     * @brief Checks whether this matrix has any value that equals NaN
     * @return True if any value equals NaN, false otherwise
     */
    FORCEINLINE bool ContainsNaN() const noexcept
    {
        for (int32 Row = 0; Row < 4; ++Row)
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
        for (int32 Row = 0; Row < 4; ++Row)
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
    FORCEINLINE bool IsEqual(const FMatrix4& Other, float Epsilon = FMath::kIsEqualEpsilon) const noexcept
    {
    #if !USE_VECTOR_MATH
        Epsilon = FMath::Abs(Epsilon);

        for (int32 Row = 0; Row < 4; ++Row)
        {
            for (int32 Col = 0; Col < 4; ++Col)
            {
                const float Diff = M[Row][Col] - Other.M[Row][Col];
                if (FMath::Abs(Diff) > Epsilon)
                {
                    return false;
                }
            }
        }

        return true;
    #else
        FFloat128 Epsilon_128 = FVectorMath::VectorSet1(FMath::Abs(Epsilon));

        for (int32 Row = 0; Row < 4; ++Row)
        {
            FFloat128 Diff       = FVectorMath::VectorSub(M[Row], Other.M[Row]);
            FFloat128 Result_128 = FVectorMath::VectorAbs(Diff);

            if (FVectorMath::VectorAllGreaterThan(Result_128, Epsilon_128))
            {
                return false;
            }
        }

        return true;
    #endif
    }

    /**
     * @brief Sets this matrix to an identity matrix
     */
    inline void SetIdentity() noexcept
    {
    #if !USE_VECTOR_MATH
        M[0][0] = 1.0f;
        M[0][1] = 0.0f;
        M[0][2] = 0.0f;
        M[0][3] = 0.0f;

        M[1][0] = 0.0f;
        M[1][1] = 1.0f;
        M[1][2] = 0.0f;
        M[1][3] = 0.0f;

        M[2][0] = 0.0f;
        M[2][1] = 0.0f;
        M[2][2] = 1.0f;
        M[2][3] = 0.0f;

        M[3][0] = 0.0f;
        M[3][1] = 0.0f;
        M[3][2] = 0.0f;
        M[3][3] = 1.0f;
    #else
        FVectorMath::VectorStore(FVectorMath::VectorSet(1.0f, 0.0f, 0.0f, 0.0f), M[0]);
        FVectorMath::VectorStore(FVectorMath::VectorSet(0.0f, 1.0f, 0.0f, 0.0f), M[1]);
        FVectorMath::VectorStore(FVectorMath::VectorSet(0.0f, 0.0f, 1.0f, 0.0f), M[2]);
        FVectorMath::VectorStore(FVectorMath::VectorSet(0.0f, 0.0f, 0.0f, 1.0f), M[3]);
    #endif
    }

    /**
     * @brief Sets the upper 3x3 matrix (rotation and scale)
     * @param RotationAndScale 3x3 matrix to set the upper quadrant to
     */
    FORCEINLINE void SetRotationAndScale(const FMatrix3& RotationAndScale) noexcept
    {
        M[0][0] = RotationAndScale.M[0][0];
        M[0][1] = RotationAndScale.M[0][1];
        M[0][2] = RotationAndScale.M[0][2];

        M[1][0] = RotationAndScale.M[1][0];
        M[1][1] = RotationAndScale.M[1][1];
        M[1][2] = RotationAndScale.M[1][2];

        M[2][0] = RotationAndScale.M[2][0];
        M[2][1] = RotationAndScale.M[2][1];
        M[2][2] = RotationAndScale.M[2][2];
    }

    /**
     * @brief Sets the translation part of the matrix
     * @param Translation The translation vector
     */
    FORCEINLINE void SetTranslation(const FVector3& Translation) noexcept
    {
        M[3][0] = Translation.X;
        M[3][1] = Translation.Y;
        M[3][2] = Translation.Z;
    }

    /**
     * @brief Returns a row of this matrix
     * @param Row The row to retrieve (0, 1, 2, or 3)
     * @return A vector containing the specified row
     */
    FORCEINLINE FVector4 GetRow(int32 Row) const noexcept
    {
        CHECK(Row < 4);
        return FVector4(M[Row][0], M[Row][1], M[Row][2], M[Row][3]);
    }

    /**
     * @brief Returns a column of this matrix
     * @param Column The column to retrieve (0, 1, 2, or 3)
     * @return A vector containing the specified column
     */
    FORCEINLINE FVector4 GetColumn(int32 Column) const noexcept
    {
        CHECK(Column < 4);
        return FVector4(M[0][Column], M[1][Column], M[2][Column], M[3][Column]);
    }

    /**
     * @brief Returns the translation part of this matrix
     * @return A vector containing the translation
     */
    FORCEINLINE FVector3 GetTranslation() const noexcept
    {
        return FVector3(M[3][0], M[3][1], M[3][2]);
    }

    /**
     * @brief Returns the upper 3x3 rotation and scale matrix
     * @return A 3x3 matrix containing the upper part of the matrix
     */
    FORCEINLINE FMatrix3 GetRotationAndScale() const noexcept
    {
        return FMatrix3(
            M[0][0], M[0][1], M[0][2],
            M[1][0], M[1][1], M[1][2],
            M[2][0], M[2][1], M[2][2]);
    }

public:

    /**
     * @brief Transforms a 4-D vector using this matrix
     * @param Other The vector to transform
     * @return The transformed vector
     */
    FORCEINLINE FVector4 operator*(const FVector4& Other) const noexcept
    {
        return Transform(Other);
    }

    /**
     * @brief Multiplies this matrix with another matrix
     * @param Other The other matrix
     * @return A matrix containing the result of the multiplication
     */
    inline FMatrix4 operator*(const FMatrix4& Other) const noexcept
    {
        FMatrix4 Result;

    #if !USE_VECTOR_MATH
        Result.M[0][0] = (M[0][0] * Other.M[0][0]) + (M[0][1] * Other.M[1][0]) + (M[0][2] * Other.M[2][0]) + (M[0][3] * Other.M[3][0]);
        Result.M[0][1] = (M[0][0] * Other.M[0][1]) + (M[0][1] * Other.M[1][1]) + (M[0][2] * Other.M[2][1]) + (M[0][3] * Other.M[3][1]);
        Result.M[0][2] = (M[0][0] * Other.M[0][2]) + (M[0][1] * Other.M[1][2]) + (M[0][2] * Other.M[2][2]) + (M[0][3] * Other.M[3][2]);
        Result.M[0][3] = (M[0][0] * Other.M[0][3]) + (M[0][1] * Other.M[1][3]) + (M[0][2] * Other.M[2][3]) + (M[0][3] * Other.M[3][3]);

        Result.M[1][0] = (M[1][0] * Other.M[0][0]) + (M[1][1] * Other.M[1][0]) + (M[1][2] * Other.M[2][0]) + (M[1][3] * Other.M[3][0]);
        Result.M[1][1] = (M[1][0] * Other.M[0][1]) + (M[1][1] * Other.M[1][1]) + (M[1][2] * Other.M[2][1]) + (M[1][3] * Other.M[3][1]);
        Result.M[1][2] = (M[1][0] * Other.M[0][2]) + (M[1][1] * Other.M[1][2]) + (M[1][2] * Other.M[2][2]) + (M[1][3] * Other.M[3][2]);
        Result.M[1][3] = (M[1][0] * Other.M[0][3]) + (M[1][1] * Other.M[1][3]) + (M[1][2] * Other.M[2][3]) + (M[1][3] * Other.M[3][3]);

        Result.M[2][0] = (M[2][0] * Other.M[0][0]) + (M[2][1] * Other.M[1][0]) + (M[2][2] * Other.M[2][0]) + (M[2][3] * Other.M[3][0]);
        Result.M[2][1] = (M[2][0] * Other.M[0][1]) + (M[2][1] * Other.M[1][1]) + (M[2][2] * Other.M[2][1]) + (M[2][3] * Other.M[3][1]);
        Result.M[2][2] = (M[2][0] * Other.M[0][2]) + (M[2][1] * Other.M[1][2]) + (M[2][2] * Other.M[2][2]) + (M[2][3] * Other.M[3][2]);
        Result.M[2][3] = (M[2][0] * Other.M[0][3]) + (M[2][1] * Other.M[1][3]) + (M[2][2] * Other.M[2][3]) + (M[2][3] * Other.M[3][3]);

        Result.M[3][0] = (M[3][0] * Other.M[0][0]) + (M[3][1] * Other.M[1][0]) + (M[3][2] * Other.M[2][0]) + (M[3][3] * Other.M[3][0]);
        Result.M[3][1] = (M[3][0] * Other.M[0][1]) + (M[3][1] * Other.M[1][1]) + (M[3][2] * Other.M[2][1]) + (M[3][3] * Other.M[3][1]);
        Result.M[3][2] = (M[3][0] * Other.M[0][2]) + (M[3][1] * Other.M[1][2]) + (M[3][2] * Other.M[2][2]) + (M[3][3] * Other.M[3][2]);
        Result.M[3][3] = (M[3][0] * Other.M[0][3]) + (M[3][1] * Other.M[1][3]) + (M[3][2] * Other.M[2][3]) + (M[3][3] * Other.M[3][3]);
    #else
        FVectorMath::MatrixMul4x4(M[0], Other.M[0], Result.M[0]);
    #endif

        return Result;
    }

    /**
     * @brief Multiplies this matrix with another matrix and assigns the result
     * @param Other The other matrix
     * @return A reference to this matrix after multiplication
     */
    inline FMatrix4& operator*=(const FMatrix4& Other) noexcept
    {
    #if !USE_VECTOR_MATH
        M[0][0] = (M[0][0] * Other.M[0][0]) + (M[0][1] * Other.M[1][0]) + (M[0][2] * Other.M[2][0]) + (M[0][3] * Other.M[3][0]);
        M[0][1] = (M[0][0] * Other.M[0][1]) + (M[0][1] * Other.M[1][1]) + (M[0][2] * Other.M[2][1]) + (M[0][3] * Other.M[3][1]);
        M[0][2] = (M[0][0] * Other.M[0][2]) + (M[0][1] * Other.M[1][2]) + (M[0][2] * Other.M[2][2]) + (M[0][3] * Other.M[3][2]);
        M[0][3] = (M[0][0] * Other.M[0][3]) + (M[0][1] * Other.M[1][3]) + (M[0][2] * Other.M[2][3]) + (M[0][3] * Other.M[3][3]);

        M[1][0] = (M[1][0] * Other.M[0][0]) + (M[1][1] * Other.M[1][0]) + (M[1][2] * Other.M[2][0]) + (M[1][3] * Other.M[3][0]);
        M[1][1] = (M[1][0] * Other.M[0][1]) + (M[1][1] * Other.M[1][1]) + (M[1][2] * Other.M[2][1]) + (M[1][3] * Other.M[3][1]);
        M[1][2] = (M[1][0] * Other.M[0][2]) + (M[1][1] * Other.M[1][2]) + (M[1][2] * Other.M[2][2]) + (M[1][3] * Other.M[3][2]);
        M[1][3] = (M[1][0] * Other.M[0][3]) + (M[1][1] * Other.M[1][3]) + (M[1][2] * Other.M[2][3]) + (M[1][3] * Other.M[3][3]);

        M[2][0] = (M[2][0] * Other.M[0][0]) + (M[2][1] * Other.M[1][0]) + (M[2][2] * Other.M[2][0]) + (M[2][3] * Other.M[3][0]);
        M[2][1] = (M[2][0] * Other.M[0][1]) + (M[2][1] * Other.M[1][1]) + (M[2][2] * Other.M[2][1]) + (M[2][3] * Other.M[3][1]);
        M[2][2] = (M[2][0] * Other.M[0][2]) + (M[2][1] * Other.M[1][2]) + (M[2][2] * Other.M[2][2]) + (M[2][3] * Other.M[3][2]);
        M[2][3] = (M[2][0] * Other.M[0][3]) + (M[2][1] * Other.M[1][3]) + (M[2][2] * Other.M[2][3]) + (M[2][3] * Other.M[3][3]);

        M[3][0] = (M[3][0] * Other.M[0][0]) + (M[3][1] * Other.M[1][0]) + (M[3][2] * Other.M[2][0]) + (M[3][3] * Other.M[3][0]);
        M[3][1] = (M[3][0] * Other.M[0][1]) + (M[3][1] * Other.M[1][1]) + (M[3][2] * Other.M[2][1]) + (M[3][3] * Other.M[3][1]);
        M[3][2] = (M[3][0] * Other.M[0][2]) + (M[3][1] * Other.M[1][2]) + (M[3][2] * Other.M[2][2]) + (M[3][3] * Other.M[3][2]);
        M[3][3] = (M[3][0] * Other.M[0][3]) + (M[3][1] * Other.M[1][3]) + (M[3][2] * Other.M[2][3]) + (M[3][3] * Other.M[3][3]);
    #else
        FVectorMath::MatrixMul4x4(M[0], Other.M[0], M[0]);
    #endif

        return *this;
    }

    /**
     * @brief Multiplies this matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A matrix containing the result of the multiplication
     */
    inline FMatrix4 operator*(float Scalar) const noexcept
    {
        FMatrix4 Result;

    #if !USE_VECTOR_MATH
        Result.M[0][0] = M[0][0] * Scalar;
        Result.M[0][1] = M[0][1] * Scalar;
        Result.M[0][2] = M[0][2] * Scalar;
        Result.M[0][3] = M[0][3] * Scalar;

        Result.M[1][0] = M[1][0] * Scalar;
        Result.M[1][1] = M[1][1] * Scalar;
        Result.M[1][2] = M[1][2] * Scalar;
        Result.M[1][3] = M[1][3] * Scalar;

        Result.M[2][0] = M[2][0] * Scalar;
        Result.M[2][1] = M[2][1] * Scalar;
        Result.M[2][2] = M[2][2] * Scalar;
        Result.M[2][3] = M[2][3] * Scalar;

        Result.M[3][0] = M[3][0] * Scalar;
        Result.M[3][1] = M[3][1] * Scalar;
        Result.M[3][2] = M[3][2] * Scalar;
        Result.M[3][3] = M[3][3] * Scalar;
    #else
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);

        FFloat128 MatrixRow0 = FVectorMath::VectorMul(M[0], Scalars_128);
        FFloat128 MatrixRow1 = FVectorMath::VectorMul(M[1], Scalars_128);
        FFloat128 MatrixRow2 = FVectorMath::VectorMul(M[2], Scalars_128);
        FFloat128 MatrixRow3 = FVectorMath::VectorMul(M[3], Scalars_128);

        FVectorMath::VectorStore(MatrixRow0, Result.M[0]);
        FVectorMath::VectorStore(MatrixRow1, Result.M[1]);
        FVectorMath::VectorStore(MatrixRow2, Result.M[2]);
        FVectorMath::VectorStore(MatrixRow3, Result.M[3]);
    #endif

        return Result;
    }

    /**
     * @brief Multiplies this matrix component-wise with a scalar and assigns the result
     * @param Scalar The scalar
     * @return A reference to this matrix after multiplication
     */
    inline FMatrix4& operator*=(float Scalar) noexcept
    {
    #if !USE_VECTOR_MATH
        M[0][0] = M[0][0] * Scalar;
        M[0][1] = M[0][1] * Scalar;
        M[0][2] = M[0][2] * Scalar;
        M[0][3] = M[0][3] * Scalar;

        M[1][0] = M[1][0] * Scalar;
        M[1][1] = M[1][1] * Scalar;
        M[1][2] = M[1][2] * Scalar;
        M[1][3] = M[1][3] * Scalar;

        M[2][0] = M[2][0] * Scalar;
        M[2][1] = M[2][1] * Scalar;
        M[2][2] = M[2][2] * Scalar;
        M[2][3] = M[2][3] * Scalar;

        M[3][0] = M[3][0] * Scalar;
        M[3][1] = M[3][1] * Scalar;
        M[3][2] = M[3][2] * Scalar;
        M[3][3] = M[3][3] * Scalar;
    #else
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);

        FFloat128 MatrixRow0 = FVectorMath::VectorMul(M[0], Scalars_128);
        FFloat128 MatrixRow1 = FVectorMath::VectorMul(M[1], Scalars_128);
        FFloat128 MatrixRow2 = FVectorMath::VectorMul(M[2], Scalars_128);
        FFloat128 MatrixRow3 = FVectorMath::VectorMul(M[3], Scalars_128);

        FVectorMath::VectorStore(MatrixRow0, M[0]);
        FVectorMath::VectorStore(MatrixRow1, M[1]);
        FVectorMath::VectorStore(MatrixRow2, M[2]);
        FVectorMath::VectorStore(MatrixRow3, M[3]);
    #endif

        return *this;
    }

    /**
     * @brief Adds this matrix component-wise with another matrix
     * @param Other The other matrix
     * @return A matrix containing the result of the addition
     */
    inline FMatrix4 operator+(const FMatrix4& Other) const noexcept
    {
        FMatrix4 Result;

    #if !USE_VECTOR_MATH
        Result.M[0][0] = M[0][0] + Other.M[0][0];
        Result.M[0][1] = M[0][1] + Other.M[0][1];
        Result.M[0][2] = M[0][2] + Other.M[0][2];
        Result.M[0][3] = M[0][3] + Other.M[0][3];

        Result.M[1][0] = M[1][0] + Other.M[1][0];
        Result.M[1][1] = M[1][1] + Other.M[1][1];
        Result.M[1][2] = M[1][2] + Other.M[1][2];
        Result.M[1][3] = M[1][3] + Other.M[1][3];

        Result.M[2][0] = M[2][0] + Other.M[2][0];
        Result.M[2][1] = M[2][1] + Other.M[2][1];
        Result.M[2][2] = M[2][2] + Other.M[2][2];
        Result.M[2][3] = M[2][3] + Other.M[2][3];

        Result.M[3][0] = M[3][0] + Other.M[3][0];
        Result.M[3][1] = M[3][1] + Other.M[3][1];
        Result.M[3][2] = M[3][2] + Other.M[3][2];
        Result.M[3][3] = M[3][3] + Other.M[3][3];
    #else
        FFloat128 MatrixRow0 = FVectorMath::VectorAdd(M[0], Other.M[0]);
        FFloat128 MatrixRow1 = FVectorMath::VectorAdd(M[1], Other.M[1]);
        FFloat128 MatrixRow2 = FVectorMath::VectorAdd(M[2], Other.M[2]);
        FFloat128 MatrixRow3 = FVectorMath::VectorAdd(M[3], Other.M[3]);

        FVectorMath::VectorStore(MatrixRow0, Result.M[0]);
        FVectorMath::VectorStore(MatrixRow1, Result.M[1]);
        FVectorMath::VectorStore(MatrixRow2, Result.M[2]);
        FVectorMath::VectorStore(MatrixRow3, Result.M[3]);
    #endif

        return Result;
    }

    /**
     * @brief Adds this matrix component-wise with another matrix and assigns the result
     * @param Other The other matrix
     * @return A reference to this matrix after addition
     */
    inline FMatrix4& operator+=(const FMatrix4& Other) noexcept
    {
    #if !USE_VECTOR_MATH
        M[0][0] = M[0][0] + Other.M[0][0];
        M[0][1] = M[0][1] + Other.M[0][1];
        M[0][2] = M[0][2] + Other.M[0][2];
        M[0][3] = M[0][3] + Other.M[0][3];

        M[1][0] = M[1][0] + Other.M[1][0];
        M[1][1] = M[1][1] + Other.M[1][1];
        M[1][2] = M[1][2] + Other.M[1][2];
        M[1][3] = M[1][3] + Other.M[1][3];

        M[2][0] = M[2][0] + Other.M[2][0];
        M[2][1] = M[2][1] + Other.M[2][1];
        M[2][2] = M[2][2] + Other.M[2][2];
        M[2][3] = M[2][3] + Other.M[2][3];

        M[3][0] = M[3][0] + Other.M[3][0];
        M[3][1] = M[3][1] + Other.M[3][1];
        M[3][2] = M[3][2] + Other.M[3][2];
        M[3][3] = M[3][3] + Other.M[3][3];
    #else
        FFloat128 MatrixRow0 = FVectorMath::VectorAdd(M[0], Other.M[0]);
        FFloat128 MatrixRow1 = FVectorMath::VectorAdd(M[1], Other.M[1]);
        FFloat128 MatrixRow2 = FVectorMath::VectorAdd(M[2], Other.M[2]);
        FFloat128 MatrixRow3 = FVectorMath::VectorAdd(M[3], Other.M[3]);

        FVectorMath::VectorStore(MatrixRow0, M[0]);
        FVectorMath::VectorStore(MatrixRow1, M[1]);
        FVectorMath::VectorStore(MatrixRow2, M[2]);
        FVectorMath::VectorStore(MatrixRow3, M[3]);
    #endif

        return *this;
    }

    /**
     * @brief Adds this matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A matrix containing the result of the addition
     */
    inline FMatrix4 operator+(float Scalar) const noexcept
    {
        FMatrix4 Result;

    #if !USE_VECTOR_MATH
        Result.M[0][0] = M[0][0] + Scalar;
        Result.M[0][1] = M[0][1] + Scalar;
        Result.M[0][2] = M[0][2] + Scalar;
        Result.M[0][3] = M[0][3] + Scalar;

        Result.M[1][0] = M[1][0] + Scalar;
        Result.M[1][1] = M[1][1] + Scalar;
        Result.M[1][2] = M[1][2] + Scalar;
        Result.M[1][3] = M[1][3] + Scalar;

        Result.M[2][0] = M[2][0] + Scalar;
        Result.M[2][1] = M[2][1] + Scalar;
        Result.M[2][2] = M[2][2] + Scalar;
        Result.M[2][3] = M[2][3] + Scalar;

        Result.M[3][0] = M[3][0] + Scalar;
        Result.M[3][1] = M[3][1] + Scalar;
        Result.M[3][2] = M[3][2] + Scalar;
        Result.M[3][3] = M[3][3] + Scalar;
    #else
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);

        FFloat128 MatrixRow0 = FVectorMath::VectorAdd(M[0], Scalars_128);
        FFloat128 MatrixRow1 = FVectorMath::VectorAdd(M[1], Scalars_128);
        FFloat128 MatrixRow2 = FVectorMath::VectorAdd(M[2], Scalars_128);
        FFloat128 MatrixRow3 = FVectorMath::VectorAdd(M[3], Scalars_128);

        FVectorMath::VectorStore(MatrixRow0, Result.M[0]);
        FVectorMath::VectorStore(MatrixRow1, Result.M[1]);
        FVectorMath::VectorStore(MatrixRow2, Result.M[2]);
        FVectorMath::VectorStore(MatrixRow3, Result.M[3]);
    #endif

        return Result;
    }

    /**
     * @brief Adds this matrix component-wise with a scalar and assigns the result
     * @param Scalar The scalar
     * @return A reference to this matrix after addition
     */
    inline FMatrix4& operator+=(float Scalar) noexcept
    {
    #if !USE_VECTOR_MATH
        M[0][0] = M[0][0] + Scalar;
        M[0][1] = M[0][1] + Scalar;
        M[0][2] = M[0][2] + Scalar;
        M[0][3] = M[0][3] + Scalar;

        M[1][0] = M[1][0] + Scalar;
        M[1][1] = M[1][1] + Scalar;
        M[1][2] = M[1][2] + Scalar;
        M[1][3] = M[1][3] + Scalar;

        M[2][0] = M[2][0] + Scalar;
        M[2][1] = M[2][1] + Scalar;
        M[2][2] = M[2][2] + Scalar;
        M[2][3] = M[2][3] + Scalar;

        M[3][0] = M[3][0] + Scalar;
        M[3][1] = M[3][1] + Scalar;
        M[3][2] = M[3][2] + Scalar;
        M[3][3] = M[3][3] + Scalar;
    #else
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);

        FFloat128 MatrixRow0 = FVectorMath::VectorAdd(M[0], Scalars_128);
        FFloat128 MatrixRow1 = FVectorMath::VectorAdd(M[1], Scalars_128);
        FFloat128 MatrixRow2 = FVectorMath::VectorAdd(M[2], Scalars_128);
        FFloat128 MatrixRow3 = FVectorMath::VectorAdd(M[3], Scalars_128);

        FVectorMath::VectorStore(MatrixRow0, M[0]);
        FVectorMath::VectorStore(MatrixRow1, M[1]);
        FVectorMath::VectorStore(MatrixRow2, M[2]);
        FVectorMath::VectorStore(MatrixRow3, M[3]);
    #endif

        return *this;
    }

    /**
     * @brief Subtracts this matrix component-wise with another matrix
     * @param Other The other matrix
     * @return A matrix containing the result of the subtraction
     */
    inline FMatrix4 operator-(const FMatrix4& Other) const noexcept
    {
        FMatrix4 Result;

    #if !USE_VECTOR_MATH
        Result.M[0][0] = M[0][0] - Other.M[0][0];
        Result.M[0][1] = M[0][1] - Other.M[0][1];
        Result.M[0][2] = M[0][2] - Other.M[0][2];
        Result.M[0][3] = M[0][3] - Other.M[0][3];

        Result.M[1][0] = M[1][0] - Other.M[1][0];
        Result.M[1][1] = M[1][1] - Other.M[1][1];
        Result.M[1][2] = M[1][2] - Other.M[1][2];
        Result.M[1][3] = M[1][3] - Other.M[1][3];

        Result.M[2][0] = M[2][0] - Other.M[2][0];
        Result.M[2][1] = M[2][1] - Other.M[2][1];
        Result.M[2][2] = M[2][2] - Other.M[2][2];
        Result.M[2][3] = M[2][3] - Other.M[2][3];

        Result.M[3][0] = M[3][0] - Other.M[3][0];
        Result.M[3][1] = M[3][1] - Other.M[3][1];
        Result.M[3][2] = M[3][2] - Other.M[3][2];
        Result.M[3][3] = M[3][3] - Other.M[3][3];
    #else
        FFloat128 MatrixRow0 = FVectorMath::VectorSub(M[0], Other.M[0]);
        FFloat128 MatrixRow1 = FVectorMath::VectorSub(M[1], Other.M[1]);
        FFloat128 MatrixRow2 = FVectorMath::VectorSub(M[2], Other.M[2]);
        FFloat128 MatrixRow3 = FVectorMath::VectorSub(M[3], Other.M[3]);

        FVectorMath::VectorStore(MatrixRow0, Result.M[0]);
        FVectorMath::VectorStore(MatrixRow1, Result.M[1]);
        FVectorMath::VectorStore(MatrixRow2, Result.M[2]);
        FVectorMath::VectorStore(MatrixRow3, Result.M[3]);
    #endif

        return Result;
    }

    /**
     * @brief Subtracts this matrix component-wise with another matrix and assigns the result
     * @param Other The other matrix
     * @return A reference to this matrix after subtraction
     */
    inline FMatrix4& operator-=(const FMatrix4& Other) noexcept
    {
    #if !USE_VECTOR_MATH
        M[0][0] = M[0][0] - Other.M[0][0];
        M[0][1] = M[0][1] - Other.M[0][1];
        M[0][2] = M[0][2] - Other.M[0][2];
        M[0][3] = M[0][3] - Other.M[0][3];

        M[1][0] = M[1][0] - Other.M[1][0];
        M[1][1] = M[1][1] - Other.M[1][1];
        M[1][2] = M[1][2] - Other.M[1][2];
        M[1][3] = M[1][3] - Other.M[1][3];

        M[2][0] = M[2][0] - Other.M[2][0];
        M[2][1] = M[2][1] - Other.M[2][1];
        M[2][2] = M[2][2] - Other.M[2][2];
        M[2][3] = M[2][3] - Other.M[2][3];

        M[3][0] = M[3][0] - Other.M[3][0];
        M[3][1] = M[3][1] - Other.M[3][1];
        M[3][2] = M[3][2] - Other.M[3][2];
        M[3][3] = M[3][3] - Other.M[3][3];
    #else
        FFloat128 MatrixRow0 = FVectorMath::VectorSub(M[0], Other.M[0]);
        FFloat128 MatrixRow1 = FVectorMath::VectorSub(M[1], Other.M[1]);
        FFloat128 MatrixRow2 = FVectorMath::VectorSub(M[2], Other.M[2]);
        FFloat128 MatrixRow3 = FVectorMath::VectorSub(M[3], Other.M[3]);

        FVectorMath::VectorStore(MatrixRow0, M[0]);
        FVectorMath::VectorStore(MatrixRow1, M[1]);
        FVectorMath::VectorStore(MatrixRow2, M[2]);
        FVectorMath::VectorStore(MatrixRow3, M[3]);
    #endif

        return *this;
    }

    /**
     * @brief Subtracts this matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A matrix containing the result of the subtraction
     */
    inline FMatrix4 operator-(float Scalar) const noexcept
    {
        FMatrix4 Result;

    #if !USE_VECTOR_MATH
        Result.M[0][0] = M[0][0] - Scalar;
        Result.M[0][1] = M[0][1] - Scalar;
        Result.M[0][2] = M[0][2] - Scalar;
        Result.M[0][3] = M[0][3] - Scalar;

        Result.M[1][0] = M[1][0] - Scalar;
        Result.M[1][1] = M[1][1] - Scalar;
        Result.M[1][2] = M[1][2] - Scalar;
        Result.M[1][3] = M[1][3] - Scalar;

        Result.M[2][0] = M[2][0] - Scalar;
        Result.M[2][1] = M[2][1] - Scalar;
        Result.M[2][2] = M[2][2] - Scalar;
        Result.M[2][3] = M[2][3] - Scalar;

        Result.M[3][0] = M[3][0] - Scalar;
        Result.M[3][1] = M[3][1] - Scalar;
        Result.M[3][2] = M[3][2] - Scalar;
        Result.M[3][3] = M[3][3] - Scalar;
    #else
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);

        FFloat128 MatrixRow0 = FVectorMath::VectorSub(M[0], Scalars_128);
        FFloat128 MatrixRow1 = FVectorMath::VectorSub(M[1], Scalars_128);
        FFloat128 MatrixRow2 = FVectorMath::VectorSub(M[2], Scalars_128);
        FFloat128 MatrixRow3 = FVectorMath::VectorSub(M[3], Scalars_128);

        FVectorMath::VectorStore(MatrixRow0, Result.M[0]);
        FVectorMath::VectorStore(MatrixRow1, Result.M[1]);
        FVectorMath::VectorStore(MatrixRow2, Result.M[2]);
        FVectorMath::VectorStore(MatrixRow3, Result.M[3]);
    #endif

        return Result;
    }

    /**
     * @brief Subtracts this matrix component-wise with a scalar and assigns the result
     * @param RHS The scalar
     * @return A reference to this matrix after subtraction
     */
    inline FMatrix4& operator-=(float Scalar) noexcept
    {
    #if !USE_VECTOR_MATH
        M[0][0] = M[0][0] - Scalar;
        M[0][1] = M[0][1] - Scalar;
        M[0][2] = M[0][2] - Scalar;
        M[0][3] = M[0][3] - Scalar;

        M[1][0] = M[1][0] - Scalar;
        M[1][1] = M[1][1] - Scalar;
        M[1][2] = M[1][2] - Scalar;
        M[1][3] = M[1][3] - Scalar;

        M[2][0] = M[2][0] - Scalar;
        M[2][1] = M[2][1] - Scalar;
        M[2][2] = M[2][2] - Scalar;
        M[2][3] = M[2][3] - Scalar;

        M[3][0] = M[3][0] - Scalar;
        M[3][1] = M[3][1] - Scalar;
        M[3][2] = M[3][2] - Scalar;
        M[3][3] = M[3][3] - Scalar;
    #else
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);

        FFloat128 MatrixRow0 = FVectorMath::VectorAdd(M[0], Scalars_128);
        FFloat128 MatrixRow1 = FVectorMath::VectorAdd(M[1], Scalars_128);
        FFloat128 MatrixRow2 = FVectorMath::VectorAdd(M[2], Scalars_128);
        FFloat128 MatrixRow3 = FVectorMath::VectorAdd(M[3], Scalars_128);

        FVectorMath::VectorStore(MatrixRow0, M[0]);
        FVectorMath::VectorStore(MatrixRow1, M[1]);
        FVectorMath::VectorStore(MatrixRow2, M[2]);
        FVectorMath::VectorStore(MatrixRow3, M[3]);
    #endif

        return *this;
    }

    /**
     * @brief Divides this matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A matrix containing the result of the division
     */
    inline FMatrix4 operator/(float Scalar) const noexcept
    {
        FMatrix4 Result;

    #if !USE_VECTOR_MATH
        Result.M[0][0] = M[0][0] / Scalar;
        Result.M[0][1] = M[0][1] / Scalar;
        Result.M[0][2] = M[0][2] / Scalar;
        Result.M[0][3] = M[0][3] / Scalar;

        Result.M[1][0] = M[1][0] / Scalar;
        Result.M[1][1] = M[1][1] / Scalar;
        Result.M[1][2] = M[1][2] / Scalar;
        Result.M[1][3] = M[1][3] / Scalar;

        Result.M[2][0] = M[2][0] / Scalar;
        Result.M[2][1] = M[2][1] / Scalar;
        Result.M[2][2] = M[2][2] / Scalar;
        Result.M[2][3] = M[2][3] / Scalar;

        Result.M[3][0] = M[3][0] / Scalar;
        Result.M[3][1] = M[3][1] / Scalar;
        Result.M[3][2] = M[3][2] / Scalar;
        Result.M[3][3] = M[3][3] / Scalar;
    #else
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);

        FFloat128 MatrixRow0 = FVectorMath::VectorDiv(M[0], Scalars_128);
        FFloat128 MatrixRow1 = FVectorMath::VectorDiv(M[1], Scalars_128);
        FFloat128 MatrixRow2 = FVectorMath::VectorDiv(M[2], Scalars_128);
        FFloat128 MatrixRow3 = FVectorMath::VectorDiv(M[3], Scalars_128);

        FVectorMath::VectorStore(MatrixRow0, Result.M[0]);
        FVectorMath::VectorStore(MatrixRow1, Result.M[1]);
        FVectorMath::VectorStore(MatrixRow2, Result.M[2]);
        FVectorMath::VectorStore(MatrixRow3, Result.M[3]);
    #endif

        return Result;
    }

    /**
     * @brief Divides this matrix component-wise with a scalar and assigns the result
     * @param Scalar The scalar
     * @return A reference to this matrix after division
     */
    inline FMatrix4& operator/=(float Scalar) noexcept
    {
    #if !USE_VECTOR_MATH
        M[0][0] = M[0][0] / Scalar;
        M[0][1] = M[0][1] / Scalar;
        M[0][2] = M[0][2] / Scalar;
        M[0][3] = M[0][3] / Scalar;

        M[1][0] = M[1][0] / Scalar;
        M[1][1] = M[1][1] / Scalar;
        M[1][2] = M[1][2] / Scalar;
        M[1][3] = M[1][3] / Scalar;

        M[2][0] = M[2][0] / Scalar;
        M[2][1] = M[2][1] / Scalar;
        M[2][2] = M[2][2] / Scalar;
        M[2][3] = M[2][3] / Scalar;

        M[3][0] = M[3][0] / Scalar;
        M[3][1] = M[3][1] / Scalar;
        M[3][2] = M[3][2] / Scalar;
        M[3][3] = M[3][3] / Scalar;
    #else
        FFloat128 Scalars_128 = FVectorMath::VectorSet1(Scalar);

        FFloat128 MatrixRow0 = FVectorMath::VectorDiv(M[0], Scalars_128);
        FFloat128 MatrixRow1 = FVectorMath::VectorDiv(M[1], Scalars_128);
        FFloat128 MatrixRow2 = FVectorMath::VectorDiv(M[2], Scalars_128);
        FFloat128 MatrixRow3 = FVectorMath::VectorDiv(M[3], Scalars_128);

        FVectorMath::VectorStore(MatrixRow0, M[0]);
        FVectorMath::VectorStore(MatrixRow1, M[1]);
        FVectorMath::VectorStore(MatrixRow2, M[2]);
        FVectorMath::VectorStore(MatrixRow3, M[3]);
    #endif

        return *this;
    }

    /**
     * @brief Compares this matrix with another matrix for equality
     * @param Other The matrix to compare with
     * @return True if matrices are equal within a default epsilon, false otherwise
     */
    FORCEINLINE bool operator==(const FMatrix4& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief Compares this matrix with another matrix for inequality
     * @param Other The matrix to compare with
     * @return True if matrices are not equal within a default epsilon, false otherwise
     */
    FORCEINLINE bool operator!=(const FMatrix4& Other) const noexcept
    {
        return !IsEqual(Other);
    }

public:

    /**
     * @brief Creates and returns an identity matrix
     * @return An identity matrix
     */
    static FORCEINLINE FMatrix4 Identity() noexcept
    {
        FMatrix4 IdentityMatrix;
        IdentityMatrix.SetIdentity();
        return IdentityMatrix;
    }

    /**
     * @brief Creates and returns a uniform scale matrix
     * @param Scale Uniform scale that represents this matrix
     * @return A scale matrix
     */
    static FORCEINLINE FMatrix4 Scale(float Scale) noexcept
    {
        return FMatrix4(
            Scale, 0.0f,  0.0f,  0.0f,
            0.0f,  Scale, 0.0f,  0.0f,
            0.0f,  0.0f,  Scale, 0.0f,
            0.0f,  0.0f,  0.0f,  1.0f);
    }

    /**
     * @brief Creates and returns a scale matrix for each axis
     * @param InX Scale for the x-axis
     * @param InY Scale for the y-axis
     * @param InZ Scale for the z-axis
     * @return A scale matrix
     */
    static FORCEINLINE FMatrix4 Scale(float InX, float InY, float InZ) noexcept
    {
        return FMatrix4(
            InX,  0.0f, 0.0f, 0.0f,
            0.0f, InY,  0.0f, 0.0f,
            0.0f, 0.0f, InZ,  0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    }

    /**
     * @brief Creates and returns a scale matrix using a vector for scaling each axis
     * @param VectorWithScale A vector containing the scale for each axis in the x-, y-, z-components
     * @return A scale matrix
     */
    static FORCEINLINE FMatrix4 Scale(const FVector3& VectorWithScale) noexcept
    {
        return FMatrix4(
            VectorWithScale.X, 0.0f,              0.0f,              0.0f,
            0.0f,              VectorWithScale.Y, 0.0f,              0.0f,
            0.0f,              0.0f,              VectorWithScale.Z, 0.0f,
            0.0f,              0.0f,              0.0f,              1.0f);
    }

    /**
     * @brief Creates and returns a translation matrix
     * @param InX Translation for the x-axis
     * @param InY Translation for the y-axis
     * @param InZ Translation for the z-axis
     * @return A translation matrix
     */
    static FORCEINLINE FMatrix4 Translation(float InX, float InY, float InZ) noexcept
    {
        return FMatrix4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            InX,  InY,  InZ,  1.0f);
    }

    /**
     * @brief Creates and returns a translation matrix
     * @param InTranslation A vector containing the translation
     * @return A translation matrix
     */
    static FORCEINLINE FMatrix4 Translation(const FVector3& InTranslation) noexcept
    {
        return FMatrix4(
            1.0f,            0.0f,            0.0f,            0.0f,
            0.0f,            1.0f,            0.0f,            0.0f,
            0.0f,            0.0f,            1.0f,            0.0f,
            InTranslation.X, InTranslation.Y, InTranslation.Z, 1.0f);
    }

    /**
     * @brief Creates and returns a rotation matrix from Roll, Pitch, and Yaw in radians
     * @param Pitch Rotation around the x-axis in radians
     * @param Yaw Rotation around the y-axis in radians
     * @param Roll Rotation around the z-axis in radians
     * @return A rotation matrix
     */
    static FORCEINLINE FMatrix4 RotationRollPitchYaw(float Pitch, float Yaw, float Roll) noexcept
    {
        const float SinP = FMath::Sin(Pitch);
        const float CosP = FMath::Cos(Pitch);
        const float SinY = FMath::Sin(Yaw);
        const float CosY = FMath::Cos(Yaw);
        const float SinR = FMath::Sin(Roll);
        const float CosR = FMath::Cos(Roll);

        const float SinRSinP = SinR * SinP;
        const float CosRSinP = CosR * SinP;

        return FMatrix4(
            (CosR * CosY) + (SinRSinP * SinY),  SinR * CosP, (SinRSinP * CosY) - (CosR * SinY), 0.0f,
            (CosRSinP * SinY) - (SinR * CosY),  CosR * CosP, (SinR * SinY) + (CosRSinP * CosY), 0.0f,
            (CosP * SinY),                     -SinP,         CosP * CosY,                      0.0f,
            0.0f,                               0.0f,         0.0f,                             1.0f);
    }

    /**
     * @brief Creates and returns a rotation matrix from Roll, Pitch, and Yaw in radians
     * @param PitchYawRoll A vector containing the PitchYawRoll (x = Pitch, y = Yaw, z = Roll)
     * @return A rotation matrix
     */
    static FORCEINLINE FMatrix4 RotationRollPitchYaw(const FVector3& PitchYawRoll) noexcept
    {
        return RotationRollPitchYaw(PitchYawRoll.X, PitchYawRoll.Y, PitchYawRoll.Z);
    }

    /**
     * @brief Creates and returns a rotation matrix around the x-axis
     * @param x Rotation around the x-axis in radians
     * @return A rotation matrix
     */
    static FORCEINLINE FMatrix4 RotationX(float x) noexcept
    {
        const float SinX = FMath::Sin(x);
        const float CosX = FMath::Cos(x);

        return FMatrix4(
            1.0f,  0.0f, 0.0f, 0.0f,
            0.0f,  CosX, SinX, 0.0f,
            0.0f, -SinX, CosX, 0.0f,
            0.0f,  0.0f, 0.0f, 1.0f);
    }

    /**
     * @brief Creates and returns a rotation matrix around the y-axis
     * @param y Rotation around the y-axis in radians
     * @return A rotation matrix
     */
    static FORCEINLINE FMatrix4 RotationY(float y) noexcept
    {
        const float SinY = FMath::Sin(y);
        const float CosY = FMath::Cos(y);

        return FMatrix4(
            CosY, 0.0f, -SinY, 0.0f,
            0.0f, 1.0f,  0.0f, 0.0f,
            SinY, 0.0f,  CosY, 0.0f,
            0.0f, 0.0f,  0.0f, 1.0f);
    }

    /**
     * @brief Creates and returns a rotation matrix around the z-axis
     * @param z Rotation around the z-axis in radians
     * @return A rotation matrix
     */
    static FORCEINLINE FMatrix4 RotationZ(float z) noexcept
    {
        const float SinZ = FMath::Sin(z);
        const float CosZ = FMath::Cos(z);

        return FMatrix4(
             CosZ, SinZ, 0.0f, 0.0f,
            -SinZ, CosZ, 0.0f, 0.0f,
             0.0f, 0.0f, 1.0f, 0.0f,
             0.0f, 0.0f, 0.0f, 1.0f);
    }

    /**
     * @brief Creates an orthographic projection matrix (Left-handed)
     * @param Width Width of the projection plane in pixels
     * @param Height Height of the projection plane in pixels
     * @param NearZ The distance to the near plane in world units
     * @param FarZ The distance to the far plane in world units
     * @return An orthographic projection matrix
     */
    static FORCEINLINE FMatrix4 OrthographicProjection(float Width, float Height, float NearZ, float FarZ) noexcept
    {
        return FMatrix4(
            2.0f / Width, 0.0f,           0.0f,                   0.0f,
            0.0f,         2.0f / Height,  0.0f,                   0.0f,
            0.0f,         0.0f,           1.0f / (FarZ - NearZ),  0.0f,
            0.0f,         0.0f,          -NearZ / (FarZ - NearZ), 1.0f);
    }

    /**
     * @brief Creates an orthographic projection matrix (Left-handed)
     * @param Left Negative offset on the x-axis in world units
     * @param Right Positive offset on the x-axis in world units
     * @param Bottom Negative offset on the y-axis in world units
     * @param Top Positive offset on the y-axis in world units
     * @param NearZ The distance to the near plane in world units
     * @param FarZ The distance to the far plane in world units
     * @return An orthographic projection matrix
     */
    static FORCEINLINE FMatrix4 OrthographicProjection(float Left, float Right, float Bottom, float Top, float NearZ, float FarZ) noexcept
    {
        const float InvWidth  = 1.0f / (Right - Left);
        const float InvHeight = 1.0f / (Top - Bottom);
        const float Range     = 1.0f / (FarZ - NearZ);

        return FMatrix4(
             2.0f * InvWidth,            0.0f,                        0.0f,          0.0f,
             0.0f,                       2.0f * InvHeight,            0.0f,          0.0f,
             0.0f,                       0.0f,                        Range,         0.0f,
            -(Left + Right) * InvWidth, -(Top + Bottom) * InvHeight, -Range * NearZ, 1.0f);
    }

    /**
     * @brief Creates a perspective projection matrix (Left-handed)
     * @param Fov Field of view of the projection in radians
     * @param AspectRatio Aspect ratio of the projection (Width / Height)
     * @param NearZ The distance to the near plane in world units
     * @param FarZ The distance to the far plane in world units
     * @return A perspective projection matrix
     */
    static FORCEINLINE FMatrix4 PerspectiveProjection(float Fov, float AspectRatio, float NearZ, float FarZ) noexcept
    {
        if ((Fov < FMath::kOneDegree_f) || (Fov > (FMath::kPI_f - FMath::kOneDegree_f)))
        {
            return FMatrix4();
        }

        const float ScaleY = 1.0f / FMath::Tan(Fov * 0.5f);
        const float ScaleX = ScaleY / AspectRatio;
        const float Range  = FarZ / (FarZ - NearZ);

        return FMatrix4(
            ScaleX, 0.0f,    0.0f,          0.0f,
            0.0f,   ScaleY,  0.0f,          0.0f,
            0.0f,   0.0f,    Range,         1.0f,
            0.0f,   0.0f,   -Range * NearZ, 0.0f);
    }

    /**
     * @brief Creates a perspective projection matrix (Left-handed)
     * @param Fov Field of view of the projection in radians
     * @param Width Width of the projection plane in pixels
     * @param Height Height of the projection plane in pixels
     * @param NearZ The distance to the near plane in world units
     * @param FarZ The distance to the far plane in world units
     * @return A perspective projection matrix
     */
    static FORCEINLINE FMatrix4 PerspectiveProjection(float Fov, float Width, float Height, float NearZ, float FarZ) noexcept
    {
        const float AspectRatio = Width / Height;
        return PerspectiveProjection(Fov, AspectRatio, NearZ, FarZ);
    }

    /**
     * @brief Creates a look-at matrix (Left-handed)
     * @param Eye Position to look from
     * @param At Position to look at
     * @param Up The up-axis of the new coordinate system in the current world-space
     * @return A look-at matrix
     */
    static FORCEINLINE FMatrix4 LookAt(const FVector3& Eye, const FVector3& At, const FVector3& Up) noexcept
    {
        const FVector3 Direction = (At - Eye).GetNormalized();
        return LookTo(Eye, Direction, Up);
    }

    /**
     * @brief Creates a look-to matrix (Left-handed)
     * @param Eye Position to look from
     * @param Direction Direction to look in
     * @param Up The up-axis of the new coordinate system in the current world-space
     * @return A look-to matrix
     */
    static FORCEINLINE FMatrix4 LookTo(const FVector3& Eye, const FVector3& Direction, const FVector3& Up) noexcept
    {
        FVector3 Forward    = Direction.GetNormalized();
        FVector3 Right      = Up.CrossProduct(Forward).GetNormalized();
        FVector3 UpAdjusted = Forward.CrossProduct(Right).GetNormalized();

        float Tx = -Eye.DotProduct(Right);
        float Ty = -Eye.DotProduct(UpAdjusted);
        float Tz = -Eye.DotProduct(Forward);

        return FMatrix4(
            Right.X, UpAdjusted.X, Forward.X, 0.0f,
            Right.Y, UpAdjusted.Y, Forward.Y, 0.0f,
            Right.Z, UpAdjusted.Z, Forward.Z, 0.0f,
            Tx,      Ty,           Tz,        1.0f);
    }

public:

    /** @brief Each element of the matrix stored as a 2-D array. M[row][column], where row and column are 0-based indices. */
    float M[4][4];
};

MARK_AS_REALLOCATABLE(FMatrix4);
