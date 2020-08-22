#pragma once
#include "Core/CoreObject.h"

class D3D12Buffer;
class D3D12Texture;
class D3D12DescriptorTable;

/*
* LightSettings
*/
struct LightSettings
{
	Uint16 ShadowMapWidth	= 4096;
	Uint16 ShadowMapHeight	= 4096;
};

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

	static FORCEINLINE void SetGlobalLightSettings(const LightSettings& InGlobalLightSettings)
	{
		GlobalLightSettings = InGlobalLightSettings;
	}

	static FORCEINLINE const LightSettings& GetGlobalLightSettings()
	{
		return GlobalLightSettings;
	}

protected:
	XMFLOAT3	Color;
	Float32		Intensity = 1.0f;

private:
	static LightSettings GlobalLightSettings;
};