#include "Frustum.h"

CFrustum::CFrustum(float FarPlane, const CMatrix4& View, const CMatrix4& Projection)
    : Planes()
{
    Create(FarPlane, View, Projection);
}

void CFrustum::Create(float FarPlane, const CMatrix4& View, const CMatrix4& Projection)
{
    // Calculate the minimum Z distance in the frustum.
    CMatrix4 TempProjection = Projection;
    float MinimumZ = -TempProjection.m32 / TempProjection.m22;
    float r = FarPlane / (FarPlane - MinimumZ);
    TempProjection.m22 = r;
    TempProjection.m32 = -r * MinimumZ;

    // Create the frustum Matrix from the view Matrix and updated projection Matrix.
    CMatrix4 TempView = View.Transpose();
    CMatrix4 Matrix = TempView * TempProjection;

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
}

bool CFrustum::CheckAABB(const SAABB& Box)
{
    const CVector3 Center = Box.GetCenter();
    const float Width = Box.GetWidth() / 2.0f;
    const float Height = Box.GetHeight() / 2.0f;
    const float Depth = Box.GetDepth() / 2.0f;

    CVector3 Coords[8];
    Coords[0] = CVector3(Center.x - Width, Center.y - Height, Center.z - Depth);
    Coords[1] = CVector3(Center.x + Width, Center.y - Height, Center.z - Depth);
    Coords[2] = CVector3(Center.x - Width, Center.y + Height, Center.z - Depth);
    Coords[3] = CVector3(Center.x + Width, Center.y + Height, Center.z - Depth);
    Coords[4] = CVector3(Center.x - Width, Center.y - Height, Center.z + Depth);
    Coords[5] = CVector3(Center.x + Width, Center.y - Height, Center.z + Depth);
    Coords[6] = CVector3(Center.x - Width, Center.y + Height, Center.z + Depth);
    Coords[7] = CVector3(Center.x + Width, Center.y + Height, Center.z + Depth);

    for (int32 Index = 0; Index < 6; Index++)
    {
        const CPlane& Plane = Planes[Index];
        if (Plane.PlaneDotCoord(Coords[0]) >= 0.0f)
        {
            continue;
        }

        if (Plane.PlaneDotCoord(Coords[1]) >= 0.0f)
        {
            continue;
        }

        if (Plane.PlaneDotCoord(Coords[2]) >= 0.0f)
        {
            continue;
        }

        if (Plane.PlaneDotCoord(Coords[3]) >= 0.0f)
        {
            continue;
        }

        if (Plane.PlaneDotCoord(Coords[4]) >= 0.0f)
        {
            continue;
        }

        if (Plane.PlaneDotCoord(Coords[5]) >= 0.0f)
        {
            continue;
        }

        if (Plane.PlaneDotCoord(Coords[6]) >= 0.0f)
        {
            continue;
        }

        if (Plane.PlaneDotCoord(Coords[7]) >= 0.0f)
        {
            continue;
        }

        return false;
    }

    return true;
}
