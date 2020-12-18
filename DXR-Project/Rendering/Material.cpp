#include "Material.h"
#include "Renderer.h"

#include "RenderingCore/CommandList.h"

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

Material::~Material()
{
	SAFEDELETE(MaterialBuffer);
}

void Material::Initialize()
{
	// Create materialbuffer
	MaterialBuffer = RenderingAPI::CreateConstantBuffer<MaterialProperties>(nullptr, BufferUsage_Default);
}

void Material::BuildBuffer(CommandList& CmdList)
{
	//CommandList->TransitionBarrier(MaterialBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
	//CommandList->UploadBufferData(MaterialBuffer, 0, &Properties, sizeof(MaterialProperties));
	//CommandList->TransitionBarrier(MaterialBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

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
