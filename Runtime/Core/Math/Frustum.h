#pragma once
#include "AABB.h"
#include "Matrix4.h"
#include "Plane.h"

#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Class for creating a frustum

class CORE_API CFrustum
{
public:

    /**
     * @brief: Default constructor
     */
    CFrustum() = default;

    /**
     * @brief: Create a new frustum based on a view and projection matrix
     * 
     * @param FarPlane: FarPlane of the camera
     * @param View: View-matrix of the camera
     * @param Projection: Projection-matrix of the camera
     */
    CFrustum(float FarPlane, const CMatrix4& View, const CMatrix4& Projection);

    /**
     * @brief: Create a new frustum based on a view and projection matrix
     *
     * @param FarPlane: FarPlane of the camera
     * @param View: View-matrix of the camera
     * @param Projection: Projection-matrix of the camera
     */
    void Create(float FarPlane, const CMatrix4& View, const CMatrix4& Projection);

    /**
     * @brief: Checks if a bounding-box is intersecting with the frustum
     * 
     * @return: Returns true if the bounding-box is intersecting with the frustum
     */
    bool CheckAABB(const SAABB& BoundingBox);

private:
    CPlane Planes[6];
};