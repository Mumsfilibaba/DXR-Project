#pragma once
#include "D3D12DeviceChild.h"

#include <dxcapi.h>

struct DescriptorTableDesc
{
	std::vector<D3D12_DESCRIPTOR_RANGE> Ranges;
	D3D12_SHADER_VISIBILITY Visibility;
};

struct RootSignatureDesc
{
	std::vector<DescriptorTableDesc>		DescriptorTables;
	std::vector<D3D12_STATIC_SAMPLER_DESC>	StaticSamplers;
	D3D12_ROOT_SIGNATURE_FLAGS Flags;
};

class D3D12RootSignature : public D3D12DeviceChild
{
public:
	D3D12RootSignature(D3D12Device* InDevice);
	~D3D12RootSignature();

	bool Initialize(const RootSignatureDesc& RootSignatureDesc);
	bool Initialize(IDxcBlob* ShaderBlob);

	// DeviceChild Interface
	virtual void SetName(const std::string& Name) override;

	FORCEINLINE ID3D12RootSignature* GetRootSignature() const
	{
		return RootSignature.Get();
	}

private:
	bool Initialize(const void* RootSignatureBlob, Uint32 BlobSizeInBytes);

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
};