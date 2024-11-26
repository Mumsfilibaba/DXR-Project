#pragma once
#include "Core/Math/Vector3.h"

struct FAABB
{
public:

    /** @brief Default constructor */
    FAABB()
        : Min(0.0f, 0.0f, 0.0f)
        , Max(0.0f, 0.0f, 0.0f)
    {
    }

    /**
     * @brief Constructs an AABB from two points.
     * @param InPointA First point.
     * @param InPointB Second point.
     */
    FAABB(const FVector3& InPointA, const FVector3& InPointB)
        : Min(FVector3(FMath::Min(InPointA.x, InPointB.x), FMath::Min(InPointA.y, InPointB.y), FMath::Min(InPointA.z, InPointB.z)))
        , Max(FVector3(FMath::Max(InPointA.x, InPointB.x), FMath::Max(InPointA.y, InPointB.y), FMath::Max(InPointA.z, InPointB.z)))
    {
    }

    /**
     * @brief Calculates the center position of the bounding box.
     * @return The center position of the bounding box.
     */
    FORCEINLINE FVector3 GetCenter() const
    {
        return (Min + Max) * 0.5f;
    }

    /**
     * @brief Calculates the size of the bounding box.
     * @return A vector representing the width, height, and depth.
     */
    FORCEINLINE FVector3 GetSize() const
    {
        return Max - Min;
    }

    /**
     * @brief Calculates the width of the bounding box along the x-axis.
     * @return The width of the bounding box.
     */
    FORCEINLINE float GetWidth() const
    {
        return Max.x - Min.x;
    }

    /**
     * @brief Calculates the height of the bounding box along the y-axis.
     * @return The height of the bounding box.
     */
    FORCEINLINE float GetHeight() const
    {
        return Max.y - Min.y;
    }

    /**
     * @brief Calculates the depth of the bounding box along the z-axis.
     * @return The depth of the bounding box.
     */
    FORCEINLINE float GetDepth() const
    {
        return Max.z - Min.z;
    }

    /**
     * @brief Checks if the bounding box has zero volume.
     * @return `true` if the bounding box is empty, `false` otherwise.
     */
    bool IsEmpty() const
    {
        return (Max.x <= Min.x) || (Max.y <= Min.y) || (Max.z <= Min.z);
    }

    /**
     * @brief Checks if a point is inside the bounding box.
     * @param Point The point to check.
     * @return `true` if the point is inside the bounding box, `false` otherwise.
     */
    bool Contains(const FVector3& Point) const
    {
        return (Point.x >= Min.x && Point.x <= Max.x) && (Point.y >= Min.y && Point.y <= Max.y) && (Point.z >= Min.z && Point.z <= Max.z);
    }

    /**
     * @brief Checks if this bounding box intersects with another.
     * @param Other The other bounding box.
     * @return `true` if the bounding boxes intersect, `false` otherwise.
     */
    bool Intersects(const FAABB& Other) const
    {
        return (Max.x >= Other.Min.x && Min.x <= Other.Max.x) && (Max.y >= Other.Min.y && Min.y <= Other.Max.y) && (Max.z >= Other.Min.z && Min.z <= Other.Max.z);
    }

    /**
     * @brief Expands this bounding box to include another bounding box.
     * @param Other The bounding box to include.
     */
    void ExpandToInclude(const FAABB& Other)
    {
        Min.x = FMath::Min(Min.x, Other.Min.x);
        Min.y = FMath::Min(Min.y, Other.Min.y);
        Min.z = FMath::Min(Min.z, Other.Min.z);

        Max.x = FMath::Max(Max.x, Other.Max.x);
        Max.y = FMath::Max(Max.y, Other.Max.y);
        Max.z = FMath::Max(Max.z, Other.Max.z);
    }

    /**
     * @brief Expands the bounding box to include the given point.
     * @param Point The point to include.
     */
    void Encapsulate(const FVector3& Point)
    {
        Min.x = FMath::Min(Min.x, Point.x);
        Min.y = FMath::Min(Min.y, Point.y);
        Min.z = FMath::Min(Min.z, Point.z);

        Max.x = FMath::Max(Max.x, Point.x);
        Max.y = FMath::Max(Max.y, Point.y);
        Max.z = FMath::Max(Max.z, Point.z);
    }

public:

    /**
     * @brief Equality operator.
     * @param Other The bounding box to compare with.
     * @return `true` if the bounding boxes are equal, `false` otherwise.
     */
    bool operator==(const FAABB& Other) const
    {
        return Min == Other.Min && Max == Other.Max;
    }

    /** @brief Inequality operator */
    bool operator!=(const FAABB& Other) const
    {
        return !(*this == Other);
    }

public:

    /** @brief Minimum corner of the bounding box */
    FVector3 Min;

    /** @brief Maximum corner of the bounding box */
    FVector3 Max;
};

MARK_AS_REALLOCATABLE(FAABB);
