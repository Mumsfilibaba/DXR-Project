#pragma once
#include "Light.h"

/*
* PointLightProperties
*/

struct PointLightProperties
{
	XMFLOAT3	Color			= XMFLOAT3(1.0f, 1.0f, 1.0f);
	float		ShadowBias		= 0.005f;
	XMFLOAT3	Position		= XMFLOAT3(0.0f, 0.0f, 0.0f);
	float		FarPlane		= 10.0f;
	float		MaxShadowBias	= 0.05f;
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
	void SetPosition(float X, float Y, float Z);

	void SetShadowNearPlane(float InShadowNearPlane);
	void SetShadowFarPlane(float InShadowFarPlane);

	FORCEINLINE const XMFLOAT3& GetPosition() const
	{
		return Position;
	}

	FORCEINLINE const XMFLOAT4X4& GetMatrix(uint32 Index) const
	{
		VALIDATE(Index < 6);
		return Matrices[Index];
	}

	FORCEINLINE const XMFLOAT4X4& GetViewMatrix(uint32 Index) const
	{
		VALIDATE(Index < 6);
		return ViewMatrices[Index];
	}

	FORCEINLINE const XMFLOAT4X4& GetProjectionMatrix(uint32 Index) const
	{
		VALIDATE(Index < 6);
		return ProjMatrices[Index];
	}

private:
	void CalculateMatrices();

	XMFLOAT4X4	Matrices[6];
	XMFLOAT4X4	ViewMatrices[6];
	XMFLOAT4X4	ProjMatrices[6];
	XMFLOAT3	Position;
};