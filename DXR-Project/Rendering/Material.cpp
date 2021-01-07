#include "Material.h"
#include "Renderer.h"

#include "RenderingCore/CommandList.h"

/*
* Material
*/

Material::Material(const MaterialProperties& InProperties)
	: AlbedoMap(nullptr)
	, NormalMap(nullptr)
	, RoughnessMap(nullptr)
	, MetallicMap(nullptr)
	, AOMap(nullptr)
	, HeightMap(nullptr)
	, MaterialBuffer(nullptr)
	, Properties(InProperties)
{
}

void Material::Initialize()
{
	// Create materialbuffer
	MaterialBuffer = RenderingAPI::CreateConstantBuffer<MaterialProperties>(nullptr, BufferUsage_Default);
}

void Material::BuildBuffer(CommandList& CmdList)
{
	CmdList.TransitionBuffer(
		MaterialBuffer.Get(),
		EResourceState::ResourceState_VertexAndConstantBuffer,
		EResourceState::ResourceState_CopyDest);

	CmdList.UpdateBuffer(
		MaterialBuffer.Get(),
		0, sizeof(MaterialProperties),
		&Properties);

	CmdList.TransitionBuffer(
		MaterialBuffer.Get(),
		EResourceState::ResourceState_CopyDest,
		EResourceState::ResourceState_VertexAndConstantBuffer);

	MaterialBufferIsDirty = false;
}

void Material::SetAlbedo(const XMFLOAT3& Albedo)
{
	Properties.Albedo = Albedo;
	MaterialBufferIsDirty = true;
}

void Material::SetAlbedo(Float R, Float G, Float B)
{
	Properties.Albedo = XMFLOAT3(R, G, B);
	MaterialBufferIsDirty = true;
}

void Material::SetMetallic(Float Metallic)
{
	Properties.Metallic = Metallic;
	MaterialBufferIsDirty = true;
}

void Material::SetRoughness(Float Roughness)
{
	Properties.Roughness = Roughness;
	MaterialBufferIsDirty = true;
}

void Material::SetAmbientOcclusion(Float AO)
{
	Properties.AO = AO;
	MaterialBufferIsDirty = true;
}

void Material::EnableHeightMap(bool EnableHeightMap)
{
	if (EnableHeightMap)
	{
		Properties.EnableHeight = 1;
	}
	else
	{
		Properties.EnableHeight = 0;
	}
}

void Material::SetDebugName(const std::string& InDebugName)
{
	DebugName = InDebugName;
}
