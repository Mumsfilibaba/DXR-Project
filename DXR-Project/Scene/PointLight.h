#pragma once
#include "Light.h"

struct PointLightProperties
{
	XMFLOAT3	Color		= XMFLOAT3(1.0f, 1.0f, 1.0f);
	Float32		ShadowBias	= 0.05f;
	XMFLOAT3	Position	= XMFLOAT3(0.0f, 0.0f, 0.0f);
	Float32		FarPlane	= 10.0f;
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

	FORCEINLINE void SetShadowBias(Float32 InShadowBias)
	{
		ShadowBias = InShadowBias;
	}

	FORCEINLINE const XMFLOAT3& GetPosition() const
	{
		return Position;
	}

	FORCEINLINE const XMFLOAT4X4& GetMatrix(Uint32 Index) const
	{
		VALIDATE(Index < 6);
		return Matrices[Index];
	}

	FORCEINLINE Float32 GetShadowNearPlane() const
	{
		return ShadowNearPlane;
	}

	FORCEINLINE Float32 GetShadowFarPlane() const
	{
		return ShadowFarPlane;
	}

	FORCEINLINE Float32 GetShadowBias() const
	{
		return ShadowBias;
	}

private:
	void CalculateMatrices();

	XMFLOAT4X4	Matrices[6];
	XMFLOAT3	Position;
	Float32		ShadowNearPlane;
	Float32		ShadowFarPlane;
	Float32		ShadowBias;
};