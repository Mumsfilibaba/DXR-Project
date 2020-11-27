#include "Material.h"
#include "Renderer.h"

#include "D3D12/D3D12CommandList.h"

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
	VALIDATE(AlbedoMap		!= nullptr);
	VALIDATE(NormalMap		!= nullptr);
	VALIDATE(RoughnessMap	!= nullptr);
	VALIDATE(HeightMap		!= nullptr);
	VALIDATE(AOMap			!= nullptr);
	VALIDATE(MetallicMap	!= nullptr);

	// Create materialbuffer
	BufferProperties MaterialBufferProps = { };
	MaterialBufferProps.Name		= "MaterialBuffer";
	MaterialBufferProps.Flags		= D3D12_RESOURCE_FLAG_NONE;
	MaterialBufferProps.InitalState	= D3D12_RESOURCE_STATE_COMMON;
	MaterialBufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_DEFAULT;
	MaterialBufferProps.SizeInBytes = 256; // Must atleast be a multiple of 256

	MaterialBuffer = RenderingAPI::Get().CreateBuffer(MaterialBufferProps);
	if (MaterialBuffer)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc = { };
		CBVDesc.BufferLocation	= MaterialBuffer->GetGPUVirtualAddress();
		CBVDesc.SizeInBytes		= MaterialBuffer->GetSizeInBytes();
		MaterialBuffer->SetConstantBufferView(TSharedPtr(RenderingAPI::Get().CreateConstantBufferView(MaterialBuffer->GetResource(), &CBVDesc)));

		TSharedPtr<D3D12ImmediateCommandList> CommandList = RenderingAPI::StaticGetImmediateCommandList();
		CommandList->TransitionBarrier(MaterialBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		CommandList->Flush();

		BuildBuffer(CommandList.Get());
		CommandList->Flush();
	}
	else
	{
		return;
	}

	// Create descriptor table
	DescriptorTable = TSharedPtr(RenderingAPI::Get().CreateDescriptorTable(8));
	DescriptorTable->SetShaderResourceView(AlbedoMap->GetShaderResourceView(0).Get(), 0);
	DescriptorTable->SetShaderResourceView(NormalMap->GetShaderResourceView(0).Get(), 1);
	DescriptorTable->SetShaderResourceView(RoughnessMap->GetShaderResourceView(0).Get(), 2);
	DescriptorTable->SetShaderResourceView(HeightMap->GetShaderResourceView(0).Get(), 3);
	DescriptorTable->SetShaderResourceView(MetallicMap->GetShaderResourceView(0).Get(), 4);
	DescriptorTable->SetShaderResourceView(AOMap->GetShaderResourceView(0).Get(), 5);
	DescriptorTable->SetConstantBufferView(MaterialBuffer->GetConstantBufferView().Get(), 6);
	
	if (AlphaMask)
	{
		DescriptorTable->SetShaderResourceView(AlphaMask->GetShaderResourceView(0).Get(), 7);
	}

	DescriptorTable->CopyDescriptors();
}

void Material::BuildBuffer(D3D12CommandList* CommandList)
{
	CommandList->TransitionBarrier(MaterialBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
	CommandList->UploadBufferData(MaterialBuffer, 0, &Properties, sizeof(MaterialProperties));
	CommandList->TransitionBarrier(MaterialBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

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
