#pragma once
#include "Vector3.h"
#include "VectorOp.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// A 4-D floating point vector (x, y, z, w) with SIMD capabilities

class VECTOR_ALIGN CVector4
{
public:

    /** Default constructor (Initialize components to zero) */
    FORCEINLINE CVector4() noexcept;

    /**
     * @brief: Constructor initializing all components with a corresponding value.
     *
     * @param InX: The x-coordinate
     * @param InY: The y-coordinate
     * @param InZ: The z-coordinate
     * @param InW: The w-coordinate
     */
    FORCEINLINE explicit CVector4(float InX, float InY, float InZ, float InW) noexcept;

    /**
     * @brief: Constructor initializing all components with an array.
     *
     * @param Arr: Array with 4 elements
     */
    FORCEINLINE explicit CVector4(const float* Arr) noexcept;

    /**
     * @brief: Constructor initializing all components with a single value.
     *
     * @param Scalar: Value to set all components to
     */
    FORCEINLINE explicit CVector4(float Scalar) noexcept;

    /**
     * @brief: Constructor copying a 3-D vector (x, y, z) into the first components, setting w-component to zero
     *
     * @param XYZ: Value to set first components to
     */
    FORCEINLINE CVector4(const CVector3& XYZ) noexcept;

    /**
     * @brief: Constructor copying a 3-D vector (x, y, z) into the first components, setting w-component to a specific value
     *
     * @param XYZ: Value to set first components to
     * @param InW: Value to set the w-component to
     */
    FORCEINLINE explicit CVector4(const CVector3& XYZ, float InW) noexcept;

     /** @brief: Normalized this vector */
    inline void Normalize() noexcept;

    /**
     * @brief: Returns a normalized version of this vector
     *
     * @return: A copy of this vector normalized
     */
    inline CVector4 GetNormalized() const noexcept;

    /**
     * @brief: Compares, within a threshold Epsilon, this vector with another vector
     *
     * @param Other: vector to compare against
     * @return: True if equal, false if not
     */
    inline bool IsEqual(const CVector4& Other, float Epsilon = NMath::kIsEqualEpsilon) const noexcept;

    /**
     * @brief: Checks weather this vector is a unit vector not
     *
     * @return: True if the length equals one, false if not
     */
    FORCEINLINE bool IsUnitVector() const noexcept;

    /**
     * @brief: Checks weather this vector has any component that equals NaN
     *
     * @return: True if the any component equals NaN, false if not
     */
    FORCEINLINE bool HasNaN() const noexcept;

    /**
     * @brief: Checks weather this vector has any component that equals infinity
     *
     * @return: True if the any component equals infinity, false if not
     */
    FORCEINLINE bool HasInfinity() const noexcept;

    /**
     * @brief: Checks weather this vector has any value that equals infinity or NaN
     *
     * @return: False if the any value equals infinity or NaN, true if not
     */
    FORCEINLINE bool IsValid() const noexcept;

    /**
     * @brief: Returns the length of this vector
     *
     * @return: The length of the vector
     */
    FORCEINLINE float Length() const noexcept;

    /**
     * @brief: Returns the length of this vector squared
     *
     * @return: The length of the vector squared
     */
    FORCEINLINE float LengthSquared() const noexcept;

    /**
     * @brief: Returns the dot product between this and another vector
     *
     * @param Other: The vector to perform dot product with
     * @return: The dot product
     */
    FORCEINLINE float DotProduct(const CVector4& Other) const noexcept;

    /**
     * @brief: Returns the cross product of this vector and another vector.
     * This function does not take the w-component into account
     *
     * @param Other: The vector to perform cross product with
     * @return: The cross product
     */
    inline CVector4 CrossProduct(const CVector4& Other) const noexcept;

    /**
     * @brief: Returns the resulting vector after projecting this vector onto another.
     *
     * @param Other: The vector to project onto
     * @return: The projected vector
     */
    inline CVector4 ProjectOn(const CVector4& Other) const noexcept;

    /**
     * @brief: Returns the reflected vector after reflecting this vector around a normal.
     *
     * @param Normal: Vector to reflect around
     * @return: The reflected vector
     */
    inline CVector4 Reflect(const CVector4& Normal) const noexcept;

    /**
     * @brief: Returns the data of this matrix as a pointer
     *
     * @return: A pointer to the data
     */
    FORCEINLINE float* Data() noexcept;

    /**
     * @brief: Returns the data of this matrix as a pointer
     *
     * @return: A pointer to the data
     */
    FORCEINLINE const float* Data() const noexcept;

public:

    /**
     * @brief: Returns a vector with the smallest of each component of two vectors
     *
     * @param First: First vector to compare with
     * @param Second: Second vector to compare with
     * @return: A vector with the smallest components of First and Second
     */
    friend FORCEINLINE CVector4 Min(const CVector4& First, const CVector4& Second) noexcept;

    /**
     * @brief: Returns a vector with the largest of each component of two vectors
     *
     * @param First: First vector to compare with
     * @param Second: Second vector to compare with
     * @return: A vector with the largest components of First and Second
     */
    friend FORCEINLINE CVector4 Max(const CVector4& First, const CVector4& Second) noexcept;

    /**
     * @brief: Returns the linear interpolation between two vectors
     *
     * @param First: First vector to interpolate
     * @param Second: Second vector to interpolate
     * @param Factor: Factor to interpolate with. Zero returns First, One returns seconds
     * @return: A vector with the result of interpolation
     */
    friend FORCEINLINE CVector4 Lerp(const CVector4& First, const CVector4& Second, float t) noexcept;

    /**
     * @brief: Returns a vector with all the components within the range of a min and max value
     *
     * @param Min: Vector with minimum values
     * @param Max: Vector with maximum values
     * @param Value: Vector to clamp
     * @return: A vector with the result of clamping
     */
    friend FORCEINLINE CVector4 Clamp(const CVector4& Min, const CVector4& Max, const CVector4& Value) noexcept;

    /**
     * @brief: Returns a vector with all the components within the range zero and one
     *
     * @param Value: Value to saturate
     * @return: A vector with the result of saturation
     */
    friend FORCEINLINE CVector4 Saturate(const CVector4& Value) noexcept;

public:

    /**
     * @brief: Return a vector with component-wise negation of this vector
     *
     * @return: A negated vector
     */
    FORCEINLINE CVector4 operator-() const noexcept;

    /**
     * @brief: Returns the result of component-wise adding this and another vector
     *
     * @param RHS: The vector to add
     * @return: A vector with the result of addition
     */
    FORCEINLINE CVector4 operator+(const CVector4& RHS) const noexcept;

    /**
     * @brief: Returns this vector after component-wise adding this with another vector
     *
     * @param RHS: The vector to add
     * @return: A reference to this vector
     */
    FORCEINLINE CVector4& operator+=(const CVector4& RHS) noexcept;

    /**
     * @brief: Returns the result of adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return: A vector with the result of addition
     */
    FORCEINLINE CVector4 operator+(float RHS) const noexcept;

    /**
     * @brief: Returns this vector after adding a scalar to each component of this vector
     *
     * @param RHS: The scalar to add
     * @return: A reference to this vector
     */
    FORCEINLINE CVector4& operator+=(float RHS) noexcept;

    /**
     * @brief: Returns the result of component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return: A vector with the result of subtraction
     */
    FORCEINLINE CVector4 operator-(const CVector4& RHS) const noexcept;

    /**
     * @brief: Returns this vector after component-wise subtraction between this and another vector
     *
     * @param RHS: The vector to subtract
     * @return: A reference to this vector
     */
    FORCEINLINE CVector4& operator-=(const CVector4& RHS) noexcept;

    /**
     * @brief: Returns the result of subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return: A vector with the result of the subtraction
     */
    FORCEINLINE CVector4 operator-(float RHS) const noexcept;

    /**
     * @brief: Returns this vector after subtracting each component of this vector with a scalar
     *
     * @param RHS: The scalar to subtract
     * @return: A reference to this vector
     */
    FORCEINLINE CVector4& operator-=(float RHS) noexcept;

    /**
     * @brief: Returns the result of component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return: A vector with the result of the multiplication
     */
    FORCEINLINE CVector4 operator*(const CVector4& RHS) const noexcept;

    /**
     * @brief: Returns this vector after component-wise multiplication with this and another vector
     *
     * @param RHS: The vector to multiply with
     * @return: A reference to this vector
     */
    FORCEINLINE CVector4& operator*=(const CVector4& RHS) noexcept;

    /**
     * @brief: Returns the result of multiplying each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return: A vector with the result of the multiplication
     */
    FORCEINLINE CVector4 operator*(float RHS) const noexcept;

    /**
     * @brief: Returns the result of multiplying each component of a vector with a scalar
     *
     * @param Lhs: The scalar to multiply with
     * @param RHS: The vector to multiply with
     * @return: A vector with the result of the multiplication
     */
    friend FORCEINLINE CVector4 operator*(float Lhs, const CVector4& RHS) noexcept;

    /**
     * @brief: Returns this vector after multiplying each component of this vector with a scalar
     *
     * @param RHS: The scalar to multiply with
     * @return: A reference to this vector
     */
    FORCEINLINE CVector4 operator*=(float RHS) noexcept;

    /**
     * @brief: Returns the result of component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return: A vector with the result of the division
     */
    FORCEINLINE CVector4 operator/(const CVector4& RHS) const noexcept;

    /**
     * @brief: Returns this vector after component-wise division with this and another vector
     *
     * @param RHS: The vector to divide with
     * @return: A reference to this vector
     */
    FORCEINLINE CVector4& operator/=(const CVector4& RHS) noexcept;

    /**
     * @brief: Returns the result of dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return: A vector with the result of the division
     */
    FORCEINLINE CVector4 operator/(float RHS) const noexcept;

    /**
     * @brief: Returns this vector after dividing each component of this vector and a scalar
     *
     * @param RHS: The scalar to divide with
     * @return: A reference to this vector
     */
    FORCEINLINE CVector4& operator/=(float RHS) noexcept;

    /**
     * @brief: Returns the result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return: True if equal, false if not
     */
    FORCEINLINE bool operator==(const CVector4& Other) const noexcept;

    /**
     * @brief: Returns the negated result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return: False if equal, true if not
     */
    FORCEINLINE bool operator!=(const CVector4& Other) const noexcept;

    /**
     * @brief: Returns the component specified
     *
     * @param Index: The component index
     * @return: The component
     */
    FORCEINLINE float& operator[](int Index) noexcept;

    /**
     * @brief: Returns the component specified
     *
     * @param Index: The component index
     * @return: The component
     */
    FORCEINLINE float operator[](int Index) const noexcept;

public:

     /** @brief: The x-coordinate */
    float x;
     /** @brief: The y-coordinate */
    float y;
     /** @brief: The z-coordinate */
    float z;
     /** @brief: The w-coordinate */
    float w;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Implementation

FORCEINLINE CVector4::CVector4() noexcept
    : x(0.0f)
    , y(0.0f)
    , z(0.0f)
    , w(0.0f)
{
}

FORCEINLINE CVector4::CVector4(float InX, float InY, float InZ, float InW) noexcept
    : x(InX)
    , y(InY)
    , z(InZ)
    , w(InW)
{
}

FORCEINLINE CVector4::CVector4(const float* Arr) noexcept
    : x(Arr[0])
    , y(Arr[1])
    , z(Arr[2])
    , w(Arr[3])
{
}

FORCEINLINE CVector4::CVector4(float Scalar) noexcept
    : x(Scalar)
    , y(Scalar)
    , z(Scalar)
    , w(Scalar)
{
}

FORCEINLINE CVector4::CVector4(const CVector3& XYZ) noexcept
    : x(XYZ.x)
    , y(XYZ.y)
    , z(XYZ.z)
    , w(0.0f)
{
}

FORCEINLINE CVector4::CVector4(const CVector3& XYZ, float InW) noexcept
    : x(XYZ.x)
    , y(XYZ.y)
    , z(XYZ.z)
    , w(InW)
{
}

inline void CVector4::Normalize() noexcept
{
#if defined(DISABLE_SIMD)

    float fLengthSquared = LengthSquared();
    if (fLengthSquared != 0.0f)
    {
        float fRecipLength = 1.0f / NMath::Sqrt(fLengthSquared);
        x = x * fRecipLength;
        y = y * fRecipLength;
        z = z * fRecipLength;
        w = w * fRecipLength;
    }

#else

    NVectorOp::Float128 Reg0 = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Reg1 = NVectorOp::Dot(Reg0, Reg0);

    float fLengthSquared = NVectorOp::GetX(Reg1);
    if (fLengthSquared != 0.0f)
    {
        Reg1 = NVectorOp::Shuffle<0, 1, 0, 1>(Reg1);
        Reg1 = NVectorOp::RecipSqrt(Reg1);
        Reg0 = NVectorOp::Mul(Reg0, Reg1);
        NVectorOp::StoreAligned(Reg0, this);
    }

#endif
}

NOINLINE CVector4 CVector4::GetNormalized() const noexcept
{
    CVector4 Result(*this);
    Result.Normalize();
    return Result;
}

FORCEINLINE bool CVector4::IsEqual(const CVector4& Other, float Epsilon) const noexcept
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

    NVectorOp::Float128 Espilon128 = NVectorOp::Load(Epsilon);
    Espilon128 = NVectorOp::Abs(Espilon128);

    NVectorOp::Float128 Diff = NVectorOp::Sub(this, &Other);
    Diff = NVectorOp::Abs(Diff);

    return NVectorOp::LessThan(Diff, Espilon128);

#endif
}

NOINLINE bool CVector4::IsUnitVector() const noexcept
{
    // LengthSquared should be the same as length if this is a unit vector
    // However, this way the sqrt can be avoided
    float fLengthSquared = NMath::Abs(1.0f - LengthSquared());
    return (fLengthSquared < NMath::kIsEqualEpsilon);
}

FORCEINLINE bool CVector4::HasNaN() const noexcept
{
    for (int i = 0; i < 4; i++)
    {
        if (NMath::IsNaN(reinterpret_cast<const float*>(this)[i]))
        {
            return true;
        }
    }

    return false;
}

FORCEINLINE bool CVector4::HasInfinity() const noexcept
{
    for (int i = 0; i < 4; i++)
    {
        if (NMath::IsInfinity(reinterpret_cast<const float*>(this)[i]))
        {
            return true;
        }
    }

    return false;
}

FORCEINLINE bool CVector4::IsValid() const noexcept
{
    return !HasNaN() && !HasInfinity();
}

NOINLINE float CVector4::Length() const noexcept
{
    float fLengthSquared = LengthSquared();
    return NMath::Sqrt(fLengthSquared);
}

FORCEINLINE float CVector4::LengthSquared() const noexcept
{
    return DotProduct(*this);
}

FORCEINLINE float CVector4::DotProduct(const CVector4& Other) const noexcept
{
#if defined(DISABLE_SIMD)

    return (x * Other.x) + (y * Other.y) + (z * Other.z) + (w * Other.w);

#else

    NVectorOp::Float128 Reg0 = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Reg1 = NVectorOp::LoadAligned(&Other);
    NVectorOp::Float128 Dot = NVectorOp::Dot(Reg0, Reg1);
    return NVectorOp::GetX(Dot);

#endif
}

FORCEINLINE CVector4 CVector4::CrossProduct(const CVector4& Other) const noexcept
{
#if defined(DISABLE_SIMD)

    float NewX = (y * Other.z) - (z * Other.y);
    float NewY = (z * Other.x) - (x * Other.z);
    float NewZ = (x * Other.y) - (y * Other.x);
    return CVector4(NewX, NewY, NewZ, 0.0f);

#else

    CVector4 Cross;

    NVectorOp::Float128 Reg0 = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Reg1 = NVectorOp::LoadAligned(&Other);
    NVectorOp::Float128 Result = NVectorOp::Cross(Reg0, Reg1);
    NVectorOp::Int128   Mask = NVectorOp::Load(~0, ~0, ~0, 0);
    Result = NVectorOp::And(Result, NVectorOp::CastIntToFloat(Mask));

    NVectorOp::StoreAligned(Result, &Cross);
    return Cross;

#endif
}

inline CVector4 CVector4::ProjectOn(const CVector4& Other) const noexcept
{
#if defined(DISABLE_SIMD)

    float AdotB = this->DotProduct(Other);
    float BdotB = Other.LengthSquared();
    return (AdotB / BdotB) * Other;

#else

    CVector4 Projected;

    NVectorOp::Float128 Reg0 = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Reg1 = NVectorOp::LoadAligned(&Other);

    NVectorOp::Float128 AdotB = NVectorOp::Dot(Reg0, Reg1);
    AdotB = NVectorOp::Shuffle<0, 1, 0, 1>(AdotB);

    NVectorOp::Float128 BdotB = NVectorOp::Dot(Reg1, Reg1);
    BdotB = NVectorOp::Shuffle<0, 1, 0, 1>(BdotB);
    BdotB = NVectorOp::Div(AdotB, BdotB);
    BdotB = NVectorOp::Mul(BdotB, Reg1);

    NVectorOp::StoreAligned(BdotB, &Projected);
    return Projected;

#endif
}

inline CVector4 CVector4::Reflect(const CVector4& Normal) const noexcept
{
#if defined(DISABLE_SIMD)

    float VdotN = this->DotProduct(Normal);
    float NdotN = Normal.LengthSquared();
    return *this - ((2.0f * (VdotN / NdotN)) * Normal);

#else

    CVector4 Reflected;

    NVectorOp::Float128 Reg0 = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Reg1 = NVectorOp::LoadAligned(reinterpret_cast<const float*>(&Normal));

    NVectorOp::Float128 VdotN = NVectorOp::Dot(Reg0, Reg1);
    VdotN = NVectorOp::Shuffle<0, 1, 0, 1>(VdotN);

    NVectorOp::Float128 NdotN = NVectorOp::Dot(Reg1, Reg1);
    NdotN = NVectorOp::Shuffle<0, 1, 0, 1>(NdotN);

    NVectorOp::Float128 Reg2 = NVectorOp::Load(2.0f);
    VdotN = NVectorOp::Div(VdotN, NdotN);
    VdotN = NVectorOp::Mul(Reg2, VdotN);
    Reg1 = NVectorOp::Mul(VdotN, Reg1);
    Reg0 = NVectorOp::Sub(Reg0, Reg1);

    NVectorOp::StoreAligned(Reg0, &Reflected);
    return Reflected;

#endif
}

FORCEINLINE float* CVector4::Data() noexcept
{
    return reinterpret_cast<float*>(this);
}

FORCEINLINE const float* CVector4::Data() const noexcept
{
    return reinterpret_cast<const float*>(this);
}

FORCEINLINE CVector4 Min(const CVector4& Lhs, const CVector4& RHS) noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(NMath::Min(Lhs.x, RHS.x), NMath::Min(Lhs.y, RHS.y), NMath::Min(Lhs.z, RHS.z), NMath::Min(Lhs.w, RHS.w));

#else

    CVector4 MinVector;

    NVectorOp::Float128 Reg0 = NVectorOp::LoadAligned(reinterpret_cast<const float*>(&Lhs));
    NVectorOp::Float128 Reg1 = NVectorOp::LoadAligned(&RHS);
    Reg0 = NVectorOp::Min(Reg0, Reg1);

    NVectorOp::StoreAligned(Reg0, &MinVector);
    return MinVector;

#endif
}

FORCEINLINE CVector4 Max(const CVector4& Lhs, const CVector4& RHS) noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(NMath::Max(Lhs.x, RHS.x), NMath::Max(Lhs.y, RHS.y), NMath::Max(Lhs.z, RHS.z), NMath::Max(Lhs.w, RHS.w));

#else

    CVector4 MaxVector;

    NVectorOp::Float128 Reg0 = NVectorOp::LoadAligned(reinterpret_cast<const float*>(&Lhs));
    NVectorOp::Float128 Reg1 = NVectorOp::LoadAligned(&RHS);
    Reg0 = NVectorOp::Max(Reg0, Reg1);

    NVectorOp::StoreAligned(Reg0, &MaxVector);
    return MaxVector;

#endif
}

FORCEINLINE CVector4 Lerp(const CVector4& First, const CVector4& Second, float t) noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(
        (1.0f - t) * First.x + t * Second.x,
        (1.0f - t) * First.y + t * Second.y,
        (1.0f - t) * First.z + t * Second.z,
        (1.0f - t) * First.w + t * Second.w);

#else

    CVector4 Lerped;

    NVectorOp::Float128 Reg0 = NVectorOp::LoadAligned(reinterpret_cast<const float*>(&First));
    NVectorOp::Float128 Reg1 = NVectorOp::LoadAligned(reinterpret_cast<const float*>(&Second));
    NVectorOp::Float128 Reg3 = NVectorOp::Load(t);
    NVectorOp::Float128 Ones = NVectorOp::MakeOnes();
    NVectorOp::Float128 Reg4 = NVectorOp::Sub(Ones, Reg3);
    Reg0 = NVectorOp::Mul(Reg0, Reg4);
    Reg1 = NVectorOp::Mul(Reg1, Reg3);
    Reg0 = NVectorOp::Add(Reg0, Reg1);

    NVectorOp::StoreAligned(Reg0, &Lerped);
    return Lerped;

#endif
}

FORCEINLINE CVector4 Clamp(const CVector4& Min, const CVector4& Max, const CVector4& Value) noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(
        NMath::Min(NMath::Max(Value.x, Min.x), Max.x),
        NMath::Min(NMath::Max(Value.y, Min.y), Max.y),
        NMath::Min(NMath::Max(Value.z, Min.z), Max.z),
        NMath::Min(NMath::Max(Value.w, Min.w), Max.w));

#else

    CVector4 Clamped;

    NVectorOp::Float128 Min128 = NVectorOp::LoadAligned(reinterpret_cast<const float*>(&Min));
    NVectorOp::Float128 Max128 = NVectorOp::LoadAligned(reinterpret_cast<const float*>(&Max));
    NVectorOp::Float128 Value128 = NVectorOp::LoadAligned(&Value);
    Value128 = NVectorOp::Max(Value128, Min128);
    Value128 = NVectorOp::Min(Value128, Max128);

    NVectorOp::StoreAligned(Value128, &Clamped);
    return Clamped;

#endif
}

FORCEINLINE CVector4 Saturate(const CVector4& Value) noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(
        NMath::Min(NMath::Max(Value.x, 0.0f), 1.0f),
        NMath::Min(NMath::Max(Value.y, 0.0f), 1.0f),
        NMath::Min(NMath::Max(Value.z, 0.0f), 1.0f),
        NMath::Min(NMath::Max(Value.w, 0.0f), 1.0f));

#else

    CVector4 Saturated;

    NVectorOp::Float128 Zeros = NVectorOp::MakeZeros();
    NVectorOp::Float128 Ones = NVectorOp::MakeOnes();
    NVectorOp::Float128 VectorValue = NVectorOp::LoadAligned(&Value);
    VectorValue = NVectorOp::Max(VectorValue, Zeros);
    VectorValue = NVectorOp::Min(VectorValue, Ones);

    NVectorOp::StoreAligned(VectorValue, &Saturated);
    return Saturated;

#endif
}

FORCEINLINE CVector4 CVector4::operator-() const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(-x, -y, -z, -w);

#else

    CVector4 Negated;

    NVectorOp::Float128 Zeros = NVectorOp::MakeZeros();
    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    This = NVectorOp::Sub(Zeros, This);

    NVectorOp::StoreAligned(This, &Negated);
    return Negated;

#endif
}

FORCEINLINE CVector4 CVector4::operator+(const CVector4& RHS) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(x + RHS.x, y + RHS.y, z + RHS.z, w + RHS.w);

#else

    CVector4 Result;

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
    This = NVectorOp::Add(This, Other);

    NVectorOp::StoreAligned(This, &Result);
    return Result;

#endif
}

FORCEINLINE CVector4& CVector4::operator+=(const CVector4& RHS) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this + RHS;

#else

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
    This = NVectorOp::Add(This, Other);

    NVectorOp::StoreAligned(This, this);
    return *this;

#endif
}

FORCEINLINE CVector4 CVector4::operator+(float RHS) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(x + RHS, y + RHS, z + RHS, w + RHS);

#else

    CVector4 Result;

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
    This = NVectorOp::Add(This, Scalars);

    NVectorOp::StoreAligned(This, &Result);
    return Result;

#endif
}

FORCEINLINE CVector4& CVector4::operator+=(float RHS) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this + RHS;

#else

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
    This = NVectorOp::Add(This, Scalars);

    NVectorOp::StoreAligned(This, this);
    return *this;

#endif
}

FORCEINLINE CVector4 CVector4::operator-(const CVector4& RHS) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(x - RHS.x, y - RHS.y, z - RHS.z, w - RHS.w);

#else

    CVector4 Result;

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
    This = NVectorOp::Sub(This, Other);

    NVectorOp::StoreAligned(This, &Result);
    return Result;

#endif
}

FORCEINLINE CVector4& CVector4::operator-=(const CVector4& RHS) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this - RHS;

#else

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
    This = NVectorOp::Sub(This, Other);

    NVectorOp::StoreAligned(This, this);
    return *this;

#endif
}

FORCEINLINE CVector4 CVector4::operator-(float RHS) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(x - RHS, y - RHS, z - RHS, w - RHS);

#else

    CVector4 Result;

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
    This = NVectorOp::Sub(This, Scalars);

    NVectorOp::StoreAligned(This, &Result);
    return Result;

#endif
}

FORCEINLINE CVector4& CVector4::operator-=(float RHS) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this - RHS;

#else

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
    This = NVectorOp::Sub(This, Scalars);

    NVectorOp::StoreAligned(This, this);
    return *this;

#endif
}

FORCEINLINE CVector4 CVector4::operator*(const CVector4& RHS) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(x * RHS.x, y * RHS.y, z * RHS.z, w * RHS.w);

#else

    CVector4 Result;

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
    This = NVectorOp::Mul(This, Other);

    NVectorOp::StoreAligned(This, &Result);
    return Result;

#endif
}

FORCEINLINE CVector4& CVector4::operator*=(const CVector4& RHS) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this * RHS;

#else

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
    This = NVectorOp::Mul(This, Other);

    NVectorOp::StoreAligned(This, this);
    return *this;

#endif
}

FORCEINLINE CVector4 CVector4::operator*(float RHS) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(x * RHS, y * RHS, z * RHS, w * RHS);

#else

    CVector4 Result;

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
    This = NVectorOp::Mul(This, Scalars);

    NVectorOp::StoreAligned(This, &Result);
    return Result;

#endif
}

FORCEINLINE CVector4 operator*(float Lhs, const CVector4& RHS) noexcept
{
    return RHS * Lhs;
}

FORCEINLINE CVector4 CVector4::operator*=(float RHS) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this * RHS;

#else

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
    This = NVectorOp::Mul(This, Scalars);

    NVectorOp::StoreAligned(This, this);
    return *this;

#endif
}

FORCEINLINE CVector4 CVector4::operator/(const CVector4& RHS) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(x / RHS.x, y / RHS.y, z / RHS.z, w / RHS.w);

#else

    CVector4 Result;

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
    This = NVectorOp::Div(This, Other);

    NVectorOp::StoreAligned(This, &Result);
    return Result;

#endif
}

FORCEINLINE CVector4& CVector4::operator/=(const CVector4& RHS) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this / RHS;

#else

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Other = NVectorOp::LoadAligned(&RHS);
    This = NVectorOp::Div(This, Other);

    NVectorOp::StoreAligned(This, this);
    return *this;

#endif
}

FORCEINLINE CVector4 CVector4::operator/(float RHS) const noexcept
{
#if defined(DISABLE_SIMD)

    return CVector4(x / RHS, y / RHS, z / RHS, w / RHS);

#else

    CVector4 Result;

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
    This = NVectorOp::Div(This, Scalars);

    NVectorOp::StoreAligned(This, &Result);
    return Result;

#endif
}

FORCEINLINE CVector4& CVector4::operator/=(float RHS) noexcept
{
#if defined(DISABLE_SIMD)

    return *this = *this / RHS;

#else

    NVectorOp::Float128 This = NVectorOp::LoadAligned(this);
    NVectorOp::Float128 Scalars = NVectorOp::Load(RHS);
    This = NVectorOp::Div(This, Scalars);

    NVectorOp::StoreAligned(This, this);
    return *this;

#endif
}

FORCEINLINE float& CVector4::operator[](int Index) noexcept
{
    Assert(Index < 4);
    return reinterpret_cast<float*>(this)[Index];
}

FORCEINLINE float CVector4::operator[](int Index) const noexcept
{
    Assert(Index < 4);
    return reinterpret_cast<const float*>(this)[Index];
}

FORCEINLINE bool CVector4::operator==(const CVector4& Other) const noexcept
{
    return IsEqual(Other);
}

FORCEINLINE bool CVector4::operator!=(const CVector4& Other) const noexcept
{
    return !IsEqual(Other);
}

namespace NMath
{
    template<>
    FORCEINLINE CVector4 ToDegrees(CVector4 Radians)
    {
        return CVector4(ToDegrees(Radians.x), ToDegrees(Radians.y), ToDegrees(Radians.z), ToDegrees(Radians.w));
    }

    template<>
    FORCEINLINE CVector4 ToRadians(CVector4 Degrees)
    {
        return CVector4(ToRadians(Degrees.x), ToRadians(Degrees.y), ToRadians(Degrees.z), ToRadians(Degrees.w));
    }
}
