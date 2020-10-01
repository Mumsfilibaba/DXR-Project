#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Views.h"

/*
* EMemoryType
*/

enum class EMemoryType : Uint32
{
	MEMORY_TYPE_UNKNOWN	= 0,
	MEMORY_TYPE_UPLOAD	= 1,
	MEMORY_TYPE_DEFAULT	= 2,
};

/*
* D3D12Resource
*/

class D3D12Resource : public D3D12DeviceChild
{
public:
	D3D12Resource(D3D12Device* InDevice);
	virtual ~D3D12Resource();

	virtual bool Initialize(ID3D12Resource* InResource);
	
	// DeviceChild Interface
	virtual void SetDebugName(const std::string& Name) override;

	void SetShaderResourceView(TSharedPtr<D3D12ShaderResourceView> InShaderResourceView, const Uint32 SubresourceIndex);
	void SetUnorderedAccessView(TSharedPtr<D3D12UnorderedAccessView> InUnorderedAccessView, const Uint32 SubresourceIndex);

	FORCEINLINE EMemoryType GetMemoryType() const
	{
		return MemoryType;
	}

	FORCEINLINE D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const
	{
		return Resource->GetGPUVirtualAddress();
	}

	FORCEINLINE const D3D12_RESOURCE_DESC& GetDesc() const
	{
		return Desc;
	}

	FORCEINLINE ID3D12Resource* GetResource() const
	{
		return Resource.Get();
	}

	FORCEINLINE TSharedPtr<D3D12ShaderResourceView> GetShaderResourceView(const Uint32 SubresourceIndex) const
	{
		return ShaderResourceViews[SubresourceIndex];
	}

	FORCEINLINE TSharedPtr<D3D12UnorderedAccessView> GetUnorderedAccessView(const Uint32 SubresourceIndex) const
	{
		return UnorderedAccessViews[SubresourceIndex];
	}

protected:
	bool CreateResource(const D3D12_RESOURCE_DESC* InDesc, const D3D12_CLEAR_VALUE* OptimizedClearValue, D3D12_RESOURCE_STATES InitalState, EMemoryType InMemoryType);

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;

	TArray<TSharedPtr<D3D12ShaderResourceView>>		ShaderResourceViews;
	TArray<TSharedPtr<D3D12UnorderedAccessView>>	UnorderedAccessViews;

	D3D12_RESOURCE_DESC Desc;
	EMemoryType MemoryType;
};