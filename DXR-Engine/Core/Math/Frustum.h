#pragma once
#include "AABB.h"
#include "Matrix4.h"
#include "Plane.h"

class CFrustum
{
public:
    CFrustum() = default;

    CFrustum( float ScreenDepth, const CMatrix4& View, const CMatrix4& Projection );

    void Create( float ScreenDepth, const CMatrix4& View, const CMatrix4& Projection );

    bool CheckAABB( const SAABB& BoundingBox );

private:
    CPlane Planes[6];
};