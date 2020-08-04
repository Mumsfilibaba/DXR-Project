#pragma once
#include "Light.h"

/*
* PointLight
*/

class PointLight
{
public:
	PointLight();
	~PointLight();

	void SetPosition(const XMFLOAT3& InPosition);
	void SetPosition(Float32 X, Float32 Y, Float32 Z);

	FORCEINLINE const XMFLOAT3& GetPosition() const
	{
		return Position;
	}

private:
	XMFLOAT3 Position;
};