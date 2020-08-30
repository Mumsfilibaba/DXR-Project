#pragma once

/*
* AABB
*/
struct AABB
{
	XMFLOAT3 Top;
	XMFLOAT3 Bottom;

	FORCEINLINE Float32 GetWidth() const
	{
		return Top.x - Bottom.x;
	}

	FORCEINLINE Float32 GetHeight() const
	{
		return Top.y - Bottom.y;
	}

	FORCEINLINE Float32 GetDepth() const
	{
		return Top.z - Bottom.z;
	}
};