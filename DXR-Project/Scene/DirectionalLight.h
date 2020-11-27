#pragma once
#include "Light.h"

/*
* DirectionalLightProperties
*/

struct DirectionalLightProperties
{
	XMFLOAT3	Color			= XMFLOAT3(1.0f, 1.0f, 1.0f);
	Float		ShadowBias		= 0.005f;
	XMFLOAT3	Direction		= XMFLOAT3(0.0f, -1.0f, 0.0f);
	Float		MaxShadowBias	= 0.05f;
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
	void SetRotation(Float X, Float Y, Float Z);

	void SetLookAt(const XMFLOAT3& InInLookAt);
	void SetLookAt(Float X, Float Y, Float Z);

	void SetShadowNearPlane(Float InShadowNearPlane);
	void SetShadowFarPlane(Float InShadowFarPlane);

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

	FORCEINLINE const XMFLOAT3& GetLookAt() const
	{
		return LookAt;
	}

	FORCEINLINE const XMFLOAT4X4& GetMatrix() const
	{
		return Matrix;
	}

private:
	void CalculateMatrix();

	XMFLOAT3	Direction;
	XMFLOAT3	Rotation;
	XMFLOAT3	LookAt;
	XMFLOAT3	ShadowMapPosition;
	XMFLOAT4X4	Matrix;
};