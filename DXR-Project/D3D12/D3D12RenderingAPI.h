#pragma once
#include "Windows/WindowsWindow.h"

#include "RenderingCore/RenderingAPI.h"

#include "D3D12Device.h"
#include "D3D12ImmediateCommandList.h"
#include "D3D12SwapChain.h"

/*
* D3D12RenderingAPI
*/

class D3D12RenderingAPI : public RenderingAPI
{
public:
	D3D12RenderingAPI();
	~D3D12RenderingAPI();

	virtual bool Initialize(TSharedRef<GenericWindow> RenderWindow, bool EnableDebug) override final;

	virtual class D3D12Texture*	CreateTexture(const struct TextureProperties& Properties) const override final;
	virtual class D3D12Buffer*	CreateBuffer(const struct BufferProperties& Properties) const override final;
	virtual class D3D12RayTracingScene*		CreateRayTracingScene(class D3D12RayTracingPipelineState* PipelineState) const override final;
	virtual class D3D12RayTracingGeometry*	CreateRayTracingGeometry() const override final;
	virtual class D3D12DescriptorTable*		CreateDescriptorTable(Uint32 DescriptorCount) const override final;

	virtual class D3D12ShaderResourceView*	CreateShaderResourceView(ID3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* InDesc) const override final;
	virtual class D3D12UnorderedAccessView* CreateUnorderedAccessView(ID3D12Resource* InCounterResource, ID3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* InDesc) const override final;
	virtual class D3D12RenderTargetView*	CreateRenderTargetView(ID3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC* InDesc) const override final;
	virtual class D3D12DepthStencilView*	CreateDepthStencilView(ID3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC* InDesc) const override final;
	virtual class D3D12ConstantBufferView*	CreateConstantBufferView(ID3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC* InDesc) const override final;

	virtual class D3D12Fence*				CreateFence(Uint64 InitalValue) const override final;
	virtual class D3D12CommandAllocator*	CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE ListType) const override final;
	virtual class D3D12CommandList*			CreateCommandList(D3D12_COMMAND_LIST_TYPE Type, D3D12CommandAllocator* Allocator, ID3D12PipelineState* InitalPipeline) const override final;
	virtual class D3D12CommandQueue*		CreateCommandQueue() const override final;

	virtual class D3D12ComputePipelineState*	CreateComputePipelineState(const struct ComputePipelineStateProperties& Properties) const override final;
	virtual class D3D12GraphicsPipelineState*	CreateGraphicsPipelineState(const struct GraphicsPipelineStateProperties& Properties) const override final;
	virtual class D3D12RayTracingPipelineState*	CreateRayTracingPipelineState(const struct RayTracingPipelineStateProperties& Properties) const override final;
	virtual D3D12RootSignature* CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& Desc) const  override final;
	virtual D3D12RootSignature* CreateRootSignature(IDxcBlob* ShaderBlob) const override final;

	virtual class D3D12CommandQueue*	GetQueue() const override final;
	virtual class D3D12SwapChain*		GetSwapChain() const override final;
	virtual TSharedPtr<D3D12ImmediateCommandList> GetImmediateCommandList() const override final;

	virtual std::string GetAdapterName() const override final
	{
		return Device->GetAdapterName();
	}

	virtual bool IsRayTracingSupported() const override final;

	virtual bool UAVSupportsFormat(DXGI_FORMAT Format) const override final;

private:
	TSharedRef<WindowsWindow>				RenderWindow;
	TSharedPtr<D3D12SwapChain>				SwapChain;
	TSharedPtr<D3D12Device>					Device;
	TSharedPtr<D3D12CommandQueue>			Queue;
	TSharedPtr<D3D12CommandQueue>			ComputeQueue;
	TSharedPtr<D3D12ImmediateCommandList>	ImmediateCommandList;
};