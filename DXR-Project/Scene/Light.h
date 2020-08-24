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

	FORCEINLINE Float32 GetIntensity() const
	{
		return Intensity;
	}

	FORCEINLINE const XMFLOAT3& GetColor() const
	{
		return Color;
	}

protected:
	XMFLOAT3	Color;
	Float32		Intensity = 1.0f;
};