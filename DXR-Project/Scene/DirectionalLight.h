#pragma once
#include "Light.h"

struct DirectionalLightProperties
{
	XMFLOAT3	Color			= XMFLOAT3(1.0f, 1.0f, 1.0f);
	Float32		ShadowBias		= 0.005f;
	XMFLOAT3	Direction		= XMFLOAT3(0.0f, -1.0f, 0.0f);
	Float32		MaxShadowBias	= 0.05f;
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

	// Rotation in Radians
	void SetRotation(const XMFLOAT3& InRotation);
	
	// Rotation in Radians
	void SetRotation(Float32 X, Float32 Y, Float32 Z);

	void SetDirection(const XMFLOAT3& InDirection);
	void SetDirection(Float32 X, Float32 Y, Float32 Z);

	void SetShadowMapPosition(const XMFLOAT3& InDirection);
	void SetShadowMapPosition(Float32 X, Float32 Y, Float32 Z);

	void SetShadowNearPlane(Float32 InShadowNearPlane);
	void SetShadowFarPlane(Float32 InShadowFarPlane);

	FORCEINLINE const XMFLOAT3& GetDirection() const
	{
		return Direction;
	}

	FORCEINLINE const XMFLOAT3& GetRotation() const
	{
		return Rotation;
	}

	FORCEINLINE const XMFLOAT3& GetShadowMapPosition() const
	{
		return ShadowMapPosition;
	}

	FORCEINLINE const XMFLOAT4X4& GetMatrix() const
	{
		return Matrix;
	}

private:
	void CalculateMatrix();

	XMFLOAT3	Direction;
	XMFLOAT3	Rotation;
	XMFLOAT3	ShadowMapPosition;
	XMFLOAT4X4	Matrix;
};