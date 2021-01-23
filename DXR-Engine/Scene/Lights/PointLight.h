#pragma once
#include "Light.h"

/*
* PointLightProperties
*/

struct PointLightProperties
{
	XMFLOAT3	Color			= XMFLOAT3(1.0f, 1.0f, 1.0f);
	Float		ShadowBias		= 0.005f;
	XMFLOAT3	Position		= XMFLOAT3(0.0f, 0.0f, 0.0f);
	Float		FarPlane		= 10.0f;
	Float		MaxShadowBias	= 0.05f;
	Float		Radius			= 5.0f;
	
	Float Padding0;
	Float Padding1;
};

/*
* PointLight
*/

class PointLight : public Light
{
	CORE_OBJECT(PointLight, Light);

public:
	PointLight();
	~PointLight() = default;

	void SetPosition(const XMFLOAT3& InPosition);
	void SetPosition(Float x, Float y, Float z);

	void SetShadowNearPlane(Float InShadowNearPlane);
	void SetShadowFarPlane(Float InShadowFarPlane);

	FORCEINLINE const XMFLOAT3& GetPosition() const
	{
		return Position;
	}

	FORCEINLINE const XMFLOAT4X4& GetMatrix(UInt32 Index) const
	{
		VALIDATE(Index < 6);
		return Matrices[Index];
	}

	FORCEINLINE const XMFLOAT4X4& GetViewMatrix(UInt32 Index) const
	{
		VALIDATE(Index < 6);
		return ViewMatrices[Index];
	}

	FORCEINLINE const XMFLOAT4X4& GetProjectionMatrix(UInt32 Index) const
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