#pragma once
#include "Light.h"

struct DirectionalLightProperties
{
	XMFLOAT3	Color		= XMFLOAT3(1.0f, 1.0f, 1.0f);
	Float32		ShadowBias	= 0.005f;
	XMFLOAT3	Direction	= XMFLOAT3(0.0f, -1.0f, 0.0f);
	Float32		Padding1;
	XMFLOAT4X4	LightMatrix;
};

/*
* DirectionalLight
*/
class DirectionalLight : public Light
{
	CORE_OBJECT(DirectionalLight, Light);

public:
	DirectionalLight();
	~DirectionalLight();

	void SetDirection(const XMFLOAT3& InDirection);
	void SetDirection(Float32 X, Float32 Y, Float32 Z);

	void SetShadowMapPosition(const XMFLOAT3& InDirection);
	void SetShadowMapPosition(Float32 X, Float32 Y, Float32 Z);

	FORCEINLINE void SetShadowBias(Float32 InShadowBias)
	{
		ShadowBias = InShadowBias;
	}

	FORCEINLINE const XMFLOAT3& GetDirection() const
	{
		return Direction;
	}

	FORCEINLINE const XMFLOAT3& GetShadowMapPosition() const
	{
		return ShadowMapPosition;
	}

	FORCEINLINE const XMFLOAT4X4& GetMatrix() const
	{
		return Matrix;
	}

	FORCEINLINE Float32 GetShadowBias() const
	{
		return ShadowBias;
	}

private:
	void CalculateMatrix();

	XMFLOAT3 Direction;
	Float32 ShadowBias;
	XMFLOAT3 ShadowMapPosition;
	XMFLOAT4X4 Matrix;
};