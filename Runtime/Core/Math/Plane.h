#pragma once
#include "Vector4.h"

// TODO: Finish this class

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Represents a 3-D plane */

class VECTOR_ALIGN CPlane
{
public:

    /**
     * @brief: Default constructor
     */
    FORCEINLINE CPlane() noexcept;

    /**
     * @brief: Constructor that creates a plane from a Vector4
     * 
     * @param Plane: Vector4 representing a plane
     */
    FORCEINLINE explicit CPlane(const CVector4& Plane) noexcept;

    /**
     * @brief: Constructor that creates a plane from a normal and offset
     *
     * @param Normal: Normal of a plane
     * @param InW: Offset from origin in direction of the normal
     */
    FORCEINLINE explicit CPlane(const CVector3& Normal, float InW) noexcept;
    
    /**
     * @brief: Constructor that creates a plane from components of a Vector4
     *
     * @param InX: x-component of a Vector4
     * @param InY: y-component of a Vector4
     * @param InZ: z-component of a Vector4
     * @param InW: w-component of a Vector4
     */
    FORCEINLINE explicit CPlane(float InX, float InY, float InZ, float InW) noexcept;

    /**
     * @brief: Compares, within a threshold Epsilon, this plane with another plane
     *
     * @param Other: plane to compare against
     * @return: True if equal, false if not
     */
    inline bool IsEqual(const CPlane& Other, float Epsilon = NMath::kIsEqualEpsilon) const noexcept;

    /**
     * @brief: Normalized the plane
     */
    FORCEINLINE void Normalize() noexcept;

    /*
     * Performs dot-product between a plane and a point
     * 
     * @param Point: Point to perform dot-product with
     * @return: Returns the dot-product
     */
    FORCEINLINE float PlaneDotCoord(const CVector3& Point) const noexcept;

    /**
     * @brief: Retrieve the normal of the plane
     * 
     * @return: Returns the normal of the plane
     */
    FORCEINLINE CVector3 GetNormal() const noexcept;

    /**
     * @brief: Retrieve the data as an array
     * 
     * @return: A pointer to the data representing the plane
     */
    FORCEINLINE float* Data() noexcept;

    /**
     * @brief: Retrieve the data as an array
     *
     * @return: A pointer to the data representing the plane
     */
    FORCEINLINE const float* Data() const noexcept;

public:

    /**
     * @brief: Returns the result after comparing this and another plane
     *
     * @param Other: The plane to compare with
     * @return: True if equal, false if not
     */
    FORCEINLINE bool operator==(const CPlane& Other) const noexcept;

    /**
     * @brief: Returns the negated result after comparing this and another plane
     *
     * @param Other: The plane to compare with
     * @return: False if equal, true if not
     */
    FORCEINLINE bool operator!=(const CPlane& Other) const noexcept;

public:

     /** @brief: The normals x-coordinate */
    float x;
     /** @brief: The normals y-coordinate */
    float y;
     /** @brief: The normals z-coordinate */
    float z;
     /** @brief: The w-coordinate */
    float w;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Implementation

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

    NVectorOp::Float128 Espilon128 = NVectorOp::Load(Epsilon);
    Espilon128 = NVectorOp::Abs(Espilon128);

    NVectorOp::Float128 Diff = NVectorOp::Sub(this, &Other);
    Diff = NVectorOp::Abs(Diff);

    return NVectorOp::LessThan(Diff, Espilon128);

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

FORCEINLINE float* CPlane::Data() noexcept
{
    return reinterpret_cast<float*>(this);
}

FORCEINLINE const float* CPlane::Data() const noexcept
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
