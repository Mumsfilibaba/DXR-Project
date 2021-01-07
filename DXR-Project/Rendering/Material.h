#pragma once
#include "RenderingCore/Buffer.h"
#include "RenderingCore/Texture.h"

/*
* MaterialProperties
*/

struct MaterialProperties
{
	XMFLOAT3 Albedo	= XMFLOAT3(1.0f, 1.0f, 1.0f);
	Float Roughness	= 0.0f;
	Float Metallic	= 0.0f;
	Float AO		= 0.5f;
	Int32 EnableHeight	= 0;
};

/*
* Material
*/

class Material
{
public:
	Material(const MaterialProperties& InProperties);
	~Material() = default;

	void Initialize();

	void BuildBuffer(class CommandList& CmdList);

	FORCEINLINE bool IsBufferDirty() const
	{
		return MaterialBufferIsDirty;
	}

	void SetAlbedo(const XMFLOAT3& Albedo);
	void SetAlbedo(Float R, Float G, Float B);

	void SetMetallic(Float Metallic);
	void SetRoughness(Float Roughness);
	void SetAmbientOcclusion(Float AO);

	void EnableHeightMap(bool EnableHeightMap);

	void SetDebugName(const std::string& InDebugName);

	FORCEINLINE ConstantBuffer* GetMaterialBuffer() const
	{
		return MaterialBuffer.Get();
	}

	FORCEINLINE bool HasAlphaMask() const
	{
		return AlphaMask != nullptr;
	}

	FORCEINLINE const MaterialProperties& GetMaterialProperties() const 
	{
		return Properties;
	}

public:
	TSharedRef<Texture2D> AlbedoMap;
	TSharedRef<Texture2D> NormalMap;
	TSharedRef<Texture2D> RoughnessMap;
	TSharedRef<Texture2D> HeightMap;
	TSharedRef<Texture2D> AOMap;
	TSharedRef<Texture2D> MetallicMap;
	TSharedRef<Texture2D> AlphaMask;

private:
	std::string	DebugName;
	Bool MaterialBufferIsDirty = true;
	
	MaterialProperties			Properties;
	TSharedRef<ConstantBuffer> 	MaterialBuffer;
};