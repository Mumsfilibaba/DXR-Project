#pragma once
#include "Vector2.h"
#include "VectorOp.h"

class VECTOR_ALIGN FMatrix2
{
public:

    /** 
     * @brief - Default constructor (Initialize components to zero) 
     */
    FORCEINLINE FMatrix2() noexcept 
        : m00(0.0f), m01(0.0f)
        , m10(0.0f), m11(0.0f)
    {
    }

    /**
     * @brief          - Constructor initializing all values on the diagonal with a single value. The other values are set to zero.
     * @param Diagonal - Value to set on the diagonal
     */
    FORCEINLINE explicit FMatrix2(float Diagonal) noexcept
        : m00(Diagonal), m01(0.0f)
        , m10(0.0f), m11(Diagonal)
    {
    }

    /**
     * @brief      - Constructor initializing all values with vectors specifying each row
     * @param Row0 - Vector to set the first row to
     * @param Row1 - Vector to set the second row to
     */
    FORCEINLINE explicit FMatrix2(const FVector2& Row0, const FVector2& Row1) noexcept
        : m00(Row0.x), m01(Row0.y)
        , m10(Row1.x), m11(Row1.y)
    {
    }

    /**
     * @brief      - Constructor initializing all values with corresponding value
     * @param In00 - Value to set on row 0 and column 0
     * @param In01 - Value to set on row 0 and column 1
     * @param In10 - Value to set on row 1 and column 0
     * @param In11 - Value to set on row 1 and column 1
     */
    FORCEINLINE explicit FMatrix2(float In00, float In01, float In10, float In11) noexcept
        : m00(In00), m01(In01)
        , m10(In10), m11(In11)
    {
    }

    /**
     * @brief     - Constructor initializing all components with an array
     * @param Arr - Array with at least 4 elements
     */
    FORCEINLINE explicit FMatrix2(const float* Arr) noexcept
        : m00(Arr[0]), m01(Arr[1])
        , m10(Arr[2]), m11(Arr[3])
    {
    }

    /**
     * @brief  - Returns the transposed version of this matrix
     * @return - Transposed matrix
     */
    inline FMatrix2 Transpose() const noexcept
    {
#if !USE_VECTOR_OP
        FMatrix2 Transpose;
        Transpose.f[0][0] = f[0][0];
        Transpose.f[0][1] = f[1][0];

        Transpose.f[1][0] = f[0][1];
        Transpose.f[1][1] = f[1][1];
        return Transpose;
#else
        FMatrix2 Transpose;

        NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
        This = NVectorOp::Shuffle<0, 2, 1, 3>(This);

        NVectorOp::StoreAligned(This, &Transpose);
        return Transpose;
#endif
    }

    /**
     * @brief  - Returns the inverted version of this matrix
     * @return - Inverse matrix
     */
    inline FMatrix2 Invert() const noexcept
    {
        const float fDeterminant = (m00 * m11) - (m01 * m10);

#if !USE_VECTOR_OP
        const float RecipDeterminant = 1.0f / fDeterminant;

        FMatrix2 Inverse;
        Inverse.m00 =  m11 * RecipDeterminant;
        Inverse.m10 = -m10 * RecipDeterminant;
        Inverse.m01 = -m01 * RecipDeterminant;
        Inverse.m11 =  m00 * RecipDeterminant;
        return Inverse;
#else
        FMatrix2 Inverse;

        NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
        This = NVectorOp::Shuffle<3, 2, 1, 0>(This);

        CONSTEXPR int32 Keep   = 0;
        CONSTEXPR int32 Negate = (1 << 31);

        NVectorOp::Int128 Mask = NVectorOp::Load(Keep, Negate, Negate, Keep);
        This = NVectorOp::Or(This, NVectorOp::CastIntToFloat(Mask));

        NVectorOp::Float128 RcpDeterminant = NVectorOp::Recip(NVectorOp::Load(fDeterminant));
        This = NVectorOp::Mul(This, RcpDeterminant);

        NVectorOp::StoreAligned(This, &Inverse);
        return Inverse;
#endif
    }

    /**
     * @brief  - Returns the adjugate of this matrix
     * @return - Adjugate matrix
     */
    FORCEINLINE FMatrix2 Adjoint() const noexcept
    {
#if !USE_VECTOR_OP
        FMatrix2 Adjugate;
        Adjugate.m00 =  m11;
        Adjugate.m10 = -m10;
        Adjugate.m01 = -m01;
        Adjugate.m11 =  m00;
        return Adjugate;
#else
        FMatrix2 Adjugate;

        NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
        This = NVectorOp::Shuffle<3, 2, 1, 0>(This);

        CONSTEXPR int32 Keep   = 0;
        CONSTEXPR int32 Negate = (1 << 31);

        NVectorOp::Int128 Mask = NVectorOp::Load(Keep, Negate, Negate, Keep);
        This = NVectorOp::Or(This, NVectorOp::CastIntToFloat(Mask));

        NVectorOp::StoreAligned(This, &Adjugate);
        return Adjugate;
#endif
    }

    /**
     * @brief  - Returns the determinant of this matrix
     * @return - The determinant
     */
    FORCEINLINE float Determinant() const noexcept
    {
        return (m00 * m11) - (m01 * m10);
    }

    /**
     * @brief  - Checks weather this matrix has any value that equals NaN
     * @return - True if the any value equals NaN, false if not
     */
    bool HasNaN() const noexcept
    {
        for (int32 Index = 0; Index < 4; ++Index)
        {
            if (NMath::IsNaN(reinterpret_cast<const float*>(this)[Index]))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief  - Checks weather this matrix has any value that equals infinity
     * @return - True if the any value equals infinity, false if not
     */
    bool HasInfinity() const noexcept
    {
        for (int32 Index = 0; Index < 4; ++Index)
        {
            if (NMath::IsInfinity(reinterpret_cast<const float*>(this)[Index]))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief  - Checks weather this matrix has any value that equals infinity or NaN
     * @return - False if the any value equals infinity or NaN, true if not
     */
    FORCEINLINE bool IsValid() const noexcept
    {
        return !HasNaN() && !HasInfinity();
    }

    /**
     * @brief       - Compares, within a threshold Epsilon, this matrix with another matrix
     * @param Other - matrix to compare against
     * @return      - True if equal, false if not
     */
    bool IsEqual(const FMatrix2& Other, float Epsilon = NMath::kIsEqualEpsilon) const noexcept
    {
#if !USE_VECTOR_OP

        Epsilon = NMath::Abs(Epsilon);

        for (int32 i = 0; i < 4; i++)
        {
            float Diff = reinterpret_cast<const float*>(this)[i] - reinterpret_cast<const float*>(&Other)[i];
            if (NMath::Abs(Diff) > Epsilon)
            {
                return false;
            }
        }

        return true;

#else
        NVectorOp::Float128 Espilon128 = NVectorOp::Load(Epsilon);
        Espilon128 = NVectorOp::Abs(Espilon128);

        NVectorOp::Float128 Diff = NVectorOp::Sub(this, &Other);
        Diff = NVectorOp::Abs(Diff);
        return NVectorOp::LessThan(Diff, Espilon128);
#endif
    }

     /** @brief - Sets this matrix to an identity matrix */
    FORCEINLINE void SetIdentity() noexcept
    {
#if !USE_VECTOR_OP
        m00 = 1.0f;
        m01 = 0.0f;

        m10 = 0.0f;
        m11 = 1.0f;
#else
        NVectorOp::Float128 Constant = NVectorOp::Load(1.0f, 0.0f, 0.0f, 1.0f);
        NVectorOp::StoreAligned(Constant, this);
#endif
    }

    /**
     * @brief     - Returns a row of this matrix
     * @param Row - The row to retrieve
     * @return    - A vector containing the specified row
     */
    FORCEINLINE FVector2 GetRow(int32 Row) const noexcept
    {
        CHECK(Row < 2);
        return FVector2(f[Row]);
    }

    /**
     * @brief        - Returns a column of this matrix
     * @param Column - The column to retrieve
     * @return       - A vector containing the specified column
     */
    FORCEINLINE FVector2 GetColumn(int32 Column) const noexcept
    {
        CHECK(Column < 2);
        return FVector2(f[0][Column], f[1][Column]);
    }

    /**
     * @brief  - Returns the data of this matrix as a pointer
     * @return - A pointer to the data
     */
    FORCEINLINE float* Data() noexcept
    {
        return reinterpret_cast<float*>(this);
    }

    /**
     * @brief  - Returns the data of this matrix as a pointer
     * @return - A pointer to the data
     */
    FORCEINLINE const float* Data() const noexcept
    {
        return reinterpret_cast<const float*>(this);
    }

public:

    /**
     * @brief     - Transforms a 2-D vector
     * @param RHS - The vector to transform
     * @return    - A vector containing the transformation
     */
    FORCEINLINE FVector2 operator*(const FVector2& RHS) const noexcept
    {
#if !USE_VECTOR_OP
        FVector2 Result;
        Result.x = (RHS[0] * m00) + (RHS[1] * m10);
        Result.y = (RHS[0] * m01) + (RHS[1] * m11);
        return Result;
#else
        FVector2 Result;

        NVectorOp::Float128 X128  = NVectorOp::LoadSingle(RHS.x);
        NVectorOp::Float128 Y128  = NVectorOp::LoadSingle(RHS.y);
        NVectorOp::Float128 Temp0 = NVectorOp::Shuffle0011<0, 0, 0, 0>(X128, Y128);
        NVectorOp::Float128 Temp1 = NVectorOp::Mul(this, Temp0);
        Temp0 = NVectorOp::Shuffle<2, 3, 2, 3>(Temp1);
        Temp1 = NVectorOp::Add(Temp0, Temp1);

        Result.x = NVectorOp::GetX(Temp1);
        Result.y = NVectorOp::GetY(Temp1);
        return Result;
#endif
    }

    /**
     * @brief     - Multiplies a matrix with another matrix
     * @param RHS - The other matrix
     * @return    - A matrix containing the result of the multiplication
     */
    FORCEINLINE FMatrix2 operator*(const FMatrix2& RHS) const noexcept
    {
#if !USE_VECTOR_OP
        FMatrix2 Result;
        Result.m00 = (m00 * RHS.m00) + (m01 * RHS.m10);
        Result.m01 = (m01 * RHS.m11) + (m00 * RHS.m01);

        Result.m10 = (m10 * RHS.m00) + (m11 * RHS.m10);
        Result.m11 = (m11 * RHS.m11) + (m10 * RHS.m01);
        return Result;
#else
        FMatrix2 Result;

        NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
        NVectorOp::Float128 Temp = NVectorOp::LoadAligned(&RHS);
        Temp = NVectorOp::Mat2Mul(This, Temp);

        NVectorOp::StoreAligned(Temp, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Multiplies this matrix with another matrix
     * @param RHS - The other matrix
     * @return    - A reference to this matrix
     */
    FORCEINLINE FMatrix2& operator*=(const FMatrix2& RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief     - Multiplies a matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A matrix containing the result of the multiplication
     */
    FORCEINLINE FMatrix2 operator*(float RHS) const noexcept
    {
#if !USE_VECTOR_OP
        FMatrix2 Result;
        Result.m00 = m00 * RHS;
        Result.m01 = m01 * RHS;

        Result.m10 = m10 * RHS;
        Result.m11 = m11 * RHS;
        return Result;
#else
        FMatrix2 Result;

        NVectorOp::Float128 Temp = NVectorOp::Load(RHS);
        Temp = NVectorOp::Mul(this, Temp);

        NVectorOp::StoreAligned(Temp, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Multiplies this matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A reference to this matrix
     */
    FORCEINLINE FMatrix2& operator*=(float RHS) noexcept
    {
        return *this = *this * RHS;
    }

    /**
     * @brief     - Adds a matrix component-wise with another matrix
     * @param RHS - The other matrix
     * @return    - A matrix containing the result of the addition
     */
    FORCEINLINE FMatrix2 operator+(const FMatrix2& RHS) const noexcept
    {
#if !USE_VECTOR_OP
        FMatrix2 Result;
        Result.m00 = m00 + RHS.m00;
        Result.m01 = m01 + RHS.m01;

        Result.m10 = m10 + RHS.m10;
        Result.m11 = m11 + RHS.m11;
        return Result;
#else
        FMatrix2 Result;
        NVectorOp::Float128 Temp = NVectorOp::Add(this, &RHS);
        NVectorOp::StoreAligned(Temp, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Adds this matrix component-wise with another matrix
     * @param RHS - The other matrix
     * @return    - A reference to this matrix
     */
    FORCEINLINE FMatrix2& operator+=(const FMatrix2& RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief     - Adds a matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A matrix containing the result of the addition
     */
    FORCEINLINE FMatrix2 operator+(float RHS) const noexcept
    {
#if !USE_VECTOR_OP
        FMatrix2 Result;
        Result.m00 = m00 + RHS;
        Result.m01 = m01 + RHS;

        Result.m10 = m10 + RHS;
        Result.m11 = m11 + RHS;
        return Result;
#else
        FMatrix2 Result;

        NVectorOp::Float128 Temp = NVectorOp::Load(RHS);
        Temp = NVectorOp::Add(this, Temp);

        NVectorOp::StoreAligned(Temp, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Adds this matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A reference to this matrix
     */
    FORCEINLINE FMatrix2& operator+=(float RHS) noexcept
    {
        return *this = *this + RHS;
    }

    /**
     * @brief     - Subtracts a matrix component-wise with another matrix
     * @param RHS - The other matrix
     * @return    - A matrix containing the result of the subtraction
     */
    FORCEINLINE FMatrix2 operator-(const FMatrix2& RHS) const noexcept
    {
#if !USE_VECTOR_OP
        FMatrix2 Result;
        Result.m00 = m00 - RHS.m00;
        Result.m01 = m01 - RHS.m01;

        Result.m10 = m10 - RHS.m10;
        Result.m11 = m11 - RHS.m11;
        return Result;
#else
        FMatrix2 Result;
        NVectorOp::Float128 Temp = NVectorOp::Sub(this, &RHS);
        NVectorOp::StoreAligned(Temp, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Subtracts this matrix component-wise with another matrix
     * @param RHS - The other matrix
     * @return    - A reference to this matrix
     */
    FORCEINLINE FMatrix2& operator-=(const FMatrix2& RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief     - Subtracts a matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A matrix containing the result of the subtraction
     */
    FORCEINLINE FMatrix2 operator-(float RHS) const noexcept
    {
#if !USE_VECTOR_OP
        FMatrix2 Result;
        Result.m00 = m00 - RHS;
        Result.m01 = m01 - RHS;

        Result.m10 = m10 - RHS;
        Result.m11 = m11 - RHS;
        return Result;
#else
        FMatrix2 Result;

        NVectorOp::Float128 Temp = NVectorOp::Load(RHS);
        Temp = NVectorOp::Sub(this, Temp);

        NVectorOp::StoreAligned(Temp, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Subtracts this matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A reference to this matrix
     */
    FORCEINLINE FMatrix2& operator-=(float RHS) noexcept
    {
        return *this = *this - RHS;
    }

    /**
     * @brief     - Divides a matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A matrix containing the result of the division
     */
    FORCEINLINE FMatrix2 operator/(float RHS) const noexcept
    {
#if !USE_VECTOR_OP
        float Recip = 1.0f / RHS;

        FMatrix2 Result;
        Result.m00 = m00 * Recip;
        Result.m01 = m01 * Recip;

        Result.m10 = m10 * Recip;
        Result.m11 = m11 * Recip;
        return Result;

#else
        FMatrix2 Result;

        NVectorOp::Float128 Temp = NVectorOp::Load(RHS);
        Temp = NVectorOp::Div(this, Temp);

        NVectorOp::StoreAligned(Temp, &Result);
        return Result;
#endif
    }

    /**
     * @brief     - Divides this matrix component-wise with a scalar
     * @param RHS - The scalar
     * @return    - A reference to this matrix
     */
    FORCEINLINE FMatrix2& operator/=(float RHS) noexcept
    {
        return *this = *this / RHS;
    }

    /**
     * @brief       - Returns the result after comparing this and another matrix
     * @param Other - The matrix to compare with
     * @return      - True if equal, false if not
     */
    FORCEINLINE bool operator==(const FMatrix2& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief       - Returns the negated result after comparing this and another matrix
     * @param Other - The matrix to compare with
     * @return      - False if equal, true if not
     */
    FORCEINLINE bool operator!=(const FMatrix2& Other) const noexcept
    {
        return !IsEqual(Other);
    }

public:

    /**
     * @brief  - Creates and returns a identity matrix
     * @return - A identity matrix
     */
    static FORCEINLINE FMatrix2 Identity() noexcept
    {
        return FMatrix2(1.0f);
    }

    /**
     * @brief       - Creates and returns a uniform scale matrix
     * @param Scale - Uniform scale that represents this matrix
     * @return      - A scale matrix
     */
    static FORCEINLINE FMatrix2 Scale(float Scale) noexcept
    {
        return FMatrix2(Scale);
    }

    /**
     * @brief   - Creates and returns a scale matrix for each axis
     * @param x - Scale for the x-axis
     * @param y - Scale for the y-axis
     * @return  - A scale matrix
     */
    static FORCEINLINE FMatrix2 Scale(float x, float y) noexcept
    {
        return FMatrix2(x, 0.0f, 0.0f, y);
    }

    /**
     * @brief                 - Creates and returns a scale matrix for each axis
     * @param VectorWithScale - A vector containing the scale for each axis in the x-, and y-components
     * @return                - A scale matrix
     */
    static FORCEINLINE FMatrix2 Scale(const FVector2& VectorWithScale) noexcept
    {
        return Scale(VectorWithScale.x, VectorWithScale.y);
    }

    /**
     * @brief          - Creates and returns a rotation matrix around the x-axis
     * @param Rotation - Rotation around in radians
     * @return         - A rotation matrix
     */
    static FORCEINLINE FMatrix2 Rotation(float Rotation) noexcept
    {
        const float SinZ = NMath::Sin(Rotation);
        const float CosZ = NMath::Cos(Rotation);

        return FMatrix2(CosZ, SinZ, -SinZ, CosZ);
    }

public:
    union
    {
         /** @brief - Each element of the matrix */
        struct
        {
            float m00, m01;
            float m10, m11;
        };

         /** @brief - 2-D array of the matrix */
        float f[2][2];
    };
};

MARK_AS_REALLOCATABLE(FMatrix2);