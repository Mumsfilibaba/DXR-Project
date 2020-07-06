#pragma once
#include "D3D12DeviceChild.h"

enum class EMemoryType : Uint32
{
	MEMORY_TYPE_UNKNOWN	= 0,
	MEMORY_TYPE_UPLOAD	= 1,
	MEMORY_TYPE_DEFAULT	= 2,
};

class D3D12ShaderResourceView;
class D3D12UnorderedAccessView;

class D3D12Resource : public D3D12DeviceChild
{
public:
	D3D12Resource(D3D12Device* InDevice);
	virtual ~D3D12Resource();

	virtual bool Initialize(ID3D12Resource* InResource);
	
	void SetShaderResourceView(std::shared_ptr<D3D12ShaderResourceView> InShaderResourceView, const Uint32 SubresourceIndex);
	void SetUnorderedAccessView(std::shared_ptr<D3D12UnorderedAccessView> InUnorderedAccessView, const Uint32 SubresourceIndex);

	FORCEINLINE D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
	{
		return Resource->GetGPUVirtualAddress();
	}

	FORCEINLINE ID3D12Resource* GetResource() const
	{
		return Resource.Get();
	}

	FORCEINLINE std::shared_ptr<D3D12ShaderResourceView> GetShaderResourceView(const Uint32 SubresourceIndex) const
	{
		return ShaderResourceViews[SubresourceIndex];
	}

	FORCEINLINE std::shared_ptr<D3D12UnorderedAccessView> GetUnorderedAccessView(const Uint32 SubresourceIndex) const
	{
		return UnorderedAccessViews[SubresourceIndex];
	}

public:
	// DeviceChild Interface
	virtual void SetName(const std::string& Name) override;
	
protected:
	bool CreateResource(const D3D12_RESOURCE_DESC* Desc, D3D12_RESOURCE_STATES InitalState, EMemoryType MemoryType);

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;

	std::vector<std::shared_ptr<D3D12ShaderResourceView>>	ShaderResourceViews;
	std::vector<std::shared_ptr<D3D12UnorderedAccessView>>	UnorderedAccessViews;
};