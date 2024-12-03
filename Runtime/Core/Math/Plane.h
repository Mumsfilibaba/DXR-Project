#pragma once
#include "Core/Math/Vector4.h"

class VECTOR_ALIGN FPlane
{
public:

    /**
     * @brief Default constructor
     */
    FPlane() noexcept
        : X(0.0f)
        , Y(1.0f)
        , Z(0.0f)
        , W(0.0f)
    {
    }

    /**
     * @brief Constructor that creates a plane from a Vector4
     * @param Plane Vector4 representing a plane
     */
    explicit FPlane(const FVector4& Plane) noexcept
        : X(Plane.x)
        , Y(Plane.y)
        , Z(Plane.z)
        , W(Plane.w)
    {
    }

    /**
     * @brief Constructor that creates a plane from a normal and offset
     * @param Normal Normal of a plane
     * @param InW Offset from origin in direction of the normal
     */
    explicit FPlane(const FVector3& Normal, float InW) noexcept
        : X(Normal.x)
        , Y(Normal.y)
        , Z(Normal.z)
        , W(InW)
    {
    }
    
    /**
     * @brief Constructor that creates a plane from components of a Vector4
     * @param InX x-component of a Vector4
     * @param InY y-component of a Vector4
     * @param InZ z-component of a Vector4
     * @param InW w-component of a Vector4
     */
    explicit FPlane(float InX, float InY, float InZ, float InW) noexcept
        : X(InX)
        , Y(InY)
        , Z(InZ)
        , W(InW)
    {
    }

    /**
     * @brief Compares, within a threshold Epsilon, this plane with another plane
     * @param Other plane to compare against
     * @return True if equal, false if not
     */
    bool IsEqual(const FPlane& Other, float Epsilon = FMath::kIsEqualEpsilon) const noexcept
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
        FFloat128 Espilon_128 = FVectorMath::VectorSet1(Epsilon);
        Espilon_128 = FVectorMath::VectorAbs(Espilon_128);

        FFloat128 Diff = FVectorMath::VectorSub(XYZW, Other.XYZW);
        Diff = FVectorMath::VectorAbs(Diff);

        return FVectorMath::VectorLessThan(Diff, Espilon_128);
    #endif
    }

    /**
     * @brief Normalized the plane
     */
    void Normalize() noexcept
    {
    #if !USE_VECTOR_MATH
        FVector3 Normal = GetNormal();

        const float RcpLength = 1.0f / Normal.Length();
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
     * @brief Performs dot-product between a plane and a point
     * @param Point Point to perform dot-product with
     * @return Returns the dot-product
     */
    float DotProductCoord(const FVector3& Point) const noexcept
    {
    #if !USE_VECTOR_MATH
        FVector3 Normal = GetNormal();
        return Normal.DotProduct(Point) + W;
    #else
        FFloat128 Normal_128     = FVectorMath::VectorSet(X, Y, Z, 0.0f);
        FFloat128 Point_128      = FVectorMath::VectorSet(Point.x, Point.y, Point.z, 0.0f);
        FFloat128 DotProduct_128 = FVectorMath::VectorDot(Normal_128, Point_128);
        return FVectorMath::VectorGetX(DotProduct_128) + W;
    #endif
    }

    /**
     * @brief Retrieve the normal of the plane
     * @return Returns the normal of the plane
     */
    FVector3 GetNormal() const noexcept
    {
        return FVector3(X, Y, Z);
    }

    /**
     * @brief Retrieve the origin of the plane
     * @return Returns the origin of the plane
     */
    FVector3 GetOrigin() const noexcept
    {
        return FVector3(X * W, Y * W, Z * W);
    }

public:

    /**
     * @brief Returns the result after comparing this and another plane
     * @param Other The plane to compare with
     * @return True if equal, false if not
     */
    bool operator==(const FPlane& Other) const noexcept
    {
        return IsEqual(Other);
    }

    /**
     * @brief Returns the negated result after comparing this and another plane
     * @param Other The plane to compare with
     * @return False if equal, true if not
     */
    bool operator!=(const FPlane& Other) const noexcept
    {
        return !IsEqual(Other);
    }

public:

    union
    {
        struct
        {
            /** @brief The normals x-coordinate */
            float X;

            /** @brief The normals y-coordinate */
            float Y;

            /** @brief The normals z-coordinate */
            float Z;

            /** @brief The w-coordinate */
            float W;
        };
        
        float XYZW[4];
    };
    
};

MARK_AS_REALLOCATABLE(FPlane);
