#pragma once
#include <DirectXMath.h>
using namespace DirectX;

#include "AABB.h"

class Frustum
{
public:
    Frustum() = default;

    Frustum(Float ScreenDepth, const XMFLOAT4X4& View, const XMFLOAT4X4& Projection);

    void Create(Float ScreenDepth, const XMFLOAT4X4& View, const XMFLOAT4X4& Projection);

    Bool CheckAABB(const AABB& BoundingBox);

private:
    XMFLOAT4 Planes[6];
};