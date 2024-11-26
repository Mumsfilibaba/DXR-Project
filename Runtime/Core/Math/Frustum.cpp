#include "Core/Math/Frustum.h"

FFrustum::FFrustum(float FarPlane, const FMatrix4& View, const FMatrix4& Projection)
    : Planes()
    , Points()
{
    Initialize(FarPlane, View, Projection);
}

void FFrustum::Initialize(float FarPlane, const FMatrix4& InView, const FMatrix4& InProjection)
{
    // Combine the view and projection matrices
    FMatrix4 View = InView.Transpose();
    FMatrix4 CombinedMatrix = View * InProjection;

    // Extract frustum planes from the combined matrix
    ExtractPlanes(CombinedMatrix);

    // Generate frustum corner points in world space
    GenerateFrustumCorners(CombinedMatrix);
}

void FFrustum::ExtractPlanes(const FMatrix4& CombinedMatrix)
{
    // Extract the six planes of the frustum from the combined matrix
    // The order is: near, far, left, right, top, bottom

    // Near Plane
    Planes[0].x = CombinedMatrix.m03 + CombinedMatrix.m02;
    Planes[0].y = CombinedMatrix.m13 + CombinedMatrix.m12;
    Planes[0].z = CombinedMatrix.m23 + CombinedMatrix.m22;
    Planes[0].w = CombinedMatrix.m33 + CombinedMatrix.m32;
    Planes[0].Normalize();

    // Far Plane
    Planes[1].x = CombinedMatrix.m03 - CombinedMatrix.m02;
    Planes[1].y = CombinedMatrix.m13 - CombinedMatrix.m12;
    Planes[1].z = CombinedMatrix.m23 - CombinedMatrix.m22;
    Planes[1].w = CombinedMatrix.m33 - CombinedMatrix.m32;
    Planes[1].Normalize();

    // Left Plane
    Planes[2].x = CombinedMatrix.m03 + CombinedMatrix.m00;
    Planes[2].y = CombinedMatrix.m13 + CombinedMatrix.m10;
    Planes[2].z = CombinedMatrix.m23 + CombinedMatrix.m20;
    Planes[2].w = CombinedMatrix.m33 + CombinedMatrix.m30;
    Planes[2].Normalize();

    // Right Plane
    Planes[3].x = CombinedMatrix.m03 - CombinedMatrix.m00;
    Planes[3].y = CombinedMatrix.m13 - CombinedMatrix.m10;
    Planes[3].z = CombinedMatrix.m23 - CombinedMatrix.m20;
    Planes[3].w = CombinedMatrix.m33 - CombinedMatrix.m30;
    Planes[3].Normalize();

    // Top Plane
    Planes[4].x = CombinedMatrix.m03 - CombinedMatrix.m01;
    Planes[4].y = CombinedMatrix.m13 - CombinedMatrix.m11;
    Planes[4].z = CombinedMatrix.m23 - CombinedMatrix.m21;
    Planes[4].w = CombinedMatrix.m33 - CombinedMatrix.m31;
    Planes[4].Normalize();

    // Bottom Plane
    Planes[5].x = CombinedMatrix.m03 + CombinedMatrix.m01;
    Planes[5].y = CombinedMatrix.m13 + CombinedMatrix.m11;
    Planes[5].z = CombinedMatrix.m23 + CombinedMatrix.m21;
    Planes[5].w = CombinedMatrix.m33 + CombinedMatrix.m31;
    Planes[5].Normalize();
}

void FFrustum::GenerateFrustumCorners(const FMatrix4& CombinedMatrix)
{
    // Define the eight corners of the frustum in normalized device coordinates (NDC)
    const FVector3 FrustumCornersNDC[8] =
    {
        FVector3(-1.0f,  1.0f, 0.0f), // Near Top Left
        FVector3( 1.0f,  1.0f, 0.0f), // Near Top Right
        FVector3( 1.0f, -1.0f, 0.0f), // Near Bottom Right
        FVector3(-1.0f, -1.0f, 0.0f), // Near Bottom Left
        FVector3(-1.0f,  1.0f, 1.0f), // Far Top Left
        FVector3( 1.0f,  1.0f, 1.0f), // Far Top Right
        FVector3( 1.0f, -1.0f, 1.0f), // Far Bottom Right
        FVector3(-1.0f, -1.0f, 1.0f), // Far Bottom Left
    };

    // Invert the combined view-projection matrix to transform NDC to world space
    FMatrix4 InverseViewProjection = CombinedMatrix.Invert();

    // Transform each corner from NDC to world space
    for (int32 Corner = 0; Corner < 8; ++Corner)
    {
        Points[Corner] = InverseViewProjection.TransformCoord(FrustumCornersNDC[Corner]);
    }
}

bool FFrustum::IntersectsAABB(const FAABB& Box) const
{
    // Get the center and half-extents of the bounding box
    const FVector3 Center  = Box.GetCenter();
    const float HalfWidth  = Box.GetWidth()  / 2.0f;
    const float HalfHeight = Box.GetHeight() / 2.0f;
    const float HalfDepth  = Box.GetDepth()  / 2.0f;

    // Calculate the eight corners of the bounding box
    FVector3 BoxCorners[8];
    BoxCorners[0] = FVector3(Center.x - HalfWidth, Center.y - HalfHeight, Center.z - HalfDepth);
    BoxCorners[1] = FVector3(Center.x + HalfWidth, Center.y - HalfHeight, Center.z - HalfDepth);
    BoxCorners[2] = FVector3(Center.x - HalfWidth, Center.y + HalfHeight, Center.z - HalfDepth);
    BoxCorners[3] = FVector3(Center.x + HalfWidth, Center.y + HalfHeight, Center.z - HalfDepth);
    BoxCorners[4] = FVector3(Center.x - HalfWidth, Center.y - HalfHeight, Center.z + HalfDepth);
    BoxCorners[5] = FVector3(Center.x + HalfWidth, Center.y - HalfHeight, Center.z + HalfDepth);
    BoxCorners[6] = FVector3(Center.x - HalfWidth, Center.y + HalfHeight, Center.z + HalfDepth);
    BoxCorners[7] = FVector3(Center.x + HalfWidth, Center.y + HalfHeight, Center.z + HalfDepth);

    // Check each frustum plane
    for (int32 PlaneIndex = 0; PlaneIndex < 6; ++PlaneIndex)
    {
        int32 NumOutside = 0;

        // Check each corner against the plane
        for (int32 CornerIndex = 0; CornerIndex < 8; ++CornerIndex)
        {
            // If the corner is behind the plane, increment the counter
            if (Planes[PlaneIndex].DotProductCoord(BoxCorners[CornerIndex]) < 0.0f)
            {
                ++NumOutside;
            }
        }

        // If all corners are outside this plane, the box does not intersect the frustum
        if (NumOutside == 8)
        {
            return false;
        }
    }

    // The box intersects the frustum
    return true;
}
