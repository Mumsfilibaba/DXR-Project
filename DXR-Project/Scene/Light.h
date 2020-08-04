#pragma once
#include "Defines.h"
#include "Types.h"

class D3D12Buffer;
class D3D12Texture;

/*
* LightSettings
*/

struct LightSettings
{
	Uint16 ShadowWidth	= 1024;
	Uint16 ShadowHeight = 1024;
};

/*
* Light
*/

class Light
{
public:
	Light();
	~Light();

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

private:
	D3D12Buffer*	LightBuffer = nullptr;
	D3D12Texture*	ShadowMap	= nullptr;

	XMFLOAT3 Color;
	Float32 Intensity = 1.0f;

	static LightSettings GlobalLightSettings;
};