#include "Frustum.h"


FFrustum::FFrustum(float FarPlane, const FMatrix4& View, const FMatrix4& Projection)
    : Planes()
{
    Create(FarPlane, View, Projection);
}

DISABLE_UNREFERENCED_VARIABLE_WARNING

void FFrustum::Create(float FarPlane, const FMatrix4& InView, const FMatrix4& InProjection)
{
    // Create the frustum Matrix from the view Matrix and updated projection Matrix.
    FMatrix4 View   = InView.Transpose();
    FMatrix4 Matrix = View * InProjection;

    // Calculate near plane of frustum.
    Planes[0].x = Matrix.m03 + Matrix.m02;
    Planes[0].y = Matrix.m13 + Matrix.m12;
    Planes[0].z = Matrix.m23 + Matrix.m22;
    Planes[0].w = Matrix.m33 + Matrix.m32;
    Planes[0].Normalize();

    // Calculate far plane of frustum.
    Planes[1].x = Matrix.m03 - Matrix.m02;
    Planes[1].y = Matrix.m13 - Matrix.m12;
    Planes[1].z = Matrix.m23 - Matrix.m22;
    Planes[1].w = Matrix.m33 - Matrix.m32;
    Planes[1].Normalize();

    // Calculate left plane of frustum.
    Planes[2].x = Matrix.m03 + Matrix.m00;
    Planes[2].y = Matrix.m13 + Matrix.m10;
    Planes[2].z = Matrix.m23 + Matrix.m20;
    Planes[2].w = Matrix.m33 + Matrix.m30;
    Planes[2].Normalize();

    // Calculate right plane of frustum.
    Planes[3].x = Matrix.m03 - Matrix.m00;
    Planes[3].y = Matrix.m13 - Matrix.m10;
    Planes[3].z = Matrix.m23 - Matrix.m20;
    Planes[3].w = Matrix.m33 - Matrix.m30;
    Planes[3].Normalize();

    // Calculate top plane of frustum.
    Planes[4].x = Matrix.m03 - Matrix.m01;
    Planes[4].y = Matrix.m13 - Matrix.m11;
    Planes[4].z = Matrix.m23 - Matrix.m21;
    Planes[4].w = Matrix.m33 - Matrix.m31;
    Planes[4].Normalize();

    // Calculate bottom plane of frustum.
    Planes[5].x = Matrix.m03 + Matrix.m01;
    Planes[5].y = Matrix.m13 + Matrix.m11;
    Planes[5].z = Matrix.m23 + Matrix.m21;
    Planes[5].w = Matrix.m33 + Matrix.m31;
    Planes[5].Normalize();

    // Generate the points
    const FVector3 FrustumCorners[8] =
    {
        FVector3(-1.0f,  1.0f, 0.0f),
        FVector3( 1.0f,  1.0f, 0.0f),
        FVector3( 1.0f, -1.0f, 0.0f),
        FVector3(-1.0f, -1.0f, 0.0f),
        FVector3(-1.0f,  1.0f, 1.0f),
        FVector3( 1.0f,  1.0f, 1.0f),
        FVector3( 1.0f, -1.0f, 1.0f),
        FVector3(-1.0f, -1.0f, 1.0f),
    };

    const FMatrix4 InverseViewProjection = Matrix.Invert();
    for (int32 Corner = 0; Corner < 8; ++Corner)
    {
        Points[Corner] = InverseViewProjection.TransformCoord(FrustumCorners[Corner]);
    }
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

bool FFrustum::CheckAABB(const FAABB& Box) const
{
    const FVector3 Center = Box.GetCenter();
    const float Width  = Box.GetWidth()  / 2.0f;
    const float Height = Box.GetHeight() / 2.0f;
    const float Depth  = Box.GetDepth()  / 2.0f;

    FVector3 Coords[8];
    Coords[0] = FVector3(Center.x - Width, Center.y - Height, Center.z - Depth);
    Coords[1] = FVector3(Center.x + Width, Center.y - Height, Center.z - Depth);
    Coords[2] = FVector3(Center.x - Width, Center.y + Height, Center.z - Depth);
    Coords[3] = FVector3(Center.x + Width, Center.y + Height, Center.z - Depth);
    Coords[4] = FVector3(Center.x - Width, Center.y - Height, Center.z + Depth);
    Coords[5] = FVector3(Center.x + Width, Center.y - Height, Center.z + Depth);
    Coords[6] = FVector3(Center.x - Width, Center.y + Height, Center.z + Depth);
    Coords[7] = FVector3(Center.x + Width, Center.y + Height, Center.z + Depth);

    for (int32 PlaneIndex = 0; PlaneIndex < 6; ++PlaneIndex)
    {
        int32 NumOutside = 0;
        for (int32 CornerIndex = 0; CornerIndex < 8; ++CornerIndex)
        {
            if (Planes[PlaneIndex].DotProductCoord(Coords[CornerIndex]) < 0.0f)
                ++NumOutside;
        }

        // We know that the primitive is completely outside of the frustum
        if (NumOutside == 8)
            return false;
    }

    // Filter out false positive (Where one plane intersects large geometry)
    {
        int32 NumOutside = 0;
        for (int32 Index = 0; Index < 8; ++Index)
            NumOutside += (Points[Index].x > Box.Top.x) ? 1 : 0;

        if (NumOutside == 8)
            return false;
    }

    {
        int32 NumOutside = 0;
        for (int32 Index = 0; Index < 8; ++Index)
            NumOutside += (Points[Index].x < Box.Bottom.x) ? 1 : 0;

        if (NumOutside == 8)
            return false;
    }

    {
        int32 NumOutside = 0;
        for (int32 Index = 0; Index < 8; ++Index)
            NumOutside += (Points[Index].y > Box.Top.y) ? 1 : 0;

        if (NumOutside == 8)
            return false;
    }

    {
        int32 NumOutside = 0;
        for (int32 Index = 0; Index < 8; ++Index)
            NumOutside += (Points[Index].y < Box.Bottom.y) ? 1 : 0;

        if (NumOutside == 8)
            return false;
    }

    {
        int32 NumOutside = 0;
        for (int32 Index = 0; Index < 8; ++Index)
            NumOutside += (Points[Index].z > Box.Top.z) ? 1 : 0;

        if (NumOutside == 8)
            return false;
    }

    {
        int32 NumOutside = 0;
        for (int32 Index = 0; Index < 8; ++Index)
            NumOutside += (Points[Index].z < Box.Bottom.z) ? 1 : 0;

        if (NumOutside == 8)
            return false;
    }

    return true;
}
