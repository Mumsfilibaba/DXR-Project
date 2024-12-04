#pragma once
#include "Core/Math/Vector3.h"

struct FAABB
{
public:

    /** @brief Default constructor */
    FORCEINLINE FAABB()
        : Min(0.0f, 0.0f, 0.0f)
        , Max(0.0f, 0.0f, 0.0f)
    {
    }

    /**
     * @brief Constructs an AABB from two points.
     * @param InPointA First point.
     * @param InPointB Second point.
     */
    FORCEINLINE FAABB(const FVector3& InPointA, const FVector3& InPointB)
        : Min(FVector3(FMath::Min(InPointA.X, InPointB.X), FMath::Min(InPointA.Y, InPointB.Y), FMath::Min(InPointA.Z, InPointB.Z)))
        , Max(FVector3(FMath::Max(InPointA.X, InPointB.X), FMath::Max(InPointA.Y, InPointB.Y), FMath::Max(InPointA.Z, InPointB.Z)))
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
        return Max.X - Min.X;
    }

    /**
     * @brief Calculates the height of the bounding box along the y-axis.
     * @return The height of the bounding box.
     */
    FORCEINLINE float GetHeight() const
    {
        return Max.Y - Min.Y;
    }

    /**
     * @brief Calculates the depth of the bounding box along the z-axis.
     * @return The depth of the bounding box.
     */
    FORCEINLINE float GetDepth() const
    {
        return Max.Z - Min.Z;
    }

    /**
     * @brief Checks if the bounding box has zero volume.
     * @return `true` if the bounding box is empty, `false` otherwise.
     */
    FORCEINLINE bool IsEmpty() const
    {
        return (Max.X <= Min.X) || (Max.Y <= Min.Y) || (Max.Z <= Min.Z);
    }

    /**
     * @brief Checks if a point is inside the bounding box.
     * @param Point The point to check.
     * @return `true` if the point is inside the bounding box, `false` otherwise.
     */
    FORCEINLINE bool Contains(const FVector3& Point) const
    {
        return (Point.X >= Min.X && Point.X <= Max.X) && (Point.Y >= Min.Y && Point.Y <= Max.Y) && (Point.Z >= Min.Z && Point.Z <= Max.Z);
    }

    /**
     * @brief Checks if this bounding box intersects with another.
     * @param Other The other bounding box.
     * @return `true` if the bounding boxes intersect, `false` otherwise.
     */
    FORCEINLINE bool Intersects(const FAABB& Other) const
    {
        return (Max.X >= Other.Min.X && Min.X <= Other.Max.X) && (Max.Y >= Other.Min.Y && Min.Y <= Other.Max.Y) && (Max.Z >= Other.Min.Z && Min.Z <= Other.Max.Z);
    }

    /**
     * @brief Expands this bounding box to include another bounding box.
     * @param Other The bounding box to include.
     */
    FORCEINLINE void ExpandToInclude(const FAABB& Other)
    {
        Min.X = FMath::Min(Min.X, Other.Min.X);
        Min.Y = FMath::Min(Min.Y, Other.Min.Y);
        Min.Z = FMath::Min(Min.Z, Other.Min.Z);

        Max.X = FMath::Max(Max.X, Other.Max.X);
        Max.Y = FMath::Max(Max.Y, Other.Max.Y);
        Max.Z = FMath::Max(Max.Z, Other.Max.Z);
    }

    /**
     * @brief Expands the bounding box to include the given point.
     * @param Point The point to include.
     */
    FORCEINLINE void Encapsulate(const FVector3& Point)
    {
        Min.X = FMath::Min(Min.X, Point.X);
        Min.Y = FMath::Min(Min.Y, Point.Y);
        Min.Z = FMath::Min(Min.Z, Point.Z);

        Max.X = FMath::Max(Max.X, Point.X);
        Max.Y = FMath::Max(Max.Y, Point.Y);
        Max.Z = FMath::Max(Max.Z, Point.Z);
    }

public:

    /**
     * @brief Equality operator.
     * @param Other The bounding box to compare with.
     * @return `true` if the bounding boxes are equal, `false` otherwise.
     */
    FORCEINLINE bool operator==(const FAABB& Other) const
    {
        return Min == Other.Min && Max == Other.Max;
    }

    /** @brief Inequality operator */
    FORCEINLINE bool operator!=(const FAABB& Other) const
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
