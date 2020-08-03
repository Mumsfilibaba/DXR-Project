#include "Material.h"
#include "Renderer.h"

#include "D3D12/D3D12CommandList.h"

Material::Material(const MaterialProperties& InProperties)
	: AlbedoMap(nullptr)
	, NormalMap(nullptr)
	, Roughness(nullptr)
	, Metallic(nullptr)
	, AO(nullptr)
	, Height(nullptr)
	, MaterialBuffer(nullptr)
	, Properties(InProperties)
{
}

Material::~Material()
{
	SAFEDELETE(DescriptorTable);
	SAFEDELETE(MaterialBuffer);
}

void Material::Initialize(D3D12Device* Device)
{
	VALIDATE(AlbedoMap	!= nullptr);
	VALIDATE(NormalMap	!= nullptr);
	VALIDATE(Roughness	!= nullptr);
	VALIDATE(Height		!= nullptr);
	VALIDATE(AO			!= nullptr);
	VALIDATE(Metallic	!= nullptr);

	// Create materialbuffer
	BufferProperties MaterialBufferProps = { };
	MaterialBufferProps.Name		= "MaterialBuffer";
	MaterialBufferProps.Flags		= D3D12_RESOURCE_FLAG_NONE;
	MaterialBufferProps.InitalState	= D3D12_RESOURCE_STATE_COMMON;
	MaterialBufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_DEFAULT;
	MaterialBufferProps.SizeInBytes = 256; // Must atleast be a multiple of 256

	MaterialBuffer = new D3D12Buffer(Device);
	if (MaterialBuffer->Initialize(MaterialBufferProps))
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc = { };
		CBVDesc.BufferLocation	= MaterialBuffer->GetGPUVirtualAddress();
		CBVDesc.SizeInBytes		= MaterialBuffer->GetSizeInBytes();
		MaterialBuffer->SetConstantBufferView(std::make_shared<D3D12ConstantBufferView>(Device, MaterialBuffer->GetResource(), &CBVDesc));

		std::shared_ptr<D3D12ImmediateCommandList> CommandList = Renderer::Get()->GetImmediateCommandList();
		CommandList->TransitionBarrier(MaterialBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		CommandList->Flush();

		BuildBuffer(CommandList.get());
		CommandList->Flush();
	}
	else
	{
		return;
	}

	// Create descriptor table
	DescriptorTable = new D3D12DescriptorTable(Device, 7);
	DescriptorTable->SetShaderResourceView(AlbedoMap->GetShaderResourceView(0).get(), 0);
	DescriptorTable->SetShaderResourceView(NormalMap->GetShaderResourceView(0).get(), 1);
	DescriptorTable->SetShaderResourceView(Roughness->GetShaderResourceView(0).get(), 2);
	DescriptorTable->SetShaderResourceView(Height->GetShaderResourceView(0).get(), 3);
	DescriptorTable->SetShaderResourceView(Metallic->GetShaderResourceView(0).get(), 4);
	DescriptorTable->SetShaderResourceView(AO->GetShaderResourceView(0).get(), 5);
	DescriptorTable->SetConstantBufferView(MaterialBuffer->GetConstantBufferView().get(), 6);
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

void Material::SetAlbedo(Float32 R, Float32 G, Float32 B)
{
	Properties.Albedo = XMFLOAT3(R, G, B);
	MaterialBufferIsDirty = true;
}

void Material::SetMetallic(Float32 Metallic)
{
	Properties.Metallic = Metallic;
	MaterialBufferIsDirty = true;
}

void Material::SetRoughness(Float32 Roughness)
{
	Properties.Roughness = Roughness;
	MaterialBufferIsDirty = true;
}

void Material::SetAmbientOcclusion(Float32 AO)
{
	Properties.AO = AO;
	MaterialBufferIsDirty = true;
}

void Material::SetDebugName(const std::string& InDebugName)
{
	DebugName = InDebugName;
}
