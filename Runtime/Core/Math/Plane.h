#pragma once
#include "Vector4.h"

// TODO: Finish this class

/* Represents a 3-D plane */
class VECTOR_ALIGN CPlane
{
public:

    FORCEINLINE CPlane() noexcept;

    FORCEINLINE explicit CPlane(const CVector4& Plane) noexcept;

    FORCEINLINE explicit CPlane(const CVector3& Normal, float InW) noexcept;

    FORCEINLINE explicit CPlane(float InX, float InY, float InZ, float InW) noexcept;

    /**
     * Compares, within a threshold Epsilon, this plane with another plane
     *
     * @param Other: plane to compare against
     * @return True if equal, false if not
     */
    inline bool IsEqual(const CPlane& Other, float Epsilon = NMath::IS_EQUAL_EPISILON) const noexcept;

    FORCEINLINE void Normalize() noexcept;

    FORCEINLINE float PlaneDotCoord(const CVector3& Point) const noexcept;

    FORCEINLINE CVector3 GetNormal() const noexcept;

    FORCEINLINE float* GetData() noexcept;

    FORCEINLINE const float* GetData() const noexcept;

public:

    /**
     * Returns the result after comparing this and another plane
     *
     * @param Other: The plane to compare with
     * @return True if equal, false if not
     */
    FORCEINLINE bool operator==(const CPlane& Other) const noexcept;

    /**
     * Returns the negated result after comparing this and another plane
     *
     * @param Other: The plane to compare with
     * @return False if equal, true if not
     */
    FORCEINLINE bool operator!=(const CPlane& Other) const noexcept;

public:
    /* The normals x-coordinate */
    float x;

    /* The normals y-coordinate */
    float y;

    /* The normals z-coordinate */
    float z;

    /* The w-coordinate */
    float w;
};

FORCEINLINE CPlane::CPlane() noexcept
    : x(0.0f)
    , y(1.0f)
    , z(0.0f)
    , w(0.0f)
{
}

FORCEINLINE CPlane::CPlane(const CVector4& Plane) noexcept
    : x(Plane.x)
    , y(Plane.y)
    , z(Plane.z)
    , w(Plane.w)
{
}

FORCEINLINE CPlane::CPlane(const CVector3& Normal, float InW) noexcept
    : x(Normal.x)
    , y(Normal.y)
    , z(Normal.z)
    , w(InW)
{
}

FORCEINLINE CPlane::CPlane(float InX, float InY, float InZ, float InW) noexcept
    : x(InX)
    , y(InY)
    , z(InZ)
    , w(InW)
{
}

FORCEINLINE bool CPlane::IsEqual(const CPlane& Other, float Epsilon) const noexcept
{
#if defined(DISABLE_SIMD)

    Epsilon = fabsf(Epsilon);

    for (int i = 0; i < 4; i++)
    {
        float Diff = reinterpret_cast<const float*>(this)[i] - reinterpret_cast<const float*>(&Other)[i];
        if (fabsf(Diff) > Epsilon)
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

FORCEINLINE void CPlane::Normalize() noexcept
{
    CVector3 Normal = GetNormal();

    float ReciprocalLength = 1.0f / Normal.Length();
    x = x * ReciprocalLength;
    y = y * ReciprocalLength;
    z = z * ReciprocalLength;
    w = w * ReciprocalLength;

    // TODO: Implement SIMD
}

FORCEINLINE float CPlane::PlaneDotCoord(const CVector3& Point) const noexcept
{
    CVector3 Normal = GetNormal();
    return Normal.DotProduct(Point) + w;

    // TODO: Implement SIMD
}

FORCEINLINE CVector3 CPlane::GetNormal() const noexcept
{
    return CVector3(x, y, z);
}

FORCEINLINE float* CPlane::GetData() noexcept
{
    return reinterpret_cast<float*>(this);
}

FORCEINLINE const float* CPlane::GetData() const noexcept
{
    return reinterpret_cast<const float*>(this);
}

FORCEINLINE bool CPlane::operator==(const CPlane& Other) const noexcept
{
    return IsEqual(Other);
}

FORCEINLINE bool CPlane::operator!=(const CPlane& Other) const noexcept
{
    return !IsEqual(Other);
}
