#pragma once
#include <DirectXMath.h>
using namespace DirectX;

#include "AABB.h"

class Frustum
{
public:
    Frustum() = default;

    Frustum( float ScreenDepth, const XMFLOAT4X4& View, const XMFLOAT4X4& Projection );

    void Create( float ScreenDepth, const XMFLOAT4X4& View, const XMFLOAT4X4& Projection );

    bool CheckAABB( const AABB& BoundingBox );

private:
    XMFLOAT4 Planes[6];
};