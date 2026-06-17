#pragma once
#include "Core/Math/Math.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"

class VECTOR_ALIGN Plane
{
public:

    /**
     * @brief Default constructor
     * Initializes a plane with default values.
     */
    FORCEINLINE Plane() noexcept
        : X(0.0f)
        , Y(1.0f)
        , Z(0.0f)
        , W(0.0f)
    {
    }

    /**
     * @brief Constructor that creates a plane from a Vector4.
     * @param Plane A Vector4 representing the plane.
     */
    FORCEINLINE explicit Plane(const Vector4& Plane) noexcept
        : X(Plane.X)
        , Y(Plane.Y)
        , Z(Plane.Z)
        , W(Plane.W)
    {
    }

    /**
     * @brief Constructor that creates a plane from a normal vector and an offset.
     * @param Normal The normal vector of the plane.
     * @param InW The offset from the origin along the normal.
     */
    FORCEINLINE explicit Plane(const Vector3& Normal, float InW) noexcept
        : X(Normal.X)
        , Y(Normal.Y)
        , Z(Normal.Z)
        , W(InW)
    {
    }
    
    /**
     * @brief Constructor that creates a plane from individual components.
     * @param InX The x-component of the plane.
     * @param InY The y-component of the plane.
     * @param InZ The z-component of the plane.
     * @param InW The w-component of the plane.
     */
    FORCEINLINE explicit Plane(float InX, float InY, float InZ, float InW) noexcept
        : X(InX)
        , Y(InY)
        , Z(InZ)
        , W(InW)
    {
    }

    /**
     * @brief Compares this plane with another plane within a specified threshold.
     * @param Other The plane to compare against.
     * @param Threshold The threshold for comparison. Defaults to Math::Constants::CmpThreshold.
     * @return True if planes are approximately equal, false otherwise.
     */
    FORCEINLINE bool IsEqual(const Plane& Other, float Threshold = Math::Constants::CmpThreshold) const noexcept
    {
    #if !USE_VECTOR_MATH
        Threshold = Math::Abs(Threshold);

        for (int32 Index = 0; Index < 4; ++Index)
        {
            float Diff = XYZW[Index] - Other.XYZW[Index];
            if (Math::Abs(Diff) > Threshold)
            {
                return false;
            }
        }

        return true;
    #else
        FFloat128 Threshold_128 = FVectorMath::VectorSet1(Threshold);
        Threshold_128 = FVectorMath::VectorAbs(Threshold_128);

        FFloat128 Diff = FVectorMath::VectorSub(XYZW, Other.XYZW);
        Diff = FVectorMath::VectorAbs(Diff);

        return FVectorMath::VectorAllLessThan(Diff, Threshold_128);
    #endif
    }

    /**
     * @brief Checks whether this plane has any component that equals NaN.
     * @return True if any component equals NaN, false otherwise.
     */
    FORCEINLINE bool ContainsNaN() const noexcept
    {
        for (int32 Index = 0; Index < 4; ++Index)
        {
            if (Math::IsNaN(XYZW[Index]))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Checks whether this plane has any component that equals infinity.
     * @return True if any component equals infinity, false otherwise.
     */
    FORCEINLINE bool ContainsInfinity() const noexcept
    {
        for (int32 Index = 0; Index < 4; ++Index)
        {
            if (Math::IsInfinity(XYZW[Index]))
            {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Flips the plane by negating its normal vector and offset.
     */
    FORCEINLINE Plane Flip() noexcept
    {
        return Plane(-X, -Y, -Z, -W);
    }

    /**
     * @brief Normalizes the plane to ensure its normal vector has unit length.
     */
    FORCEINLINE void Normalize() noexcept
    {
    #if !USE_VECTOR_MATH
        Vector3 Normal = GetNormal();

        const float RcpLength = 1.0f / Normal.GetLength();
        X *= RcpLength;
        Y *= RcpLength;
        Z *= RcpLength;
        W *= RcpLength;
    #else
        FFloat128 XYZW_128      = FVectorMath::VectorLoad(XYZW);
        FFloat128 VectorA       = FVectorMath::VectorDot(XYZW_128, XYZW_128);
        FFloat128 RcpLength_128 = FVectorMath::VectorRecipSqrt(VectorA);
        FFloat128 Result_128    = FVectorMath::VectorMul(XYZW_128, RcpLength_128);
        FVectorMath::VectorStore(Result_128, XYZW);
    #endif
    }

    /**
     * @brief Performs the dot product between the plane and a point.
     * @param Point The point to compute the dot product with.
     * @return The resulting dot product value.
     */
    FORCEINLINE float DotProductCoord(const Vector3& Point) const noexcept
    {
    #if !USE_VECTOR_MATH
        Vector3 Normal = GetNormal();
        return Normal.DotProduct(Point) + W;
    #else
        FFloat128 Normal_128     = FVectorMath::VectorSet(X, Y, Z, 0.0f);
        FFloat128 Point_128      = FVectorMath::VectorSet(Point.X, Point.Y, Point.Z, 0.0f);
        FFloat128 DotProduct_128 = FVectorMath::VectorDot(Normal_128, Point_128);
        return FVectorMath::VectorGetX(DotProduct_128) + W;
    #endif
    }

    /**
     * @brief Creates a plane from a point on the plane and the plane's normal vector.
     * @param Point A point on the plane.
     * @param Normal The normal vector of the plane (does not need to be normalized).
     * @return A plane with normalized normal and W = -Normal.DotProduct(Point).
     */
    static FORCEINLINE Plane FromPointAndNormal(const Vector3& Point, const Vector3& Normal) noexcept
    {
        const Vector3 NormalizedNormal = Normal.GetNormalized();
        return Plane(NormalizedNormal, -NormalizedNormal.DotProduct(Point));
    }

    /**
     * @brief Computes the intersection parameter t of a ray with this plane.
     *        The intersection point is Origin + t * Direction.
     * @param Origin The ray origin.
     * @param Direction The ray direction (does not need to be normalized).
     * @return The parameter t, or -1.0f if the ray is parallel to the plane (no intersection).
     */
    FORCEINLINE float IntersectRay(const Vector3& Origin, const Vector3& Direction) const noexcept
    {
        const float Denominator = GetNormal().DotProduct(Direction);
        if (Math::Abs(Denominator) < Math::Constants::Epsilon)
        {
            return -1.0f;
        }
        const float Numerator = DotProductCoord(Origin);
        return -(Numerator / Denominator);
    }

    /**
     * @brief Retrieves the normal vector of the plane.
     * @return A Vector3 representing the plane's normal vector.
     */
    FORCEINLINE Vector3 GetNormal() const noexcept
    {
        return Vector3(X, Y, Z);
    }

    /**
     * @brief Retrieves the origin point of the plane.
     * @return A Vector3 representing the plane's origin.
     */
    FORCEINLINE Vector3 GetOrigin() const noexcept
    {
        return Vector3(X * W, Y * W, Z * W);
    }

public:

    /**
     * @brief Compares this plane with another for equality.
     * @param Other The plane to compare with.
     * @return True if planes are equal, false otherwise.
     */
    FORCEINLINE bool operator==(const Plane& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief Compares this plane with another for inequality.
     * @param Other The plane to compare with.
     * @return True if planes are not equal, false otherwise.
     */
    FORCEINLINE bool operator!=(const Plane& Other) const noexcept
    {
        return !IsEqual(Other);
    }

public:

    union
    {
        struct
        {
            /** @brief The normal's x-coordinate. */
            float X;

            /** @brief The normal's y-coordinate. */
            float Y;

            /** @brief The normal's z-coordinate. */
            float Z;

            /** @brief The w-component of the plane. */
            float W;
        };
        
        float XYZW[4];
    };
};

MARK_AS_REALLOCATABLE(Plane);
