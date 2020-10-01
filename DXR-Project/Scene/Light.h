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
	void SetColor(Float32 R, Float32 G, Float32 B);
	
	void SetIntensity(Float32 InIntensity);

	FORCEINLINE void SetShadowBias(Float32 InShadowBias)
	{
		ShadowBias = InShadowBias;
	}

	FORCEINLINE void SetMaxShadowBias(Float32 InShadowBias)
	{
		MaxShadowBias = InShadowBias;
	}

	FORCEINLINE Float32 GetIntensity() const
	{
		return Intensity;
	}

	FORCEINLINE const XMFLOAT3& GetColor() const
	{
		return Color;
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

	FORCEINLINE Float32 GetMaxShadowBias() const
	{
		return MaxShadowBias;
	}

protected:
	XMFLOAT3	Color;
	Float32		Intensity = 1.0f;
	Float32		ShadowNearPlane;
	Float32		ShadowFarPlane;
	Float32		ShadowBias;
	Float32		MaxShadowBias;
};