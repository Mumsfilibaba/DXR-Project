#pragma once
#include "Core/CoreObject.h"

/*
* Light
*/

class Light : public CoreObject
{
	CORE_OBJECT(Light, CoreObject);

public:
	Light();
	virtual ~Light();

	void SetColor(const XMFLOAT3& InColor);
	void SetColor(float R, float G, float B);
	
	void SetIntensity(float InIntensity);

	FORCEINLINE void SetShadowBias(float InShadowBias)
	{
		ShadowBias = InShadowBias;
	}

	FORCEINLINE void SetMaxShadowBias(float InShadowBias)
	{
		MaxShadowBias = InShadowBias;
	}

	FORCEINLINE float GetIntensity() const
	{
		return Intensity;
	}

	FORCEINLINE const XMFLOAT3& GetColor() const
	{
		return Color;
	}

	FORCEINLINE float GetShadowNearPlane() const
	{
		return ShadowNearPlane;
	}

	FORCEINLINE float GetShadowFarPlane() const
	{
		return ShadowFarPlane;
	}

	FORCEINLINE float GetShadowBias() const
	{
		return ShadowBias;
	}

	FORCEINLINE float GetMaxShadowBias() const
	{
		return MaxShadowBias;
	}

protected:
	XMFLOAT3 Color;
	float	 Intensity = 1.0f;
	float	 ShadowNearPlane;
	float	 ShadowFarPlane;
	float	 ShadowBias;
	float	 MaxShadowBias;
};