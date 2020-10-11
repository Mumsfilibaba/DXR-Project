#pragma once
#include "D3D12DeviceChild.h"

class D3D12Device;
class D3D12OfflineDescriptorHeap;

/*
* D3D12View
*/

class D3D12View : public D3D12DeviceChild
{
public:
	D3D12View(D3D12Device* InDevice, ID3D12Resource* InResource);
	virtual ~D3D12View();

	void ResetResource();

	FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const
	{
		return OfflineHandle;
	}

	FORCEINLINE ID3D12Resource* GetResource() const
	{
		return Resource.Get();
	}

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	
	D3D12OfflineDescriptorHeap* Heap				= nullptr;
	Uint32						OfflineHeapIndex	= 0;
	D3D12_CPU_DESCRIPTOR_HANDLE	OfflineHandle;
};

/*
* D3D12ConstantBufferView
*/

class D3D12ConstantBufferView : public D3D12View
{
public:
	D3D12ConstantBufferView(D3D12Device* InDevice, ID3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc);
	~D3D12ConstantBufferView() = default;

	void CreateView(ID3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC& InDesc);

	FORCEINLINE const D3D12_CONSTANT_BUFFER_VIEW_DESC& GetDesc() const
	{
		return Desc;
	}

private:
	D3D12_CONSTANT_BUFFER_VIEW_DESC Desc;
};

/*
* D3D12ShaderResourceView
*/

class D3D12ShaderResourceView : public D3D12View
{
public:
	D3D12ShaderResourceView(D3D12Device* InDevice, ID3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc);
	~D3D12ShaderResourceView() = default;

	void CreateView(ID3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& InDesc);

	FORCEINLINE const D3D12_SHADER_RESOURCE_VIEW_DESC& GetDesc() const
	{
		return Desc;
	}

private:
	D3D12_SHADER_RESOURCE_VIEW_DESC Desc;
};

/*
* D3D12UnorderedAccessView
*/

class D3D12UnorderedAccessView : public D3D12View
{
public:
	D3D12UnorderedAccessView(D3D12Device* InDevice, ID3D12Resource* InCounterResource, ID3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc);
	~D3D12UnorderedAccessView() = default;

	void CreateView(ID3D12Resource* InCounterResource, ID3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& InDesc);

	FORCEINLINE const D3D12_UNORDERED_ACCESS_VIEW_DESC& GetDesc() const
	{
		return Desc;
	}

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
	D3D12RenderTargetView(D3D12Device* InDevice, ID3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc);
	~D3D12RenderTargetView() = default;

	void CreateView(ID3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC& InDesc);

	FORCEINLINE const D3D12_RENDER_TARGET_VIEW_DESC& GetDesc() const
	{
		return Desc;
	}

private:
	D3D12_RENDER_TARGET_VIEW_DESC Desc;
};

/*
* D3D12DepthStencilView
*/

class D3D12DepthStencilView : public D3D12View
{
public:
	D3D12DepthStencilView(D3D12Device* InDevice, ID3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc);
	~D3D12DepthStencilView() = default;

	void CreateView(ID3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC& InDesc);

	FORCEINLINE const D3D12_DEPTH_STENCIL_VIEW_DESC& GetDesc() const
	{
		return Desc;
	}

private:
	D3D12_DEPTH_STENCIL_VIEW_DESC Desc;
};