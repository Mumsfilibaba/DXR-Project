#pragma once
#include <DirectXMath.h>
using namespace DirectX;

#include "AABB.h"

/*
* Frustum
*/

class Frustum
{
public:
	Frustum();
	Frustum(Float ScreenDepth, const XMFLOAT4X4& View, const XMFLOAT4X4& Projection);

	void Create(Float ScreenDepth, const XMFLOAT4X4& View, const XMFLOAT4X4& Projection);

	bool CheckAABB(const AABB& BoundingBox);

private:
	XMFLOAT4 Planes[6];
};