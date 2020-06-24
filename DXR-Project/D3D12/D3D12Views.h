#pragma once
#include "D3D12DeviceChild.h"

/*
* D3D12DescriptorHandle
*/

class D3D12DescriptorHandle
{
public:
	D3D12DescriptorHandle::D3D12DescriptorHandle()
		: CPUHandle({ 0 })
		, GPUHandle({ 0 })
	{
	}

	D3D12DescriptorHandle::D3D12DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE InCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE InGPUHandle)
		: CPUHandle(InCPUHandle)
		, GPUHandle(InGPUHandle)
	{
	}

	D3D12DescriptorHandle::~D3D12DescriptorHandle()
	{
	}

	FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const
	{
		return CPUHandle;
	}

	FORCEINLINE D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const
	{
		return GPUHandle;
	}

private:
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
};

/*
* D3D12View
*/

class D3D12View : public D3D12DeviceChild
{
public:
	// DeviceChild Interface
	virtual void SetName(const std::string& Name) override;

protected:
	D3D12View(D3D12Device* InDevice, ID3D12Resource* InResource);
	~D3D12View();

	FORCEINLINE ID3D12Resource* GetResource() const
	{
		return Resource.Get();
	}

protected:
	D3D12DescriptorHandle					Descriptor;
	Microsoft::WRL::ComPtr<ID3D12Resource>	Resource;
};

/*
* D3D12ShaderResourceView
*/

class D3D12ShaderResourceView : public D3D12View
{
public:
	D3D12ShaderResourceView(D3D12Device* InDevice, ID3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* InDesc);
	~D3D12ShaderResourceView() = default;

private:
	D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
};

/*
* D3D12UnorderedAccessView
*/

class D3D12UnorderedAccessView : public D3D12View
{
public:
	D3D12UnorderedAccessView(D3D12Device* InDevice, ID3D12Resource* InCounterResource, ID3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* InDesc);
	~D3D12UnorderedAccessView() = default;

private:
	Microsoft::WRL::ComPtr<ID3D12Resource>	CounterResource;
	D3D12_UNORDERED_ACCESS_VIEW_DESC		Desc;
};

/*
* D3D12RenderTargetView
*/

class D3D12RenderTargetView : public D3D12View
{
public:
	D3D12RenderTargetView(D3D12Device* InDevice, ID3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC* InDesc);
	~D3D12RenderTargetView() = default;

private:
	D3D12_RENDER_TARGET_VIEW_DESC Desc;
};

/*
* D3D12DepthStencilView
*/

class D3D12DepthStencilView : public D3D12View
{
public:
	D3D12DepthStencilView(D3D12Device* InDevice, ID3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC* InDesc);
	~D3D12DepthStencilView() = default;

private:
	D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
};