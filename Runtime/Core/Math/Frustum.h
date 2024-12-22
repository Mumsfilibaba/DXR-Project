#pragma once
#include "Core/Math/AABB.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Plane.h"

class CORE_API FFrustum
{
public:

    /**
     * @brief Default constructor initializes an empty frustum.
     */
    FFrustum() = default;

    /**
     * @brief Constructs a frustum based on view and projection matrices.
     * @param FarPlane Far plane distance of the camera.
     * @param View View matrix of the camera.
     * @param Projection Projection matrix of the camera.
     */
    FFrustum(float FarPlane, const FMatrix4& View, const FMatrix4& Projection);

    /**
     * @brief Initializes or updates the frustum based on view and projection matrices.
     * @param FarPlane Far plane distance of the camera.
     * @param View View matrix of the camera.
     * @param Projection Projection matrix of the camera.
     */
    void Initialize(float FarPlane, const FMatrix4& View, const FMatrix4& Projection);

    /**
     * @brief Checks if a bounding box intersects with the frustum.
     * @param BoundingBox The axis-aligned bounding box to check.
     * @return True if the bounding box intersects with the frustum; otherwise, false.
     */
    bool IntersectsAABB(const FAABB& BoundingBox) const;

private:
    void ExtractPlanes(const FMatrix4& CombinedMatrix);
    void GenerateFrustumCorners(const FMatrix4& CombinedMatrix);

    FPlane   Planes[6];
    FVector3 Points[8];
};

MARK_AS_REALLOCATABLE(FFrustum);
