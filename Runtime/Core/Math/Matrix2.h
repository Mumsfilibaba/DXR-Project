#pragma once
#include "Core/Math/Vector2.h"
#include "Core/Math/VectorMath/VectorMath.h"

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
        FMatrix2 Transpose;

    #if !USE_VECTOR_MATH
        Transpose.f[0][0] = f[0][0];
        Transpose.f[0][1] = f[1][0];
        Transpose.f[1][0] = f[0][1];
        Transpose.f[1][1] = f[1][1];
    #else
        FFloat128 This = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        This = FVectorMath::Shuffle<0, 2, 1, 3>(This);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(&Transpose));
    #endif

        return Transpose;
    }

    /**
     * @brief  - Returns the inverted version of this matrix
     * @return - Inverse matrix
     */
    inline FMatrix2 Invert() const noexcept
    {
        FMatrix2 Inverse;

        const float fDeterminant = (m00 * m11) - (m01 * m10);
    #if !USE_VECTOR_MATH
        const float RecipDeterminant = 1.0f / fDeterminant;

        Inverse.m00 =  m11 * RecipDeterminant;
        Inverse.m10 = -m10 * RecipDeterminant;
        Inverse.m01 = -m01 * RecipDeterminant;
        Inverse.m11 =  m00 * RecipDeterminant;
    #else
        FFloat128 This = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        This = FVectorMath::Shuffle<3, 2, 1, 0>(This);

        constexpr int32 Keep   = 0;
        constexpr int32 Negate = (1 << 31);

        FInt128 Mask = FVectorMath::Load(Keep, Negate, Negate, Keep);
        This = FVectorMath::Or(This, FVectorMath::CastIntToFloat(Mask));

        FFloat128 RcpDeterminant = FVectorMath::Recip(FVectorMath::Load(fDeterminant));
        This = FVectorMath::Mul(This, RcpDeterminant);
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(&Inverse));
    #endif

        return Inverse;
    }

    /**
     * @brief  - Returns the adjugate of this matrix
     * @return - Adjugate matrix
     */
    FORCEINLINE FMatrix2 Adjoint() const noexcept
    {
        FMatrix2 Adjugate;

    #if !USE_VECTOR_MATH
        Adjugate.m00 =  m11;
        Adjugate.m10 = -m10;
        Adjugate.m01 = -m01;
        Adjugate.m11 =  m00;
    #else
        FFloat128 This = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        This = FVectorMath::Shuffle<3, 2, 1, 0>(This);

        constexpr int32 Keep   = 0;
        constexpr int32 Negate = (1 << 31);

        FInt128 Mask = FVectorMath::Load(Keep, Negate, Negate, Keep);
        This = FVectorMath::Or(This, FVectorMath::CastIntToFloat(Mask));
        FVectorMath::StoreAligned(This, reinterpret_cast<float*>(&Adjugate));
    #endif

        return Adjugate;
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
            if (FMath::IsNaN(reinterpret_cast<const float*>(this)[Index]))
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
            if (FMath::IsInfinity(reinterpret_cast<const float*>(this)[Index]))
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
    bool IsEqual(const FMatrix2& Other, float Epsilon = FMath::kIsEqualEpsilon) const noexcept
    {
    #if !USE_VECTOR_MATH
        Epsilon = FMath::Abs(Epsilon);
        for (int32 i = 0; i < 4; i++)
        {
            float Diff = reinterpret_cast<const float*>(this)[i] - reinterpret_cast<const float*>(&Other)[i];
            if (FMath::Abs(Diff) > Epsilon)
            {
                return false;
            }
        }

        return true;
    #else
        FFloat128 Espilon128 = FVectorMath::Load(Epsilon);
        Espilon128 = FVectorMath::Abs(Espilon128);

        FFloat128 Diff = FVectorMath::Sub(this, &Other);
        Diff = FVectorMath::Abs(Diff);
        return FVectorMath::LessThan(Diff, Espilon128);
    #endif
    }

     /** @brief - Sets this matrix to an identity matrix */
    FORCEINLINE void SetIdentity() noexcept
    {
    #if !USE_VECTOR_MATH
        m00 = 1.0f;
        m01 = 0.0f;
        m10 = 0.0f;
        m11 = 1.0f;
    #else
        FFloat128 Constant = FVectorMath::Load(1.0f, 0.0f, 0.0f, 1.0f);
        FVectorMath::StoreAligned(Constant, reinterpret_cast<float*>(this));
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
        FVector2 Result;

    #if !USE_VECTOR_MATH
        Result.x = (RHS[0] * m00) + (RHS[1] * m10);
        Result.y = (RHS[0] * m01) + (RHS[1] * m11);
    #else
        FFloat128 X128  = FVectorMath::LoadSingle(RHS.x);
        FFloat128 Y128  = FVectorMath::LoadSingle(RHS.y);
        FFloat128 Temp0 = FVectorMath::Shuffle0011<0, 0, 0, 0>(X128, Y128);
        FFloat128 Temp1 = FVectorMath::Mul(this, Temp0);
        Temp0 = FVectorMath::Shuffle<2, 3, 2, 3>(Temp1);
        Temp1 = FVectorMath::Add(Temp0, Temp1);

        Result.x = FVectorMath::GetX(Temp1);
        Result.y = FVectorMath::GetY(Temp1);
    #endif

        return Result;
    }

    /**
     * @brief     - Multiplies a matrix with another matrix
     * @param RHS - The other matrix
     * @return    - A matrix containing the result of the multiplication
     */
    FORCEINLINE FMatrix2 operator*(const FMatrix2& RHS) const noexcept
    {
        FMatrix2 Result;
        
    #if !USE_VECTOR_MATH
        Result.m00 = (m00 * RHS.m00) + (m01 * RHS.m10);
        Result.m01 = (m01 * RHS.m11) + (m00 * RHS.m01);
        Result.m10 = (m10 * RHS.m00) + (m11 * RHS.m10);
        Result.m11 = (m11 * RHS.m11) + (m10 * RHS.m01);
    #else
        FFloat128 This = FVectorMath::LoadAligned(reinterpret_cast<const float*>(this));
        FFloat128 Temp = FVectorMath::LoadAligned(reinterpret_cast<const float*>(&RHS));
        Temp = FVectorMath::Mat2Mul(This, Temp);
        FVectorMath::StoreAligned(Temp, reinterpret_cast<float*>(&Result));
    #endif
        
        return Result;
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
        FMatrix2 Result;

    #if !USE_VECTOR_MATH
        Result.m00 = m00 * RHS;
        Result.m01 = m01 * RHS;
        Result.m10 = m10 * RHS;
        Result.m11 = m11 * RHS;
    #else
        FFloat128 Temp = FVectorMath::Load(RHS);
        Temp = FVectorMath::Mul(this, Temp);
        FVectorMath::StoreAligned(Temp, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
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
        FMatrix2 Result;

    #if !USE_VECTOR_MATH
        Result.m00 = m00 + RHS.m00;
        Result.m01 = m01 + RHS.m01;
        Result.m10 = m10 + RHS.m10;
        Result.m11 = m11 + RHS.m11;
    #else
        FFloat128 Temp = FVectorMath::Add(this, &RHS);
        FVectorMath::StoreAligned(Temp, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
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
        FMatrix2 Result;
        
    #if !USE_VECTOR_MATH
        Result.m00 = m00 + RHS;
        Result.m01 = m01 + RHS;
        Result.m10 = m10 + RHS;
        Result.m11 = m11 + RHS;
    #else
        FFloat128 Temp = FVectorMath::Load(RHS);
        Temp = FVectorMath::Add(this, Temp);
        FVectorMath::StoreAligned(Temp, reinterpret_cast<float*>(&Result));
    #endif
        
        return Result;
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
        FMatrix2 Result;

    #if !USE_VECTOR_MATH
        Result.m00 = m00 - RHS.m00;
        Result.m01 = m01 - RHS.m01;
        Result.m10 = m10 - RHS.m10;
        Result.m11 = m11 - RHS.m11;
    #else
        FFloat128 Temp = FVectorMath::Sub(this, &RHS);
        FVectorMath::StoreAligned(Temp, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
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
        FMatrix2 Result;

    #if !USE_VECTOR_MATH
        Result.m00 = m00 - RHS;
        Result.m01 = m01 - RHS;
        Result.m10 = m10 - RHS;
        Result.m11 = m11 - RHS;
    #else
        FFloat128 Temp = FVectorMath::Load(RHS);
        Temp = FVectorMath::Sub(this, Temp);
        FVectorMath::StoreAligned(Temp, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
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
        FMatrix2 Result;

    #if !USE_VECTOR_MATH
        float Recip = 1.0f / RHS;
        Result.m00 = m00 * Recip;
        Result.m01 = m01 * Recip;
        Result.m10 = m10 * Recip;
        Result.m11 = m11 * Recip;
    #else
        FFloat128 Temp = FVectorMath::Load(RHS);
        Temp = FVectorMath::Div(this, Temp);
        FVectorMath::StoreAligned(Temp, reinterpret_cast<float*>(&Result));
    #endif

        return Result;
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
        const float SinZ = FMath::Sin(Rotation);
        const float CosZ = FMath::Cos(Rotation);

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
