#pragma once
#include "RenderingCore/Buffer.h"
#include "RenderingCore/Texture.h"

/*
* MaterialProperties
*/

struct MaterialProperties
{
	XMFLOAT3 Albedo		= XMFLOAT3(1.0f, 1.0f, 1.0f);
	Float32 Roughness	= 0.0f;
	Float32 Metallic	= 0.0f;
	Float32 AO			= 0.5f;
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
	void SetAlbedo(Float32 R, Float32 G, Float32 B);

	void SetMetallic(Float32 Metallic);
	void SetRoughness(Float32 Roughness);
	void SetAmbientOcclusion(Float32 AO);

	void SetDebugName(const std::string& InDebugName);

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

private:
	std::string				DebugName;
	MaterialProperties		Properties;
	TSharedRef<ConstantBuffer> MaterialBuffer;
	bool MaterialBufferIsDirty = true;
};