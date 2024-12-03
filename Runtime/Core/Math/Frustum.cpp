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
    FMatrix4 View = InView.GetTranspose();
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
    Planes[0].X = CombinedMatrix.M[0][3] + CombinedMatrix.M[0][2];
    Planes[0].Y = CombinedMatrix.M[1][3] + CombinedMatrix.M[1][2];
    Planes[0].Z = CombinedMatrix.M[2][3] + CombinedMatrix.M[2][2];
    Planes[0].W = CombinedMatrix.M[3][3] + CombinedMatrix.M[3][2];
    Planes[0].Normalize();

    // Far Plane
    Planes[1].X = CombinedMatrix.M[0][3] - CombinedMatrix.M[0][2];
    Planes[1].Y = CombinedMatrix.M[1][3] - CombinedMatrix.M[1][2];
    Planes[1].Z = CombinedMatrix.M[2][3] - CombinedMatrix.M[2][2];
    Planes[1].W = CombinedMatrix.M[3][3] - CombinedMatrix.M[3][2];
    Planes[1].Normalize();

    // Left Plane
    Planes[2].X = CombinedMatrix.M[0][3] + CombinedMatrix.M[0][0];
    Planes[2].Y = CombinedMatrix.M[1][3] + CombinedMatrix.M[1][0];
    Planes[2].Z = CombinedMatrix.M[2][3] + CombinedMatrix.M[2][0];
    Planes[2].W = CombinedMatrix.M[3][3] + CombinedMatrix.M[3][0];
    Planes[2].Normalize();

    // Right Plane
    Planes[3].X = CombinedMatrix.M[0][3] - CombinedMatrix.M[0][0];
    Planes[3].Y = CombinedMatrix.M[1][3] - CombinedMatrix.M[1][0];
    Planes[3].Z = CombinedMatrix.M[2][3] - CombinedMatrix.M[2][0];
    Planes[3].W = CombinedMatrix.M[3][3] - CombinedMatrix.M[3][0];
    Planes[3].Normalize();

    // Top Plane
    Planes[4].X = CombinedMatrix.M[0][3] - CombinedMatrix.M[0][1];
    Planes[4].Y = CombinedMatrix.M[1][3] - CombinedMatrix.M[1][1];
    Planes[4].Z = CombinedMatrix.M[2][3] - CombinedMatrix.M[2][1];
    Planes[4].W = CombinedMatrix.M[3][3] - CombinedMatrix.M[3][1];
    Planes[4].Normalize();

    // Bottom Plane
    Planes[5].X = CombinedMatrix.M[0][3] + CombinedMatrix.M[0][1];
    Planes[5].Y = CombinedMatrix.M[1][3] + CombinedMatrix.M[1][1];
    Planes[5].Z = CombinedMatrix.M[2][3] + CombinedMatrix.M[2][1];
    Planes[5].W = CombinedMatrix.M[3][3] + CombinedMatrix.M[3][1];
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
    FMatrix4 InverseViewProjection = CombinedMatrix.GetInverse();

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
