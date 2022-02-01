#pragma once
#include "Vector2.h"
#include "SIMD.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// A 2x2 matrix with SIMD capabilities

class VECTOR_ALIGN CMatrix2
{
public:

    /** Default constructor (Initialize components to zero) */
    FORCEINLINE CMatrix2() noexcept;

    /**
     * Constructor initializing all values on the diagonal with a single value. The other values are set to zero.
     *
     * @param Diagonal: Value to set on the diagonal
     */
    FORCEINLINE explicit CMatrix2(float Diagonal) noexcept;

    /**
     * Constructor initializing all values with vectors specifying each row
     *
     * @param Row0: Vector to set the first row to
     * @param Row1: Vector to set the second row to
     */
    FORCEINLINE explicit CMatrix2(const CVector2& Row0, const CVector2& Row1) noexcept;

    /**
     * Constructor initializing all values with corresponding value
     *
     * @param In00: Value to set on row 0 and column 0
     * @param In01: Value to set on row 0 and column 1
     * @param In10: Value to set on row 1 and column 0
     * @param In11: Value to set on row 1 and column 1
     */
    FORCEINLINE explicit CMatrix2(float In00, float In01, float In10, float In11) noexcept;

    /**
     * Constructor initializing all components with an array
     *
     * @param Arr: Array with at least 4 elements
     */
    FORCEINLINE explicit CMatrix2(const float* Arr) noexcept;

    /**
     * Returns the transposed version of this matrix
     *
     * @return Transposed matrix
     */
    inline CMatrix2 Transpose() const noexcept;

    /**
     * Returns the inverted version of this matrix
     *
     * @return Inverse matrix
     */
    inline CMatrix2 Invert() const noexcept;

    /**
     * Returns the adjugate of this matrix
     *
     * @return Adjugate matrix
     */
    FORCEINLINE CMatrix2 Adjoint() const noexcept;

    /**
     * Returns the determinant of this matrix
     *
     * @return The determinant
     */
    FORCEINLINE float Determinant() const noexcept;

    /**
     * Checks weather this matrix has any value that equals NaN
     *
     * @return True if the any value equals NaN, false if not
     */
    inline bool HasNan() const noexcept;

    /**
     * Checks weather this matrix has any value that equals infinity
     *
     * @return True if the any value equals infinity, false if not
     */
    inline bool HasInfinity() const noexcept;

    /**
     * Checks weather this matrix has any value that equals infinity or NaN
     *
     * @return False if the any value equals infinity or NaN, true if not
     */
    FORCEINLINE bool IsValid() const noexcept;

    /**
     * Compares, within a threshold Epsilon, this matrix with another matrix
     *
     * @param Other: matrix to compare against
     * @return True if equal, false if not
     */
    inline bool IsEqual(const CMatrix2& Other, float Epsilon = NMath::IS_EQUAL_EPISILON) const noexcept;

    /* Sets this matrix to an identity matrix */
    FORCEINLINE void SetIdentity() noexcept;

    /**
     * Returns a row of this matrix
     *
     * @param Row: The row to retrieve
     * @return A vector containing the specified row
     */
    FORCEINLINE CVector2 GetRow(int Row) const noexcept;

    /**
     * Returns a column of this matrix
     *
     * @param Column: The column to retrieve
     * @return A vector containing the specified column
     */
    FORCEINLINE CVector2 GetColumn(int Column) const noexcept;

    /**
     * Returns the data of this matrix as a pointer
     *
     * @return A pointer to the data
     */
    FORCEINLINE float* GetData() noexcept;

    /**
     * Returns the data of this matrix as a pointer
     *
     * @return A pointer to the data
     */
    FORCEINLINE const float* GetData() const noexcept;

public:

    /**
     * Transforms a 2-D vector
     *
     * @param Rhs: The vector to transform
     * @return A vector containing the transformation
     */
    FORCEINLINE CVector2 operator*(const CVector2& Rhs) const noexcept;

    /**
     * Multiplies a matrix with another matrix
     *
     * @param Rhs: The other matrix
     * @return A matrix containing the result of the multiplication
     */
    FORCEINLINE CMatrix2 operator*(const CMatrix2& Rhs) const noexcept;

    /**
     * Multiplies this matrix with another matrix
     *
     * @param Rhs: The other matrix
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix2& operator*=(const CMatrix2& Rhs) noexcept;

    /**
     * Multiplies a matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A matrix containing the result of the multiplication
     */
    FORCEINLINE CMatrix2 operator*(float Rhs) const noexcept;

    /**
     * Multiplies this matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix2& operator*=(float Rhs) noexcept;

    /**
     * Adds a matrix component-wise with another matrix
     *
     * @param Rhs: The other matrix
     * @return A matrix containing the result of the addition
     */
    FORCEINLINE CMatrix2 operator+(const CMatrix2& Rhs) const noexcept;

    /**
     * Adds this matrix component-wise with another matrix
     *
     * @param Rhs: The other matrix
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix2& operator+=(const CMatrix2& Rhs) noexcept;

    /**
     * Adds a matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A matrix containing the result of the addition
     */
    FORCEINLINE CMatrix2 operator+(float Rhs) const noexcept;

    /**
     * Adds this matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix2& operator+=(float Rhs) noexcept;

    /**
     * Subtracts a matrix component-wise with another matrix
     *
     * @param Rhs: The other matrix
     * @return A matrix containing the result of the subtraction
     */
    FORCEINLINE CMatrix2 operator-(const CMatrix2& Rhs) const noexcept;

    /**
     * Subtracts this matrix component-wise with another matrix
     *
     * @param Rhs: The other matrix
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix2& operator-=(const CMatrix2& Rhs) noexcept;

    /**
     * Subtracts a matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A matrix containing the result of the subtraction
     */
    FORCEINLINE CMatrix2 operator-(float Rhs) const noexcept;

    /**
     * Subtracts this matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix2& operator-=(float Rhs) noexcept;

    /**
     * Divides a matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A matrix containing the result of the division
     */
    FORCEINLINE CMatrix2 operator/(float Rhs) const noexcept;

    /**
     * Divides this matrix component-wise with a scalar
     *
     * @param Rhs: The scalar
     * @return A reference to this matrix
     */
    FORCEINLINE CMatrix2& operator/=(float Rhs) noexcept;

    /**
     * Returns the result after comparing this and another matrix
     *
     * @param Other: The matrix to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==(const CMatrix2& Other) const noexcept;

    /**
     * Returns the negated result after comparing this and another matrix
     *
     * @param Other: The matrix to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=(const CMatrix2& Other) const noexcept;

public:

    /**
     * Creates and returns a identity matrix
     *
     * @return A identity matrix
     */
    inline static CMatrix2 Identity() noexcept;

    /**
     * Creates and returns a uniform scale matrix
     *
     * @param Scale: Uniform scale that represents this matrix
     * @return A scale matrix
     */
    inline static CMatrix2 Scale(float Scale) noexcept;

    /**
     * Creates and returns a scale matrix for each axis
     *
     * @param x: Scale for the x-axis
     * @param y: Scale for the y-axis
     * @return A scale matrix
     */
    inline static CMatrix2 Scale(float x, float y) noexcept;

    /**
     * Creates and returns a scale matrix for each axis
     *
     * @param VectorWithScale: A vector containing the scale for each axis in the x-, and y-components
     * @return A scale matrix
     */
    inline static CMatrix2 Scale(const CVector2& VectorWithScale) noexcept;

    /**
     * Creates and returns a rotation matrix around the x-axis
     *
     * @param Rotation: Rotation around in radians
     * @return A rotation matrix
     */
    inline static CMatrix2 Rotation(float Rotation) noexcept;

public:
    union
    {
        /* Each element of the matrix */
        struct
        {
            float m00, m01;
            float m10, m11;
        };

        /* 2-D array of the matrix */
        float f[2][2];
    };
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Implementation

FORCEINLINE CMatrix2::CMatrix2() noexcept
    : m00(0.0f), m01(0.0f)
    , m10(0.0f), m11(0.0f)
{
}

FORCEINLINE CMatrix2::CMatrix2(float Diagonal) noexcept
    : m00(Diagonal), m01(0.0f)
    , m10(0.0f), m11(Diagonal)
{
}

FORCEINLINE CMatrix2::CMatrix2(const CVector2& Row0, const CVector2& Row1) noexcept
    : m00(Row0.x), m01(Row0.y)
    , m10(Row1.x), m11(Row1.y)
{
}

FORCEINLINE CMatrix2::CMatrix2(float In00, float In01, float In10, float In11) noexcept
    : m00(In00), m01(In01)
    , m10(In10), m11(In11)
{
}

FORCEINLINE CMatrix2::CMatrix2(const float* Arr) noexcept
    : m00(Arr[0]), m01(Arr[1])
    , m10(Arr[2]), m11(Arr[3])
{
}

FORCEINLINE CMatrix2 CMatrix2::Transpose() const noexcept
{
#if defined(DISABLE_SIMD)

    CMatrix2 Transpose;
    Transpose.f[0][0] = f[0][0];
    Transpose.f[0][1] = f[1][0];

    Transpose.f[1][0] = f[0][1];
    Transpose.f[1][1] = f[1][1];
    return Transpose;

#else

    CMatrix2 Transpose;

    NSIMD::Float128 This = NSIMD::LoadAligned(this);
    This = NSIMD::Shuffle<0, 2, 1, 3>(This);

    NSIMD::StoreAligned(This, &Transpose);
    return Transpose;

#endif
}

inline CMatrix2 CMatrix2::Invert() const noexcept
{
    const float fDeterminant = (m00 * m11) - (m01 * m10);

#if defined(DISABLE_SIMD)

    const float RecipDeterminant = 1.0f / fDeterminant;

    CMatrix2 Inverse;
    Inverse.m00 = m11 * RecipDeterminant;
    Inverse.m10 = -m10 * RecipDeterminant;
    Inverse.m01 = -m01 * RecipDeterminant;
    Inverse.m11 = m00 * RecipDeterminant;
    return Inverse;

#else

    CMatrix2 Inverse;

    NSIMD::Float128 This = NSIMD::LoadAligned(this);
    This = NSIMD::Shuffle<3, 2, 1, 0>(This);

    constexpr int Keep = 0;
    constexpr int Negate = 1 << 31;

    NSIMD::Int128 Mask = NSIMD::Load(Keep, Negate, Negate, Keep);
    This = NSIMD::Or(This, NSIMD::CastIntToFloat(Mask));

    NSIMD::Float128 RcpDeterminant = NSIMD::Recip(NSIMD::Load(fDeterminant));
    This = NSIMD::Mul(This, RcpDeterminant);

    NSIMD::StoreAligned(This, &Inverse);
    return Inverse;

#endif
}

FORCEINLINE CMatrix2 CMatrix2::Adjoint() const noexcept
{
#if defined(DISABLE_SIMD)

    CMatrix2 Adjugate;
    Adjugate.m00 = m11;
    Adjugate.m10 = -m10;
    Adjugate.m01 = -m01;
    Adjugate.m11 = m00;
    return Adjugate;

#else

    CMatrix2 Adjugate;

    NSIMD::Float128 This = NSIMD::LoadAligned(this);
    This = NSIMD::Shuffle<3, 2, 1, 0>(This);

    constexpr int Keep = 0;
    constexpr int Negate = 1 << 31;

    NSIMD::Int128 Mask = NSIMD::Load(Keep, Negate, Negate, Keep);
    This = NSIMD::Or(This, NSIMD::CastIntToFloat(Mask));

    NSIMD::StoreAligned(This, &Adjugate);
    return Adjugate;

#endif
}

FORCEINLINE float CMatrix2::Determinant() const noexcept
{
    return (m00 * m11) - (m01 * m10);
}

inline bool CMatrix2::HasNan() const noexcept
{
    for (int i = 0; i < 4; i++)
    {
        if (NMath::IsNan(reinterpret_cast<const float*>(this)[i]))
        {
            return true;
        }
    }

    return false;
}

inline bool CMatrix2::HasInfinity() const noexcept
{
    for (int i = 0; i < 4; i++)
    {
        if (NMath::IsInf(reinterpret_cast<const float*>(this)[i]))
        {
            return true;
        }
    }

    return false;
}

FORCEINLINE bool CMatrix2::IsValid() const noexcept
{
    return !HasNan() && !HasInfinity();
}

inline bool CMatrix2::IsEqual(const CMatrix2& Other, float Epsilon) const noexcept
{
#if defined(DISABLE_SIMD)

    Epsilon = NMath::Abs(Epsilon);

    for (int i = 0; i < 4; i++)
    {
        float Diff = reinterpret_cast<const float*>(this)[i] - reinterpret_cast<const float*>(&Other)[i];
        if (NMath::Abs(Diff) > Epsilon)
        {
            return false;
        }
    }

    return true;

#else

    NSIMD::Float128 Espilon128 = NSIMD::Load(Epsilon);
    Espilon128 = NSIMD::Abs(Espilon128);

    NSIMD::Float128 Diff = NSIMD::Sub(this, &Other);
    Diff = NSIMD::Abs(Diff);
    return NSIMD::LessThan(Diff, Espilon128);

#endif
}

FORCEINLINE void CMatrix2::SetIdentity() noexcept
{
#if defined(DISABLE_SIMD)

    m00 = 1.0f;
    m01 = 0.0f;

    m10 = 0.0f;
    m11 = 1.0f;

#else

    NSIMD::Float128 Constant = NSIMD::Load(1.0f, 0.0f, 0.0f, 1.0f);
    NSIMD::StoreAligned(Constant, this);

#endif
}

FORCEINLINE CVector2 CMatrix2::GetRow(int Row) const noexcept
{
    Assert(Row < 2);
    return CVector2(f[Row]);
}

FORCEINLINE CVector2 CMatrix2::GetColumn(int Column) const noexcept
{
    Assert(Column < 2);
    return CVector2(f[0][Column], f[1][Column]);
}

FORCEINLINE float* CMatrix2::GetData() noexcept
{
    return reinterpret_cast<float*>(this);
}

FORCEINLINE const float* CMatrix2::GetData() const noexcept
{
    return reinterpret_cast<const float*>(this);
}

FORCEINLINE bool CMatrix2::operator==(const CMatrix2& Other) const noexcept
{
    return IsEqual(Other);
}

FORCEINLINE bool CMatrix2::operator!=(const CMatrix2& Other) const noexcept
{
    return !IsEqual(Other);
}

FORCEINLINE CVector2 CMatrix2::operator*(const CVector2& Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    CVector2 Result;
    Result.x = (Rhs[0] * m00) + (Rhs[1] * m10);
    Result.y = (Rhs[0] * m01) + (Rhs[1] * m11);
    return Result;

#else

    CVector2 Result;

    NSIMD::Float128 X128 = NSIMD::LoadSingle(Rhs.x);
    NSIMD::Float128 Y128 = NSIMD::LoadSingle(Rhs.y);
    NSIMD::Float128 Temp0 = NSIMD::Shuffle0011<0, 0, 0, 0>(X128, Y128);
    NSIMD::Float128 Temp1 = NSIMD::Mul(this, Temp0);
    Temp0 = NSIMD::Shuffle<2, 3, 2, 3>(Temp1);
    Temp1 = NSIMD::Add(Temp0, Temp1);

    Result.x = NSIMD::GetX(Temp1);
    Result.y = NSIMD::GetY(Temp1);
    return Result;

#endif
}

FORCEINLINE CMatrix2 CMatrix2::operator*(const CMatrix2& Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    CMatrix2 Result;
    Result.m00 = (m00 * Rhs.m00) + (m01 * Rhs.m10);
    Result.m01 = (m01 * Rhs.m11) + (m00 * Rhs.m01);

    Result.m10 = (m10 * Rhs.m00) + (m11 * Rhs.m10);
    Result.m11 = (m11 * Rhs.m11) + (m10 * Rhs.m01);
    return Result;

#else

    CMatrix2 Result;

    NSIMD::Float128 This = NSIMD::LoadAligned(this);
    NSIMD::Float128 Temp = NSIMD::LoadAligned(&Rhs);
    Temp = NSIMD::Mat2Mul(This, Temp);

    NSIMD::StoreAligned(Temp, &Result);
    return Result;

#endif
}

FORCEINLINE CMatrix2& CMatrix2::operator*=(const CMatrix2& Rhs) noexcept
{
    return *this = *this * Rhs;
}

FORCEINLINE CMatrix2 CMatrix2::operator*(float Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    CMatrix2 Result;
    Result.m00 = m00 * Rhs;
    Result.m01 = m01 * Rhs;

    Result.m10 = m10 * Rhs;
    Result.m11 = m11 * Rhs;
    return Result;

#else

    CMatrix2 Result;

    NSIMD::Float128 Temp = NSIMD::Load(Rhs);
    Temp = NSIMD::Mul(this, Temp);

    NSIMD::StoreAligned(Temp, &Result);
    return Result;

#endif
}

FORCEINLINE CMatrix2& CMatrix2::operator*=(float Rhs) noexcept
{
    return *this = *this * Rhs;
}

FORCEINLINE CMatrix2 CMatrix2::operator+(const CMatrix2& Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    CMatrix2 Result;
    Result.m00 = m00 + Rhs.m00;
    Result.m01 = m01 + Rhs.m01;

    Result.m10 = m10 + Rhs.m10;
    Result.m11 = m11 + Rhs.m11;
    return Result;

#else

    CMatrix2 Result;
    NSIMD::Float128 Temp = NSIMD::Add(this, &Rhs);
    NSIMD::StoreAligned(Temp, &Result);
    return Result;

#endif
}

FORCEINLINE CMatrix2& CMatrix2::operator+=(const CMatrix2& Rhs) noexcept
{
    return *this = *this + Rhs;
}

FORCEINLINE CMatrix2 CMatrix2::operator+(float Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    CMatrix2 Result;
    Result.m00 = m00 + Rhs;
    Result.m01 = m01 + Rhs;

    Result.m10 = m10 + Rhs;
    Result.m11 = m11 + Rhs;
    return Result;

#else

    CMatrix2 Result;

    NSIMD::Float128 Temp = NSIMD::Load(Rhs);
    Temp = NSIMD::Add(this, Temp);

    NSIMD::StoreAligned(Temp, &Result);
    return Result;

#endif
}

FORCEINLINE CMatrix2& CMatrix2::operator+=(float Rhs) noexcept
{
    return *this = *this + Rhs;
}

FORCEINLINE CMatrix2 CMatrix2::operator-(const CMatrix2& Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    CMatrix2 Result;
    Result.m00 = m00 - Rhs.m00;
    Result.m01 = m01 - Rhs.m01;

    Result.m10 = m10 - Rhs.m10;
    Result.m11 = m11 - Rhs.m11;
    return Result;

#else

    CMatrix2 Result;
    NSIMD::Float128 Temp = NSIMD::Sub(this, &Rhs);
    NSIMD::StoreAligned(Temp, &Result);
    return Result;

#endif
}

FORCEINLINE CMatrix2& CMatrix2::operator-=(const CMatrix2& Rhs) noexcept
{
    return *this = *this - Rhs;
}

FORCEINLINE CMatrix2 CMatrix2::operator-(float Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    CMatrix2 Result;
    Result.m00 = m00 - Rhs;
    Result.m01 = m01 - Rhs;

    Result.m10 = m10 - Rhs;
    Result.m11 = m11 - Rhs;
    return Result;

#else

    CMatrix2 Result;

    NSIMD::Float128 Temp = NSIMD::Load(Rhs);
    Temp = NSIMD::Sub(this, Temp);

    NSIMD::StoreAligned(Temp, &Result);
    return Result;

#endif
}

FORCEINLINE CMatrix2& CMatrix2::operator-=(float Rhs) noexcept
{
    return *this = *this - Rhs;
}

FORCEINLINE CMatrix2 CMatrix2::operator/(float Rhs) const noexcept
{
#if defined(DISABLE_SIMD)

    float Recip = 1.0f / Rhs;

    CMatrix2 Result;
    Result.m00 = m00 * Recip;
    Result.m01 = m01 * Recip;

    Result.m10 = m10 * Recip;
    Result.m11 = m11 * Recip;
    return Result;

#else

    CMatrix2 Result;

    NSIMD::Float128 Temp = NSIMD::Load(Rhs);
    Temp = NSIMD::Div(this, Temp);

    NSIMD::StoreAligned(Temp, &Result);
    return Result;

#endif
}

FORCEINLINE CMatrix2& CMatrix2::operator/=(float Rhs) noexcept
{
    return *this = *this / Rhs;
}

inline CMatrix2 CMatrix2::Identity() noexcept
{
    return CMatrix2(1.0f);
}

inline CMatrix2 CMatrix2::Scale(float Scale) noexcept
{
    return CMatrix2(Scale);
}

inline CMatrix2 CMatrix2::Scale(float x, float y) noexcept
{
    return CMatrix2(x, 0.0f, 0.0f, y);
}

inline CMatrix2 CMatrix2::Scale(const CVector2& VectorWithScale) noexcept
{
    return Scale(VectorWithScale.x, VectorWithScale.y);
}

inline CMatrix2 CMatrix2::Rotation(float Rotation) noexcept
{
    const float SinZ = sinf(Rotation);
    const float CosZ = cosf(Rotation);

    return CMatrix2(CosZ, SinZ, -SinZ, CosZ);
}
