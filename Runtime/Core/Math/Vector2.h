#pragma once
#include "MathCommon.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// A 2-D floating point vector (x, y)

class CVector2
{
public:

    /** Default constructor (Initialize components to zero) */
    FORCEINLINE CVector2() noexcept;

    /**
     * @brief: Constructor initializing all components with a corresponding value.
     *
     * @param InX: The x-coordinate
     * @param InY: The y-coordinate
     */
    FORCEINLINE explicit CVector2(float InX, float InY) noexcept;

    /**
     * @brief: Constructor initializing all components with an array.
     *
     * @param Arr: Array with 2 elements
     */
    FORCEINLINE explicit CVector2(const float* Arr) noexcept;

    /**
     * @brief: Constructor initializing all components with a single value.
     *
     * @param Scalar: Value to set all components to
     */
    FORCEINLINE explicit CVector2(float Scalar) noexcept;

     /** @brief: Normalized this vector */
    inline void Normalize() noexcept;

    /**
     * @brief: Returns a normalized version of this vector
     *
     * @return: A copy of this vector normalized
     */
    FORCEINLINE CVector2 GetNormalized() const noexcept;

    /**
     * @brief: Compares, within a threshold Epsilon, this vector with another vector
     *
     * @param Other: vector to compare against
     * @return: True if equal, false if not
     */
    inline bool IsEqual(const CVector2& Other, float Epsilon = NMath::kIsEqualEpsilon) const noexcept;

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
    FORCEINLINE bool HasNan() const noexcept;

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
    FORCEINLINE float DotProduct(const CVector2& Other) const noexcept;

    /**
     * @brief: Returns the resulting vector after projecting this vector onto another.
     *
     * @param Other: The vector to project onto
     * @return: The projected vector
     */
    inline CVector2 ProjectOn(const CVector2& Other) const noexcept;

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
    friend FORCEINLINE CVector2 Min(const CVector2& First, const CVector2& Second) noexcept;

    /**
     * @brief: Returns a vector with the largest of each component of two vectors
     *
     * @param First: First vector to compare with
     * @param Second: Second vector to compare with
     * @return: A vector with the largest components of First and Second
     */
    friend FORCEINLINE CVector2 Max(const CVector2& First, const CVector2& Second) noexcept;

    /**
     * @brief: Returns the linear interpolation between two vectors
     *
     * @param First: First vector to interpolate
     * @param Second: Second vector to interpolate
     * @param Factor: Factor to interpolate with. Zero returns First, One returns seconds
     * @return: A vector with the result of interpolation
     */
    friend FORCEINLINE CVector2 Lerp(const CVector2& First, const CVector2& Second, float t) noexcept;

    /**
     * @brief: Returns a vector with all the components within the range of a min and max value
     *
     * @param Min: Vector with minimum values
     * @param Max: Vector with maximum values
     * @param Value: Vector to clamp
     * @return: A vector with the result of clamping
     */
    friend FORCEINLINE CVector2 Clamp(const CVector2& Min, const CVector2& Max, const CVector2& Value) noexcept;

    /**
     * @brief: Returns a vector with all the components within the range zero and one
     *
     * @param Value: Value to saturate
     * @return: A vector with the result of saturation
     */
    friend FORCEINLINE CVector2 Saturate(const CVector2& Value) noexcept;

public:

    /**
     * @brief: Return a vector with component-wise negation of this vector
     *
     * @return: A negated vector
     */
    FORCEINLINE CVector2 operator-() const noexcept;

    /**
     * @brief: Returns the result of component-wise adding this and another vector
     *
     * @param Rhs: The vector to add
     * @return: A vector with the result of addition
     */
    FORCEINLINE CVector2 operator+(const CVector2& Rhs) const noexcept;

    /**
     * @brief: Returns this vector after component-wise adding this with another vector
     *
     * @param Rhs: The vector to add
     * @return: A reference to this vector
     */
    FORCEINLINE CVector2& operator+=(const CVector2& Rhs) noexcept;

    /**
     * @brief: Returns the result of adding a scalar to each component of this vector
     *
     * @param Rhs: The scalar to add
     * @return: A vector with the result of addition
     */
    FORCEINLINE CVector2 operator+(float Rhs) const noexcept;

    /**
     * @brief: Returns this vector after adding a scalar to each component of this vector
     *
     * @param Rhs: The scalar to add
     * @return: A reference to this vector
     */
    FORCEINLINE CVector2& operator+=(float Rhs) noexcept;

    /**
     * @brief: Returns the result of component-wise subtraction between this and another vector
     *
     * @param Rhs: The vector to subtract
     * @return: A vector with the result of subtraction
     */
    FORCEINLINE CVector2 operator-(const CVector2& Rhs) const noexcept;

    /**
     * @brief: Returns this vector after component-wise subtraction between this and another vector
     *
     * @param Rhs: The vector to subtract
     * @return: A reference to this vector
     */
    FORCEINLINE CVector2& operator-=(const CVector2& Rhs) noexcept;

    /**
     * @brief: Returns the result of subtracting each component of this vector with a scalar
     *
     * @param Rhs: The scalar to subtract
     * @return: A vector with the result of the subtraction
     */
    FORCEINLINE CVector2 operator-(float Rhs) const noexcept;

    /**
     * @brief: Returns this vector after subtracting each component of this vector with a scalar
     *
     * @param Rhs: The scalar to subtract
     * @return: A reference to this vector
     */
    FORCEINLINE CVector2& operator-=(float Rhs) noexcept;

    /**
     * @brief: Returns the result of component-wise multiplication with this and another vector
     *
     * @param Rhs: The vector to multiply with
     * @return: A vector with the result of the multiplication
     */
    FORCEINLINE CVector2 operator*(const CVector2& Rhs) const noexcept;

    /**
     * @brief: Returns this vector after component-wise multiplication with this and another vector
     *
     * @param Rhs: The vector to multiply with
     * @return: A reference to this vector
     */
    FORCEINLINE CVector2& operator*=(const CVector2& Rhs) noexcept;

    /**
     * @brief: Returns the result of multiplying each component of this vector with a scalar
     *
     * @param Rhs: The scalar to multiply with
     * @return: A vector with the result of the multiplication
     */
    FORCEINLINE CVector2 operator*(float Rhs) const noexcept;

    /**
     * @brief: Returns the result of multiplying each component of a vector with a scalar
     *
     * @param Lhs: The scalar to multiply with
     * @param Rhs: The vector to multiply with
     * @return: A vector with the result of the multiplication
     */
    friend FORCEINLINE CVector2 operator*(float Lhs, const CVector2& Rhs) noexcept;

    /**
     * @brief: Returns this vector after multiplying each component of this vector with a scalar
     *
     * @param Rhs: The scalar to multiply with
     * @return: A reference to this vector
     */
    FORCEINLINE CVector2 operator*=(float Rhs) noexcept;

    /**
     * @brief: Returns the result of component-wise division with this and another vector
     *
     * @param Rhs: The vector to divide with
     * @return: A vector with the result of the division
     */
    FORCEINLINE CVector2 operator/(const CVector2& Rhs) const noexcept;

    /**
     * @brief: Returns this vector after component-wise division with this and another vector
     *
     * @param Rhs: The vector to divide with
     * @return: A reference to this vector
     */
    FORCEINLINE CVector2& operator/=(const CVector2& Rhs) noexcept;

    /**
     * @brief: Returns the result of dividing each component of this vector and a scalar
     *
     * @param Rhs: The scalar to divide with
     * @return: A vector with the result of the division
     */
    FORCEINLINE CVector2 operator/(float Rhs) const noexcept;

    /**
     * @brief: Returns this vector after dividing each component of this vector and a scalar
     *
     * @param Rhs: The scalar to divide with
     * @return: A reference to this vector
     */
    FORCEINLINE CVector2& operator/=(float Rhs) noexcept;

    /**
     * @brief: Returns the result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return: True if equal, false if not
     */
    FORCEINLINE bool operator==(const CVector2& Other) const noexcept;

    /**
     * @brief: Returns the negated result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return: False if equal, true if not
     */
    FORCEINLINE bool operator!=(const CVector2& Other) const noexcept;

    /**
     * @brief: Returns the component specifed
     *
     * @param Index: The component index
     * @return: The component
     */
    FORCEINLINE float& operator[](int Index) noexcept;

    /**
     * @brief: Returns the component specifed
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
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Implementation

FORCEINLINE CVector2::CVector2() noexcept
    : x(0.0f)
    , y(0.0f)
{
}

FORCEINLINE CVector2::CVector2(float InX, float InY) noexcept
    : x(InX)
    , y(InY)
{
}

FORCEINLINE CVector2::CVector2(const float* Arr) noexcept
    : x(Arr[0])
    , y(Arr[1])
{
}

FORCEINLINE CVector2::CVector2(float Scalar) noexcept
    : x(Scalar)
    , y(Scalar)
{
}

inline void CVector2::Normalize() noexcept
{
    float fLengthSquared = LengthSquared();
    if (fLengthSquared != 0.0f)
    {
        float fRecipLength = 1.0f / NMath::Sqrt(fLengthSquared);
        x = x * fRecipLength;
        y = y * fRecipLength;
    }
}

FORCEINLINE CVector2 CVector2::GetNormalized() const noexcept
{
    CVector2 Result(*this);
    Result.Normalize();
    return Result;
}

FORCEINLINE bool CVector2::IsEqual(const CVector2& Other, float Epsilon) const noexcept
{
    Epsilon = NMath::Abs(Epsilon);

    for (int i = 0; i < 2; i++)
    {
        float Diff = reinterpret_cast<const float*>(this)[i] - reinterpret_cast<const float*>(&Other)[i];
        if (NMath::Abs(Diff) > Epsilon)
        {
            return false;
        }
    }

    return true;
}

FORCEINLINE bool CVector2::IsUnitVector() const noexcept
{
    // LengthSquared should be the same as length if this is a unit vector
    // However, this way the sqrt can be avoided
    float fLengthSquared = NMath::Abs(1.0f - LengthSquared());
    return (fLengthSquared < NMath::kIsEqualEpsilon);
}

FORCEINLINE bool CVector2::HasNan() const noexcept
{
    for (int i = 0; i < 2; i++)
    {
        if (NMath::IsNaN(reinterpret_cast<const float*>(this)[i]))
        {
            return true;
        }
    }

    return false;
}

FORCEINLINE bool CVector2::HasInfinity() const noexcept
{
    for (int i = 0; i < 2; i++)
    {
        if (NMath::IsInfinity(reinterpret_cast<const float*>(this)[i]))
        {
            return true;
        }
    }

    return false;
}

FORCEINLINE bool CVector2::IsValid() const noexcept
{
    return !HasNan() && !HasInfinity();
}

FORCEINLINE float CVector2::Length() const noexcept
{
    float fLengthSquared = LengthSquared();
    return NMath::Sqrt(fLengthSquared);
}

FORCEINLINE float CVector2::LengthSquared() const noexcept
{
    return DotProduct(*this);
}

FORCEINLINE float CVector2::DotProduct(const CVector2& Other) const noexcept
{
    return (x * Other.x) + (y * Other.y);
}

inline CVector2 CVector2::ProjectOn(const CVector2& Other) const noexcept
{
    float AdotB = this->DotProduct(Other);
    float BdotB = Other.LengthSquared();
    return (AdotB / BdotB) * Other;
}

FORCEINLINE float* CVector2::Data() noexcept
{
    return reinterpret_cast<float*>(this);
}

FORCEINLINE const float* CVector2::Data() const noexcept
{
    return reinterpret_cast<const float*>(this);
}

FORCEINLINE CVector2 Min(const CVector2& Lhs, const CVector2& Rhs) noexcept
{
    return CVector2(NMath::Min(Lhs.x, Rhs.x), NMath::Min(Lhs.y, Rhs.y));
}

FORCEINLINE CVector2 Max(const CVector2& Lhs, const CVector2& Rhs) noexcept
{
    return CVector2(NMath::Max(Lhs.x, Rhs.x), NMath::Max(Lhs.y, Rhs.y));
}

FORCEINLINE CVector2 Lerp(const CVector2& First, const CVector2& Second, float t) noexcept
{
    return CVector2(
        (1.0f - t) * First.x + t * Second.x,
        (1.0f - t) * First.y + t * Second.y);
}

FORCEINLINE CVector2 Clamp(const CVector2& Min, const CVector2& Max, const CVector2& Value) noexcept
{
    return CVector2(
        NMath::Min(NMath::Max(Value.x, Min.x), Max.x),
        NMath::Min(NMath::Max(Value.y, Min.y), Max.y));
}

FORCEINLINE CVector2 Saturate(const CVector2& Value) noexcept
{
    return CVector2(
        NMath::Min(NMath::Max(Value.x, 0.0f), 1.0f),
        NMath::Min(NMath::Max(Value.y, 0.0f), 1.0f));
}

FORCEINLINE CVector2 CVector2::operator-() const noexcept
{
    return CVector2(-x, -y);
}

FORCEINLINE CVector2 CVector2::operator+(const CVector2& Rhs) const noexcept
{
    return CVector2(x + Rhs.x, y + Rhs.y);
}

FORCEINLINE CVector2& CVector2::operator+=(const CVector2& Rhs) noexcept
{
    return *this = *this + Rhs;
}

FORCEINLINE CVector2 CVector2::operator+(float Rhs) const noexcept
{
    return CVector2(x + Rhs, y + Rhs);
}

FORCEINLINE CVector2& CVector2::operator+=(float Rhs) noexcept
{
    return *this = *this + Rhs;
}

FORCEINLINE CVector2 CVector2::operator-(const CVector2& Rhs) const noexcept
{
    return CVector2(x - Rhs.x, y - Rhs.y);
}

FORCEINLINE CVector2& CVector2::operator-=(const CVector2& Rhs) noexcept
{
    return *this = *this - Rhs;
}

FORCEINLINE CVector2 CVector2::operator-(float Rhs) const noexcept
{
    return CVector2(x - Rhs, y - Rhs);
}

FORCEINLINE CVector2& CVector2::operator-=(float Rhs) noexcept
{
    return *this = *this - Rhs;
}

FORCEINLINE CVector2 CVector2::operator*(const CVector2& Rhs) const noexcept
{
    return CVector2(x * Rhs.x, y * Rhs.y);
}

FORCEINLINE CVector2& CVector2::operator*=(const CVector2& Rhs) noexcept
{
    return *this = *this * Rhs;
}

FORCEINLINE CVector2 CVector2::operator*(float Rhs) const noexcept
{
    return CVector2(x * Rhs, y * Rhs);
}

FORCEINLINE CVector2 operator*(float Lhs, const CVector2& Rhs) noexcept
{
    return CVector2(Lhs * Rhs.x, Lhs * Rhs.y);
}

FORCEINLINE CVector2 CVector2::operator*=(float Rhs) noexcept
{
    return *this = *this * Rhs;
}

FORCEINLINE CVector2 CVector2::operator/(const CVector2& Rhs) const noexcept
{
    return CVector2(x / Rhs.x, y / Rhs.y);
}

FORCEINLINE CVector2& CVector2::operator/=(const CVector2& Rhs) noexcept
{
    return *this = *this / Rhs;
}

FORCEINLINE CVector2 CVector2::operator/(float Rhs) const noexcept
{
    return CVector2(x / Rhs, y / Rhs);
}

FORCEINLINE CVector2& CVector2::operator/=(float Rhs) noexcept
{
    return *this = *this / Rhs;
}

FORCEINLINE float& CVector2::operator[](int Index) noexcept
{
    Assert(Index < 2);
    return reinterpret_cast<float*>(this)[Index];
}

FORCEINLINE float CVector2::operator[](int Index) const noexcept
{
    Assert(Index < 2);
    return reinterpret_cast<const float*>(this)[Index];
}

FORCEINLINE bool CVector2::operator==(const CVector2& Other) const noexcept
{
    return IsEqual(Other);
}

FORCEINLINE bool CVector2::operator!=(const CVector2& Other) const noexcept
{
    return !IsEqual(Other);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Radians and degree conversion

namespace NMath
{
    template<>
    FORCEINLINE CVector2 ToDegrees(CVector2 Radians)
    {
        return CVector2(ToDegrees(Radians.x), ToDegrees(Radians.y));
    }

    template<>
    FORCEINLINE CVector2 ToRadians(CVector2 Degrees)
    {
        return CVector2(ToRadians(Degrees.x), ToRadians(Degrees.y));
    }
}