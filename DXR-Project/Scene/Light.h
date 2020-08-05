#pragma once
#include "Defines.h"
#include "Types.h"

class D3D12Buffer;
class D3D12Texture;
class D3D12DescriptorTable;

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
	virtual ~Light();

	virtual bool Initialize(class D3D12Device* Device) = 0;

	virtual void BuildBuffer(class D3D12CommandList* CommandList) = 0;

	FORCEINLINE bool IsLightBufferDirty() const
	{
		return LightBufferDirty;
	}

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

	FORCEINLINE D3D12DescriptorTable* GetDescriptorTable() const
	{
		return DescriptorTable;
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
	D3D12Buffer*	LightBuffer = nullptr;
	D3D12Texture*	ShadowMap	= nullptr;
	D3D12DescriptorTable* DescriptorTable = nullptr;

	XMFLOAT3 Color;
	Float32 Intensity = 1.0f;

	bool LightBufferDirty = false;

private:
	static LightSettings GlobalLightSettings;
};