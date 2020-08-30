#pragma once

/*
* AABB
*/
struct AABB
{
	XMFLOAT3 Top;
	XMFLOAT3 Bottom;

	FORCEINLINE XMFLOAT3 GetCenter() const
	{
		return XMFLOAT3((Bottom.x + Top.x) * 0.5f, (Bottom.y + Top.y) * 0.5f, (Bottom.z + Top.z) * 0.5f);
	}

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