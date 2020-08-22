#pragma once
#include "Light.h"

struct PointLightProperties
{
	XMFLOAT3 Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	Float32 Padding;
	XMFLOAT3 Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
};

/*
* PointLight
*/
class PointLight : public Light
{
	CORE_OBJECT(PointLight, Light);

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