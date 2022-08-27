#pragma once
#include "AABB.h"
#include "Matrix4.h"
#include "Plane.h"

#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Class for creating a frustum

class CORE_API FFrustum
{
public:

    /**
     * @brief: Default constructor
     */
    FFrustum() = default;

    /**
     * @brief: Create a new frustum based on a view and projection matrix
     * 
     * @param FarPlane: FarPlane of the camera
     * @param View: View-matrix of the camera
     * @param Projection: Projection-matrix of the camera
     */
    FFrustum(float FarPlane, const FMatrix4& View, const FMatrix4& Projection);

    /**
     * @brief: Create a new frustum based on a view and projection matrix
     *
     * @param FarPlane: FarPlane of the camera
     * @param View: View-matrix of the camera
     * @param Projection: Projection-matrix of the camera
     */
    void Create(float FarPlane, const FMatrix4& View, const FMatrix4& Projection);

    /**
     * @brief: Checks if a bounding-box is intersecting with the frustum
     * 
     * @return: Returns true if the bounding-box is intersecting with the frustum
     */
    bool CheckAABB(const FAABB& BoundingBox);

private:
    FPlane Planes[6];
};