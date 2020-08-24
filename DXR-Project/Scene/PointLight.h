#pragma once
#include "Light.h"

struct PointLightProperties
{
	XMFLOAT3	Color			= XMFLOAT3(1.0f, 1.0f, 1.0f);
	Float32		ShadowBias		= 0.005f;
	XMFLOAT3	Position		= XMFLOAT3(0.0f, 0.0f, 0.0f);
	Float32		FarPlane		= 10.0f;
	Float32		MaxShadowBias	= 0.05f;
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

	void SetShadowNearPlane(Float32 InShadowNearPlane);
	void SetShadowFarPlane(Float32 InShadowFarPlane);

	FORCEINLINE const XMFLOAT3& GetPosition() const
	{
		return Position;
	}

	FORCEINLINE const XMFLOAT4X4& GetMatrix(Uint32 Index) const
	{
		VALIDATE(Index < 6);
		return Matrices[Index];
	}

private:
	void CalculateMatrices();

	XMFLOAT4X4	Matrices[6];
	XMFLOAT3	Position;
};