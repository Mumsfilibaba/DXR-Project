#pragma once
#include "MathCommon.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// A 3-D floating point vector (x, y, z)

class CVector3
{
public:

    /** Default constructor (Initialize components to zero) */
    FORCEINLINE CVector3() noexcept;

    /**
     * @brief: Constructor initializing all components with a corresponding value.
     *
     * @param InX: The x-coordinate
     * @param InY: The y-coordinate
     * @param InZ: The z-coordinate
     */
    FORCEINLINE explicit CVector3(float InX, float InY, float InZ) noexcept;

    /**
     * @brief: Constructor initializing all components with an array.
     *
     * @param Arr: Array with 3 elements
     */
    FORCEINLINE explicit CVector3(const float* Arr) noexcept;

    /**
     * @brief: Constructor initializing all components with a single value.
     *
     * @param Scalar: Value to set all components to
     */
    FORCEINLINE explicit CVector3(float Scalar) noexcept;

    /* Normalized this vector */
    inline void Normalize() noexcept;

    /**
     * @brief: Returns a normalized version of this vector
     *
     * @return A copy of this vector normalized
     */
    FORCEINLINE CVector3 GetNormalized() const noexcept;

    /**
     * @brief: Compares, within a threshold Epsilon, this vector with another vector
     *
     * @param Other: vector to compare against
     * @return True if equal, false if not
     */
    inline bool IsEqual(const CVector3& Other, float Epsilon = NMath::kIsEqualEpsilon) const noexcept;

    /**
     * @brief: Checks weather this vector is a unit vector not
     *
     * @return True if the length equals one, false if not
     */
    FORCEINLINE bool IsUnitVector() const noexcept;

    /**
     * @brief: Checks weather this vector has any component that equals NaN
     *
     * @return True if the any component equals NaN, false if not
     */
    FORCEINLINE bool HasNan() const noexcept;

    /**
     * @brief: Checks weather this vector has any component that equals infinity
     *
     * @return True if the any component equals infinity, false if not
     */
    FORCEINLINE bool HasInfinity() const noexcept;

    /**
     * @brief: Checks weather this vector has any value that equals infinity or NaN
     *
     * @return False if the any value equals infinity or NaN, true if not
     */
    FORCEINLINE bool IsValid() const noexcept;

    /**
     * @brief: Returns the length of this vector
     *
     * @return The length of the vector
     */
    FORCEINLINE float Length() const noexcept;

    /**
     * @brief: Returns the length of this vector squared
     *
     * @return The length of the vector squared
     */
    FORCEINLINE float LengthSquared() const noexcept;

    /**
     * @brief: Returns the dot product between this and another vector
     *
     * @param Other: The vector to perform dot product with
     * @return The dot product
     */
    FORCEINLINE float DotProduct(const CVector3& Other) const noexcept;

    /**
     * @brief: Returns the cross product of this vector and another vector.
     *
     * @param Other: The vector to perform cross product with
     * @return The cross product
     */
    inline CVector3 CrossProduct(const CVector3& Other) const noexcept;

    /**
     * @brief: Returns the resulting vector after projecting this vector onto another.
     *
     * @param Other: The vector to project onto
     * @return The projected vector
     */
    inline CVector3 ProjectOn(const CVector3& Other) const noexcept;

    /**
     * @brief: Returns the reflected vector after reflecting this vector around a normal.
     *
     * @param Normal: Vector to reflect around
     * @return The reflected vector
     */
    inline CVector3 Reflect(const CVector3& Normal) const noexcept;

    /**
     * @brief: Returns the data of this matrix as a pointer
     *
     * @return A pointer to the data
     */
    FORCEINLINE float* Data() noexcept;

    /**
     * @brief: Returns the data of this matrix as a pointer
     *
     * @return A pointer to the data
     */
    FORCEINLINE const float* Data() const noexcept;

public:

    /**
     * @brief: Returns a vector with the smallest of each component of two vectors
     *
     * @param First: First vector to compare with
     * @param Second: Second vector to compare with
     * @return A vector with the smallest components of First and Second
     */
    friend FORCEINLINE CVector3 Min(const CVector3& First, const CVector3& Second) noexcept;

    /**
     * @brief: Returns a vector with the largest of each component of two vectors
     *
     * @param First: First vector to compare with
     * @param Second: Second vector to compare with
     * @return A vector with the largest components of First and Second
     */
    friend FORCEINLINE CVector3 Max(const CVector3& First, const CVector3& Second) noexcept;

    /**
     * @brief: Returns the linear interpolation between two vectors
     *
     * @param First: First vector to interpolate
     * @param Second: Second vector to interpolate
     * @param Factor: Factor to interpolate with. Zero returns First, One returns seconds
     * @return A vector with the result of interpolation
     */
    friend FORCEINLINE CVector3 Lerp(const CVector3& First, const CVector3& Second, float t) noexcept;

    /**
     * @brief: Returns a vector with all the components within the range of a min and max value
     *
     * @param Min: Vector with minimum values
     * @param Max: Vector with maximum values
     * @param Value: Vector to clamp
     * @return A vector with the result of clamping
     */
    friend FORCEINLINE CVector3 Clamp(const CVector3& Min, const CVector3& Max, const CVector3& Value) noexcept;

    /**
     * @brief: Returns a vector with all the components within the range zero and one
     *
     * @param Value: Value to saturate
     * @return A vector with the result of saturation
     */
    friend FORCEINLINE CVector3 Saturate(const CVector3& Value) noexcept;

public:

    /**
     * @brief: Return a vector with component-wise negation of this vector
     *
     * @return A negated vector
     */
    FORCEINLINE CVector3 operator-() const noexcept;

    /**
     * @brief: Returns the result of component-wise adding this and another vector
     *
     * @param Rhs: The vector to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CVector3 operator+(const CVector3& Rhs) const noexcept;

    /**
     * @brief: Returns this vector after component-wise adding this with another vector
     *
     * @param Rhs: The vector to add
     * @return A reference to this vector
     */
    FORCEINLINE CVector3& operator+=(const CVector3& Rhs) noexcept;

    /**
     * @brief: Returns the result of adding a scalar to each component of this vector
     *
     * @param Rhs: The scalar to add
     * @return A vector with the result of addition
     */
    FORCEINLINE CVector3 operator+(float Rhs) const noexcept;

    /**
     * @brief: Returns this vector after adding a scalar to each component of this vector
     *
     * @param Rhs: The scalar to add
     * @return A reference to this vector
     */
    FORCEINLINE CVector3& operator+=(float Rhs) noexcept;

    /**
     * @brief: Returns the result of component-wise subtraction between this and another vector
     *
     * @param Rhs: The vector to subtract
     * @return A vector with the result of subtraction
     */
    FORCEINLINE CVector3 operator-(const CVector3& Rhs) const noexcept;

    /**
     * @brief: Returns this vector after component-wise subtraction between this and another vector
     *
     * @param Rhs: The vector to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CVector3& operator-=(const CVector3& Rhs) noexcept;

    /**
     * @brief: Returns the result of subtracting each component of this vector with a scalar
     *
     * @param Rhs: The scalar to subtract
     * @return A vector with the result of the subtraction
     */
    FORCEINLINE CVector3 operator-(float Rhs) const noexcept;

    /**
     * @brief: Returns this vector after subtracting each component of this vector with a scalar
     *
     * @param Rhs: The scalar to subtract
     * @return A reference to this vector
     */
    FORCEINLINE CVector3& operator-=(float Rhs) noexcept;

    /**
     * @brief: Returns the result of component-wise multiplication with this and another vector
     *
     * @param Rhs: The vector to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CVector3 operator*(const CVector3& Rhs) const noexcept;

    /**
     * @brief: Returns this vector after component-wise multiplication with this and another vector
     *
     * @param Rhs: The vector to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CVector3& operator*=(const CVector3& Rhs) noexcept;

    /**
     * @brief: Returns the result of multiplying each component of this vector with a scalar
     *
     * @param Rhs: The scalar to multiply with
     * @return A vector with the result of the multiplication
     */
    FORCEINLINE CVector3 operator*(float Rhs) const noexcept;

    /**
     * @brief: Returns the result of multiplying each component of a vector with a scalar
     *
     * @param Lhs: The scalar to multiply with
     * @param Rhs: The vector to multiply with
     * @return A vector with the result of the multiplication
     */
    friend FORCEINLINE CVector3 operator*(float Lhs, const CVector3& Rhs) noexcept;

    /**
     * @brief: Returns this vector after multiplying each component of this vector with a scalar
     *
     * @param Rhs: The scalar to multiply with
     * @return A reference to this vector
     */
    FORCEINLINE CVector3 operator*=(float Rhs) noexcept;

    /**
     * @brief: Returns the result of component-wise division with this and another vector
     *
     * @param Rhs: The vector to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CVector3 operator/(const CVector3& Rhs) const noexcept;

    /**
     * @brief: Returns this vector after component-wise division with this and another vector
     *
     * @param Rhs: The vector to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CVector3& operator/=(const CVector3& Rhs) noexcept;

    /**
     * @brief: Returns the result of dividing each component of this vector and a scalar
     *
     * @param Rhs: The scalar to divide with
     * @return A vector with the result of the division
     */
    FORCEINLINE CVector3 operator/(float Rhs) const noexcept;

    /**
     * @brief: Returns this vector after dividing each component of this vector and a scalar
     *
     * @param Rhs: The scalar to divide with
     * @return A reference to this vector
     */
    FORCEINLINE CVector3& operator/=(float Rhs) noexcept;

    /**
     * @brief: Returns the result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==(const CVector3& Other) const noexcept;

    /**
     * @brief: Returns the negated result after comparing this and another vector
     *
     * @param Other: The vector to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=(const CVector3& Other) const noexcept;

    /**
     * @brief: Returns the component specified
     *
     * @param Index: The component index
     * @return The component
     */
    FORCEINLINE float& operator[](int Index) noexcept;

    /**
     * @brief: Returns the component specified
     *
     * @param Index: The component index
     * @return The component
     */
    FORCEINLINE float operator[](int Index) const noexcept;

public:

    /* The x-coordinate */
    float x;
    /* The y-coordinate */
    float y;
    /* The z-coordinate */
    float z;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Implementation

FORCEINLINE CVector3::CVector3() noexcept
    : x(0.0f)
    , y(0.0f)
    , z(0.0f)
{
}

FORCEINLINE CVector3::CVector3(float InX, float InY, float InZ) noexcept
    : x(InX)
    , y(InY)
    , z(InZ)
{
}

FORCEINLINE CVector3::CVector3(const float* Arr) noexcept
    : x(Arr[0])
    , y(Arr[1])
    , z(Arr[2])
{
}

FORCEINLINE CVector3::CVector3(float Scalar) noexcept
    : x(Scalar)
    , y(Scalar)
    , z(Scalar)
{
}

inline void CVector3::Normalize() noexcept
{
    float fLengthSquared = LengthSquared();
    if (fLengthSquared != 0.0f)
    {
        float fRecipLength = 1.0f / NMath::Sqrt(fLengthSquared);
        x = x * fRecipLength;
        y = y * fRecipLength;
        z = z * fRecipLength;
    }
}

FORCEINLINE CVector3 CVector3::GetNormalized() const noexcept
{
    CVector3 Result(*this);
    Result.Normalize();
    return Result;
}

FORCEINLINE bool CVector3::IsEqual(const CVector3& Other, float Epsilon) const noexcept
{
    Epsilon = NMath::Abs(Epsilon);

    for (int i = 0; i < 3; i++)
    {
        float Diff = reinterpret_cast<const float*>(this)[i] - reinterpret_cast<const float*>(&Other)[i];
        if (NMath::Abs(Diff) > Epsilon)
        {
            return false;
        }
    }

    return true;
}

FORCEINLINE bool CVector3::IsUnitVector() const noexcept
{
    // LengthSquared should be the same as length if this is a unit vector
    // However, this way the sqrt can be avoided
    float fLengthSquared = NMath::Abs(1.0f - LengthSquared());
    return (fLengthSquared < NMath::kIsEqualEpsilon);
}

FORCEINLINE bool CVector3::HasNan() const noexcept
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

FORCEINLINE bool CVector3::HasInfinity() const noexcept
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

FORCEINLINE bool CVector3::IsValid() const noexcept
{
    return !HasNan() && !HasInfinity();
}

FORCEINLINE float CVector3::Length() const noexcept
{
    float fLengthSquared = LengthSquared();
    return NMath::Sqrt(fLengthSquared);
}

FORCEINLINE float CVector3::LengthSquared() const noexcept
{
    return DotProduct(*this);
}

FORCEINLINE float CVector3::DotProduct(const CVector3& Other) const noexcept
{
    return (x * Other.x) + (y * Other.y) + (z * Other.z);
}

FORCEINLINE CVector3 CVector3::CrossProduct(const CVector3& Other) const noexcept
{
    float NewX = (y * Other.z) - (z * Other.y);
    float NewY = (z * Other.x) - (x * Other.z);
    float NewZ = (x * Other.y) - (y * Other.x);
    return CVector3(NewX, NewY, NewZ);
}

inline CVector3 CVector3::ProjectOn(const CVector3& Other) const noexcept
{
    float AdotB = this->DotProduct(Other);
    float BdotB = Other.LengthSquared();
    return (AdotB / BdotB) * Other;
}

inline CVector3 CVector3::Reflect(const CVector3& Normal) const noexcept
{
    float VdotN = this->DotProduct(Normal);
    float NdotN = Normal.LengthSquared();
    return *this - ((2.0f * (VdotN / NdotN)) * Normal);
}

FORCEINLINE float* CVector3::Data() noexcept
{
    return reinterpret_cast<float*>(this);
}

FORCEINLINE const float* CVector3::Data() const noexcept
{
    return reinterpret_cast<const float*>(this);
}

FORCEINLINE CVector3 Min(const CVector3& Lhs, const CVector3& Rhs) noexcept
{
    return CVector3(NMath::Min(Lhs.x, Rhs.x), NMath::Min(Lhs.y, Rhs.y), NMath::Min(Lhs.z, Rhs.z));
}

FORCEINLINE CVector3 Max(const CVector3& Lhs, const CVector3& Rhs) noexcept
{
    return CVector3(NMath::Max(Lhs.x, Rhs.x), NMath::Max(Lhs.y, Rhs.y), NMath::Max(Lhs.z, Rhs.z));
}

FORCEINLINE CVector3 Lerp(const CVector3& First, const CVector3& Second, float t) noexcept
{
    return CVector3(
        (1.0f - t) * First.x + t * Second.x,
        (1.0f - t) * First.y + t * Second.y,
        (1.0f - t) * First.z + t * Second.z);
}

FORCEINLINE CVector3 Clamp(const CVector3& Min, const CVector3& Max, const CVector3& Value) noexcept
{
    return CVector3(
        NMath::Min(NMath::Max(Value.x, Min.x), Max.x),
        NMath::Min(NMath::Max(Value.y, Min.y), Max.y),
        NMath::Min(NMath::Max(Value.z, Min.z), Max.z));
}

FORCEINLINE CVector3 Saturate(const CVector3& Value) noexcept
{
    return CVector3(
        NMath::Min(NMath::Max(Value.x, 0.0f), 1.0f),
        NMath::Min(NMath::Max(Value.y, 0.0f), 1.0f),
        NMath::Min(NMath::Max(Value.z, 0.0f), 1.0f));
}

FORCEINLINE CVector3 CVector3::operator-() const noexcept
{
    return CVector3(-x, -y, -z);
}

FORCEINLINE CVector3 CVector3::operator+(const CVector3& Rhs) const noexcept
{
    return CVector3(x + Rhs.x, y + Rhs.y, z + Rhs.z);
}

FORCEINLINE CVector3& CVector3::operator+=(const CVector3& Rhs) noexcept
{
    return *this = *this + Rhs;
}

FORCEINLINE CVector3 CVector3::operator+(float Rhs) const noexcept
{
    return CVector3(x + Rhs, y + Rhs, z + Rhs);
}

FORCEINLINE CVector3& CVector3::operator+=(float Rhs) noexcept
{
    return *this = *this + Rhs;
}

FORCEINLINE CVector3 CVector3::operator-(const CVector3& Rhs) const noexcept
{
    return CVector3(x - Rhs.x, y - Rhs.y, z - Rhs.z);
}

FORCEINLINE CVector3& CVector3::operator-=(const CVector3& Rhs) noexcept
{
    return *this = *this - Rhs;
}

FORCEINLINE CVector3 CVector3::operator-(float Rhs) const noexcept
{
    return CVector3(x - Rhs, y - Rhs, z - Rhs);
}

FORCEINLINE CVector3& CVector3::operator-=(float Rhs) noexcept
{
    return *this = *this - Rhs;
}

FORCEINLINE CVector3 CVector3::operator*(const CVector3& Rhs) const noexcept
{
    return CVector3(x * Rhs.x, y * Rhs.y, z * Rhs.z);
}

FORCEINLINE CVector3& CVector3::operator*=(const CVector3& Rhs) noexcept
{
    return *this = *this * Rhs;
}

FORCEINLINE CVector3 CVector3::operator*(float Rhs) const noexcept
{
    return CVector3(x * Rhs, y * Rhs, z * Rhs);
}

FORCEINLINE CVector3 operator*(float Lhs, const CVector3& Rhs) noexcept
{
    return CVector3(Lhs * Rhs.x, Lhs * Rhs.y, Lhs * Rhs.z);
}

FORCEINLINE CVector3 CVector3::operator*=(float Rhs) noexcept
{
    return *this = *this * Rhs;
}

FORCEINLINE CVector3 CVector3::operator/(const CVector3& Rhs) const noexcept
{
    return CVector3(x / Rhs.x, y / Rhs.y, z / Rhs.z);
}

FORCEINLINE CVector3& CVector3::operator/=(const CVector3& Rhs) noexcept
{
    return *this = *this / Rhs;
}

FORCEINLINE CVector3 CVector3::operator/(float Rhs) const noexcept
{
    return CVector3(x / Rhs, y / Rhs, z / Rhs);
}

FORCEINLINE CVector3& CVector3::operator/=(float Rhs) noexcept
{
    return *this = *this / Rhs;
}

FORCEINLINE float& CVector3::operator[](int Index) noexcept
{
    Assert(Index < 4);
    return reinterpret_cast<float*>(this)[Index];
}

FORCEINLINE float CVector3::operator[](int Index) const noexcept
{
    Assert(Index < 4);
    return reinterpret_cast<const float*>(this)[Index];
}

FORCEINLINE bool CVector3::operator==(const CVector3& Other) const noexcept
{
    return IsEqual(Other);
}

FORCEINLINE bool CVector3::operator!=(const CVector3& Other) const noexcept
{
    return !IsEqual(Other);
}

namespace NMath
{
    template<>
    FORCEINLINE CVector3 ToDegrees(CVector3 Radians)
    {
        return CVector3(ToDegrees(Radians.x), ToDegrees(Radians.y), ToDegrees(Radians.z));
    }

    template<>
    FORCEINLINE CVector3 ToRadians(CVector3 Degrees)
    {
        return CVector3(ToRadians(Degrees.x), ToRadians(Degrees.y), ToRadians(Degrees.z));
    }
}
