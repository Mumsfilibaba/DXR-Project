#pragma once
#include "Core/Math/Vector3.h"

/** @brief 3D Matrix class with float components. */
class FMatrix3
{
public:

    /** @brief Default constructor (Initializes all components to zero) */
    FMatrix3() noexcept
        : M{ { 0.0f, 0.0f, 0.0f },
             { 0.0f, 0.0f, 0.0f },
             { 0.0f, 0.0f, 0.0f } }
    {
    }

    /**
     * @brief Constructor initializing all diagonal values with a single value. The other values are set to zero.
     * @param Diagonal Value to set on the diagonal
     */
    explicit FMatrix3(float Diagonal) noexcept
        : M{ { Diagonal, 0.0f, 0.0f },
             { 0.0f, Diagonal, 0.0f },
             { 0.0f, 0.0f, Diagonal } }
    {
    }

    /**
     * @brief Constructor initializing all values with vectors specifying each row
     * @param Row0 Vector to set the first row to
     * @param Row1 Vector to set the second row to
     * @param Row2 Vector to set the third row to
     */
    explicit FMatrix3(const FVector3& Row0, const FVector3& Row1, const FVector3& Row2) noexcept
        : M{ { Row0.x, Row0.y, Row0.z },
             { Row1.x, Row1.y, Row1.z },
             { Row2.x, Row2.y, Row2.z } }
    {
    }

    /**
     * @brief Constructor initializing all values with corresponding values
     * @param M00 Value to set on row 0 and column 0
     * @param M01 Value to set on row 0 and column 1
     * @param M02 Value to set on row 0 and column 2
     * @param M10 Value to set on row 1 and column 0
     * @param M11 Value to set on row 1 and column 1
     * @param M12 Value to set on row 1 and column 2
     * @param M20 Value to set on row 2 and column 0
     * @param M21 Value to set on row 2 and column 1
     * @param M22 Value to set on row 2 and column 2
     */
    explicit FMatrix3(
        float M00, float M01, float M02,
        float M10, float M11, float M12,
        float M20, float M21, float M22) noexcept
        : M{ {M00, M01, M02},
             {M10, M11, M12},
             {M20, M21, M22} }
    {
    }

    /**
     * @brief Constructor initializing all components with an array
     * @param Array Array with at least 9 elements
     */
    explicit FMatrix3(const float* Array) noexcept
    {
        CHECK(Array != nullptr);

        M[0][0] = Array[0];
        M[0][1] = Array[1];
        M[0][2] = Array[2];
        M[1][0] = Array[3];
        M[1][1] = Array[4];
        M[1][2] = Array[5];
        M[2][0] = Array[6];
        M[2][1] = Array[7];
        M[2][2] = Array[8];
    }

    /**
     * @brief Returns the transposed version of this matrix
     * @return Transposed matrix
     */
    FMatrix3 GetTranspose() const noexcept
    {
        FMatrix3 Transposed;

        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 3; ++Col)
            {
                Transposed.M[Col][Row] = M[Row][Col];
            }
        }

        return Transposed;
    }

    /**
     * @brief Returns the inverted version of this matrix
     * @return Inverse matrix
     */
    FMatrix3 GetInverse() const noexcept
    {
        FMatrix3 Inverse;

        // Calculate the determinant
        const float Determinant = GetDeterminant();
        CHECK(Determinant != 0.0f);

        const float RcpDeterminant = 1.0f / Determinant;

        // Compute the adjugate matrix and multiply by reciprocal of determinant
        Inverse.M[0][0] =  (M[1][1] * M[2][2] - M[1][2] * M[2][1]) * RcpDeterminant;
        Inverse.M[0][1] = -(M[0][1] * M[2][2] - M[0][2] * M[2][1]) * RcpDeterminant;
        Inverse.M[0][2] =  (M[0][1] * M[1][2] - M[0][2] * M[1][1]) * RcpDeterminant;

        Inverse.M[1][0] = -(M[1][0] * M[2][2] - M[1][2] * M[2][0]) * RcpDeterminant;
        Inverse.M[1][1] =  (M[0][0] * M[2][2] - M[0][2] * M[2][0]) * RcpDeterminant;
        Inverse.M[1][2] = -(M[0][0] * M[1][2] - M[0][2] * M[1][0]) * RcpDeterminant;

        Inverse.M[2][0] =  (M[1][0] * M[2][1] - M[1][1] * M[2][0]) * RcpDeterminant;
        Inverse.M[2][1] = -(M[0][0] * M[2][1] - M[0][1] * M[2][0]) * RcpDeterminant;
        Inverse.M[2][2] =  (M[0][0] * M[1][1] - M[0][1] * M[1][0]) * RcpDeterminant;

        return Inverse;
    }

    /**
     * @brief Returns the adjugate of this matrix
     * @return Adjugate matrix
     */
    FMatrix3 GetAdjugate() const noexcept
    {
        FMatrix3 Adjugate;

        Adjugate.M[0][0] =  (M[1][1] * M[2][2] - M[1][2] * M[2][1]);
        Adjugate.M[0][1] = -(M[0][1] * M[2][2] - M[0][2] * M[2][1]);
        Adjugate.M[0][2] =  (M[0][1] * M[1][2] - M[0][2] * M[1][1]);

        Adjugate.M[1][0] = -(M[1][0] * M[2][2] - M[1][2] * M[2][0]);
        Adjugate.M[1][1] =  (M[0][0] * M[2][2] - M[0][2] * M[2][0]);
        Adjugate.M[1][2] = -(M[0][0] * M[1][2] - M[0][2] * M[1][0]);

        Adjugate.M[2][0] =  (M[1][0] * M[2][1] - M[1][1] * M[2][0]);
        Adjugate.M[2][1] = -(M[0][0] * M[2][1] - M[0][1] * M[2][0]);
        Adjugate.M[2][2] =  (M[0][0] * M[1][1] - M[0][1] * M[1][0]);

        return Adjugate;
    }

    /**
     * @brief Returns the determinant of this matrix
     * @return The determinant
     */
    float GetDeterminant() const noexcept
    {
        float a = M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]);
        float b = M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0]);
        float c = M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0]);
        return a - b + c;
    }

    /**
     * @brief Checks whether this matrix has any value that equals NaN
     * @return True if any value equals NaN, false otherwise
     */
    bool ContainsNaN() const noexcept
    {
        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 3; ++Col)
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
    bool ContainsInfinity() const noexcept
    {
        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 3; ++Col)
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
    bool IsEqual(const FMatrix3& Other, float Epsilon = FMath::kIsEqualEpsilon) const noexcept
    {
        Epsilon = FMath::Abs(Epsilon);

        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 3; ++Col)
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

    /** 
     * @brief Sets this matrix to an identity matrix 
     */
    void SetIdentity() noexcept
    {
        M[0][0] = 1.0f; M[0][1] = 0.0f; M[0][2] = 0.0f;
        M[1][0] = 0.0f; M[1][1] = 1.0f; M[1][2] = 0.0f;
        M[2][0] = 0.0f; M[2][1] = 0.0f; M[2][2] = 1.0f;
    }

    /**
     * @brief Returns a row of this matrix
     * @param Row The row to retrieve (0, 1, or 2)
     * @return A vector containing the specified row
     */
    FVector3 GetRow(int32 Row) const noexcept
    {
        CHECK(Row < 3);
        return FVector3(M[Row][0], M[Row][1], M[Row][2]);
    }

    /**
     * @brief Returns a column of this matrix
     * @param Column The column to retrieve (0, 1, or 2)
     * @return A vector containing the specified column
     */
    FVector3 GetColumn(int32 Column) const noexcept
    {
        CHECK(Column < 3);
        return FVector3(M[0][Column], M[1][Column], M[2][Column]);
    }

public:

    /**
     * @brief Transforms a 3-D vector
     * @param Other The vector to transform
     * @return A vector containing the transformation
     */
    FVector3 operator*(const FVector3& Other) const noexcept
    {
        FVector3 Result;
        Result.x = (Other.x * M[0][0]) + (Other.y * M[1][0]) + (Other.z * M[2][0]);
        Result.y = (Other.x * M[0][1]) + (Other.y * M[1][1]) + (Other.z * M[2][1]);
        Result.z = (Other.x * M[0][2]) + (Other.y * M[1][2]) + (Other.z * M[2][2]);
        return Result;
    }

    /**
     * @brief Multiplies a matrix with another matrix
     * @param Other The other matrix
     * @return A matrix containing the result of the multiplication
     */
    FMatrix3 operator*(const FMatrix3& Other) const noexcept
    {
        FMatrix3 Result;

        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 3; ++Col)
            {
                Result.M[Row][Col] = 0.0f;
                for (int32 k = 0; k < 3; ++k)
                {
                    Result.M[Row][Col] += M[Row][k] * Other.M[k][Col];
                }
            }
        }

        return Result;
    }

    /**
     * @brief Multiplies this matrix with another matrix
     * @param Other The other matrix
     * @return A reference to this matrix after multiplication
     */
    FMatrix3& operator*=(const FMatrix3& Other) noexcept
    {
        *this = *this * Other;
        return *this;
    }

    /**
     * @brief Multiplies this matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A matrix containing the result of the multiplication
     */
    FMatrix3 operator*(float Scalar) const noexcept
    {
        FMatrix3 Result;

        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 3; ++Col)
            {
                Result.M[Row][Col] = M[Row][Col] * Scalar;
            }
        }

        return Result;
    }

    /**
     * @brief Multiplies this matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A reference to this matrix after multiplication
     */
    FMatrix3& operator*=(float Scalar) noexcept
    {
        *this = *this * Scalar;
        return *this;
    }

    /**
     * @brief Adds a matrix component-wise with another matrix
     * @param Other The other matrix
     * @return A matrix containing the result of the addition
     */
    FMatrix3 operator+(const FMatrix3& Other) const noexcept
    {
        FMatrix3 Result;

        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 3; ++Col)
            {
                Result.M[Row][Col] = M[Row][Col] + Other.M[Row][Col];
            }
        }

        return Result;
    }

    /**
     * @brief Adds this matrix component-wise with another matrix
     * @param Other The other matrix
     * @return A reference to this matrix after addition
     */
    FMatrix3& operator+=(const FMatrix3& Other) noexcept
    {
        *this = *this + Other;
        return *this;
    }

    /**
     * @brief Adds this matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A matrix containing the result of the addition
     */
    FMatrix3 operator+(float Scalar) const noexcept
    {
        FMatrix3 Result;

        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 3; ++Col)
            {
                Result.M[Row][Col] = M[Row][Col] + Scalar;
            }
        }

        return Result;
    }

    /**
     * @brief Adds this matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A reference to this matrix after addition
     */
    FMatrix3& operator+=(float Scalar) noexcept
    {
        *this = *this + Scalar;
        return *this;
    }

    /**
     * @brief Subtracts a matrix component-wise with another matrix
     * @param Other The other matrix
     * @return A matrix containing the result of the subtraction
     */
    FMatrix3 operator-(const FMatrix3& Other) const noexcept
    {
        FMatrix3 Result;

        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 3; ++Col)
            {
                Result.M[Row][Col] = M[Row][Col] - Other.M[Row][Col];
            }
        }

        return Result;
    }

    /**
     * @brief Subtracts this matrix component-wise with another matrix
     * @param Other The other matrix
     * @return A reference to this matrix after subtraction
     */
    FMatrix3& operator-=(const FMatrix3& Other) noexcept
    {
        *this = *this - Other;
        return *this;
    }

    /**
     * @brief Subtracts this matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A matrix containing the result of the subtraction
     */
    FMatrix3 operator-(float Scalar) const noexcept
    {
        FMatrix3 Result;

        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 3; ++Col)
            {
                Result.M[Row][Col] = M[Row][Col] - Scalar;
            }
        }

        return Result;
    }

    /**
     * @brief Subtracts this matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A reference to this matrix after subtraction
     */
    FMatrix3& operator-=(float Scalar) noexcept
    {
        *this = *this - Scalar;
        return *this;
    }

    /**
     * @brief Divides this matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A matrix containing the result of the division
     */
    FMatrix3 operator/(float Scalar) const noexcept
    {
        const float Recip = 1.0f / Scalar;

        FMatrix3 Result;

        for (int32 Row = 0; Row < 3; ++Row)
        {
            for (int32 Col = 0; Col < 3; ++Col)
            {
                Result.M[Row][Col] = M[Row][Col] * Recip;
            }
        }

        return Result;
    }

    /**
     * @brief Divides this matrix component-wise with a scalar
     * @param Scalar The scalar
     * @return A reference to this matrix after division
     */
    FMatrix3& operator/=(float Scalar) noexcept
    {
        *this = *this / Scalar;
        return *this;
    }

    /**
     * @brief Compares this matrix with another matrix for equality
     * @param Other The matrix to compare with
     * @return True if matrices are equal, false otherwise
     */
    bool operator==(const FMatrix3& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief Compares this matrix with another matrix for inequality
     * @param Other The matrix to compare with
     * @return True if matrices are not equal, false otherwise
     */
    bool operator!=(const FMatrix3& Other) const noexcept
    {
        return !IsEqual(Other);
    }

public:

    /**
     * @brief Creates and returns an identity matrix
     * @return An identity matrix
     */
    static FMatrix3 Identity() noexcept
    {
        return FMatrix3(1.0f);
    }

    /**
     * @brief Creates and returns a uniform scale matrix
     * @param Scale Uniform scale that represents this matrix
     * @return A scale matrix
     */
    static FMatrix3 Scale(float Scale) noexcept
    {
        return FMatrix3(Scale);
    }

    /**
     * @brief Creates and returns a scale matrix for each axis
     * @param InX Scale for the x-axis
     * @param InY Scale for the y-axis
     * @param InZ Scale for the z-axis
     * @return A scale matrix
     */
    static FMatrix3 Scale(float InX, float InY, float InZ) noexcept
    {
        return FMatrix3(
            InX, 0.0f, 0.0f,
            0.0f, InY, 0.0f,
            0.0f, 0.0f, InZ);
    }

    /**
     * @brief Creates and returns a scale matrix using a vector for scaling each axis
     * @param VectorWithScale A vector containing the scale for each axis in the x-, y-, z-components
     * @return A scale matrix
     */
    static FMatrix3 Scale(const FVector3& VectorWithScale) noexcept
    {
        return Scale(VectorWithScale.x, VectorWithScale.y, VectorWithScale.z);
    }

    /**
     * @brief Creates and returns a rotation matrix from Roll, Pitch, and Yaw in radians
     * @param Pitch Rotation around the x-axis in radians
     * @param Yaw Rotation around the y-axis in radians
     * @param Roll Rotation around the z-axis in radians
     * @return A rotation matrix
     */
    static FMatrix3 RotationRollPitchYaw(float Pitch, float Yaw, float Roll) noexcept
    {
        const float SinP = FMath::Sin(Pitch);
        const float SinY = FMath::Sin(Yaw);
        const float SinR = FMath::Sin(Roll);
        const float CosP = FMath::Cos(Pitch);
        const float CosY = FMath::Cos(Yaw);
        const float CosR = FMath::Cos(Roll);

        const float SinRSinP = SinR * SinP;
        const float CosRSinP = CosR * SinP;

        return FMatrix3(
            (CosR * CosY) + (SinRSinP * SinY), SinR * CosP, (SinRSinP * CosY) - (CosR * SinY),
            (CosRSinP * SinY) - (SinR * CosY), CosR * CosP, (SinR * SinY) + (CosRSinP * CosY),
            CosP * SinY, -SinP, CosP * CosY);
    }

    /**
     * @brief Creates and returns a rotation matrix around the x-axis
     * @param AxisX Rotation around the x-axis in radians
     * @return A rotation matrix
     */
    static FMatrix3 RotationX(float AxisX) noexcept
    {
        const float SinX = FMath::Sin(AxisX);
        const float CosX = FMath::Cos(AxisX);

        return FMatrix3(
            1.0f,  0.0f, 0.0f,
            0.0f,  CosX, SinX,
            0.0f, -SinX, CosX);
    }

    /**
     * @brief Creates and returns a rotation matrix around the y-axis
     * @param AxisY Rotation around the y-axis in radians
     * @return A rotation matrix
     */
    static FMatrix3 RotationY(float AxisY) noexcept
    {
        const float SinY = FMath::Sin(AxisY);
        const float CosY = FMath::Cos(AxisY);

        return FMatrix3(
            CosY, 0.0f, -SinY,
            0.0f, 1.0f,  0.0f,
            SinY, 0.0f,  CosY);
    }

    /**
     * @brief Creates and returns a rotation matrix around the z-axis
     * @param AxisZ Rotation around the z-axis in radians
     * @return A rotation matrix
     */
    static FMatrix3 RotationZ(float AxisZ) noexcept
    {
        const float SinZ = FMath::Sin(AxisZ);
        const float CosZ = FMath::Cos(AxisZ);

        return FMatrix3(
             CosZ, SinZ, 0.0f,
            -SinZ, CosZ, 0.0f,
             0.0f, 0.0f, 1.0f);
    }

public:

    /** @brief Each element of the matrix stored as a 2-D array. M[row][column], where row and column are 0-based indices. */
    float M[3][3];
};

MARK_AS_REALLOCATABLE(FMatrix3);
