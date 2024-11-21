#pragma once
#include "Core/Math/Vector4.h"

// TODO: Finish this class

class VECTOR_ALIGN FPlane
{
public:

    /**
     * @brief - Default constructor
     */
    FORCEINLINE FPlane() noexcept
        : x(0.0f)
        , y(1.0f)
        , z(0.0f)
        , w(0.0f)
    {
    }

    /**
     * @brief       - Constructor that creates a plane from a Vector4
     * @param Plane - Vector4 representing a plane
     */
    FORCEINLINE explicit FPlane(const FVector4& Plane) noexcept
        : x(Plane.x)
        , y(Plane.y)
        , z(Plane.z)
        , w(Plane.w)
    {
    }

    /**
     * @brief        - Constructor that creates a plane from a normal and offset
     * @param Normal - Normal of a plane
     * @param InW    - Offset from origin in direction of the normal
     */
    FORCEINLINE explicit FPlane(const FVector3& Normal, float InW) noexcept
        : x(Normal.x)
        , y(Normal.y)
        , z(Normal.z)
        , w(InW)
    {
    }
    
    /**
     * @brief     - Constructor that creates a plane from components of a Vector4
     * @param InX - x-component of a Vector4
     * @param InY - y-component of a Vector4
     * @param InZ - z-component of a Vector4
     * @param InW - w-component of a Vector4
     */
    FORCEINLINE explicit FPlane(float InX, float InY, float InZ, float InW) noexcept
        : x(InX)
        , y(InY)
        , z(InZ)
        , w(InW)
    {
    }

    /**
     * @brief       - Compares, within a threshold Epsilon, this plane with another plane
     * @param Other - plane to compare against
     * @return      - True if equal, false if not
     */
    inline bool IsEqual(const FPlane& Other, float Epsilon = FMath::kIsEqualEpsilon) const noexcept
    {
    #if !USE_VECTOR_MATH
        Epsilon = FMath::Abs(Epsilon);

        for (int32 Index = 0; Index < 4; ++Index)
        {
            float Diff = reinterpret_cast<const float*>(this)[Index] - reinterpret_cast<const float*>(&Other)[Index];
            if (FMath::Abs(Diff) > Epsilon)
            {
                return false;
            }
        }

        return true;
    #else
        FFloat128 Espilon128 = FVectorMath::Load(Epsilon);
        Espilon128 = FVectorMath::Abs(Espilon128);
        
        FFloat128 Diff = FVectorMath::VectorSub(reinterpret_cast<const float*>(this), reinterpret_cast<const float*>(&Other));
        Diff = FVectorMath::Abs(Diff);
        
        return FVectorMath::LessThan(Diff, Espilon128);
    #endif
    }

    /**
     * @brief - Normalized the plane
     */
    FORCEINLINE void Normalize() noexcept
    {
        FVector3 Normal = GetNormal();

        const float ReciprocalLength = 1.0f / Normal.Length();
        x *= ReciprocalLength;
        y *= ReciprocalLength;
        z *= ReciprocalLength;
        w *= ReciprocalLength;

        // TODO: Implement SIMD
    }

    /**
     * @brief       - Performs dot-product between a plane and a point
     * @param Point - Point to perform dot-product with
     * @return      - Returns the dot-product
     */
    FORCEINLINE float DotProductCoord(const FVector3& Point) const noexcept
    {
        FVector3 Normal = GetNormal();
        return Normal.DotProduct(Point) + w;

        // TODO: Implement SIMD
    }

    /**
     * @brief  - Retrieve the normal of the plane
     * @return - Returns the normal of the plane
     */
    FORCEINLINE FVector3 GetNormal() const noexcept
    {
        return *reinterpret_cast<const FVector3*>(this);
    }

    /**
     * @brief  - Retrieve the data as an array
     * @return - A pointer to the data representing the plane
     */
    FORCEINLINE float* Data() noexcept
    {
        return reinterpret_cast<float*>(this);
    }

    /**
     * @brief  - Retrieve the data as an array
     * @return - A pointer to the data representing the plane
     */
    FORCEINLINE const float* Data() const noexcept
    {
        return reinterpret_cast<const float*>(this);
    }

public:

    /**
     * @brief       - Returns the result after comparing this and another plane
     * @param Other - The plane to compare with
     * @return      - True if equal, false if not
     */
    FORCEINLINE bool operator==(const FPlane& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief       - Returns the negated result after comparing this and another plane
     * @param Other - The plane to compare with
     * @return      - False if equal, true if not
     */
    FORCEINLINE bool operator!=(const FPlane& Other) const noexcept
    {
        return !IsEqual(Other);
    }

public:

     /** @brief - The normals x-coordinate */
    float x;

     /** @brief - The normals y-coordinate */
    float y;
    
    /** @brief - The normals z-coordinate */
    float z;
    
    /** @brief - The w-coordinate */
    float w;
};

MARK_AS_REALLOCATABLE(FPlane);
